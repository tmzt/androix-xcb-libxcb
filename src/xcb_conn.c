/* Copyright (C) 2001-2004 Bart Massey and Jamey Sharp.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 * Except as contained in this notice, the names of the authors or their
 * institutions shall not be used in advertising or otherwise to promote the
 * sale, use or other dealings in this Software without prior written
 * authorization from the authors.
 */

/* Connection management: the core of XCB. */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/select.h>

#include "xcb.h"
#include "xcbint.h"

static int write_setup(XCBConnection *c, XCBAuthInfo *auth_info)
{
    XCBConnSetupReq out;
    struct iovec parts[3];
    int count = 0;
    int endian = 0x01020304;
    int ret;

    memset(&out, 0, sizeof(out));

    /* B = 0x42 = MSB first, l = 0x6c = LSB first */
    if(htonl(endian) == endian)
        out.byte_order = 0x42;
    else
        out.byte_order = 0x6c;
    out.protocol_major_version = X_PROTOCOL;
    out.protocol_minor_version = X_PROTOCOL_REVISION;
    out.authorization_protocol_name_len = 0;
    out.authorization_protocol_data_len = 0;
    parts[count].iov_len = sizeof(XCBConnSetupReq);
    parts[count++].iov_base = &out;

    if(auth_info)
    {
        parts[count].iov_len = out.authorization_protocol_name_len = auth_info->namelen;
        parts[count++].iov_base = auth_info->name;
        parts[count].iov_len = out.authorization_protocol_data_len = auth_info->datalen;
        parts[count++].iov_base = auth_info->data;
    }

    pthread_mutex_lock(&c->iolock);
    _xcb_out_write_block(c, parts, count);
    ret = _xcb_out_flush(c);
    pthread_mutex_unlock(&c->iolock);
    if(ret <= 0)
        return 0;
    return 1;
}

static int read_setup(XCBConnection *c)
{
    /* Read the server response */
    c->setup = malloc(sizeof(XCBConnSetupGenericRep));
    if(!c->setup)
        return 0;

    if(_xcb_read_block(c->fd, c->setup, sizeof(XCBConnSetupGenericRep)) != sizeof(XCBConnSetupGenericRep))
        return 0;

    {
        void *tmp = realloc(c->setup, c->setup->length * 4 + sizeof(XCBConnSetupGenericRep));
        if(!tmp)
            return 0;
        c->setup = tmp;
    }

    if(_xcb_read_block(c->fd, (char *) c->setup + sizeof(XCBConnSetupGenericRep), c->setup->length * 4) <= 0)
        return 0;

    /* 0 = failed, 2 = authenticate, 1 = success */
    switch(c->setup->status)
    {
    case 0: /* failed */
        {
            XCBConnSetupFailedRep *setup = (XCBConnSetupFailedRep *) c->setup;
            write(STDERR_FILENO, XCBConnSetupFailedRepReason(setup), XCBConnSetupFailedRepReasonLength(setup));
            return 0;
        }

    case 2: /* authenticate */
        {
            XCBConnSetupAuthenticateRep *setup = (XCBConnSetupAuthenticateRep *) c->setup;
            write(STDERR_FILENO, XCBConnSetupAuthenticateRepReason(setup), XCBConnSetupAuthenticateRepReasonLength(setup));
            return 0;
        }
    }

    return 1;
}

/* Public interface */

XCBConnSetupSuccessRep *XCBGetSetup(XCBConnection *c)
{
    /* doesn't need locking because it's never written to. */
    return c->setup;
}

int XCBGetFileDescriptor(XCBConnection *c)
{
    /* doesn't need locking because it's never written to. */
    return c->fd;
}

XCBConnection *XCBConnectToFD(int fd, XCBAuthInfo *auth_info)
{
    XCBConnection* c;

    c = calloc(1, sizeof(XCBConnection));
    if(!c)
        return 0;

    c->fd = fd;

    if(!(
        _xcb_set_fd_flags(fd) &&
        pthread_mutex_init(&c->iolock, 0) == 0 &&
        _xcb_in_init(&c->in) &&
        _xcb_out_init(&c->out) &&
        write_setup(c, auth_info) &&
        read_setup(c) &&
        _xcb_ext_init(c) &&
        _xcb_xid_init(c)
        ))
    {
        XCBDisconnect(c);
        return 0;
    }

    return c;
}

void XCBDisconnect(XCBConnection *c)
{
    if(!c)
        return;

    free(c->setup);
    close(c->fd);

    pthread_mutex_destroy(&c->iolock);
    _xcb_in_destroy(&c->in);
    _xcb_out_destroy(&c->out);

    _xcb_ext_destroy(c);
    _xcb_xid_destroy(c);

    free(c);
}

/* Private interface */

int _xcb_conn_wait(XCBConnection *c, const int should_write, pthread_cond_t *cond)
{
    int ret = 1;
    fd_set rfds, wfds;
#if USE_THREAD_ASSERT
    static __thread int already_here = 0;

    assert(!already_here);
    ++already_here;
#endif

    _xcb_assert_valid_sequence(c);

    /* If the thing I should be doing is already being done, wait for it. */
    if(should_write ? c->out.writing : c->in.reading)
    {
        pthread_cond_wait(cond, &c->iolock);
#if USE_THREAD_ASSERT
        --already_here;
#endif
        return 1;
    }

    FD_ZERO(&rfds);
    FD_SET(c->fd, &rfds);
    ++c->in.reading;

    FD_ZERO(&wfds);
    if(should_write)
    {
        FD_SET(c->fd, &wfds);
        ++c->out.writing;
    }

    pthread_mutex_unlock(&c->iolock);
    ret = select(c->fd + 1, &rfds, &wfds, 0, 0);
    pthread_mutex_lock(&c->iolock);

    if(ret <= 0) /* error: select failed */
        goto done;

    if(FD_ISSET(c->fd, &rfds))
        if((ret = _xcb_in_read(c)) <= 0)
            goto done;

    if(FD_ISSET(c->fd, &wfds))
        if((ret = _xcb_out_write(c)) <= 0)
            goto done;

done:
    if(should_write)
        --c->out.writing;
    --c->in.reading;

#if USE_THREAD_ASSERT
    --already_here;
#endif
    return ret;
}
