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
#include <fcntl.h>
#include <errno.h>

#include "xcb.h"
#include "xcbint.h"

typedef struct {
    CARD8  status;
    CARD8  pad0[5];
    CARD16 length;
} XCBSetupGeneric;

static const int error_connection = 1;

static int set_fd_flags(const int fd)
{
    long flags = fcntl(fd, F_GETFL, 0);
    if(flags == -1)
        return 0;
    flags |= O_NONBLOCK;
    if(fcntl(fd, F_SETFL, flags) == -1)
        return 0;
    if(fcntl(fd, F_SETFD, FD_CLOEXEC) == -1)
        return 0;
    return 1;
}

static int write_setup(XCBConnection *c, XCBAuthInfo *auth_info)
{
    static const char pad[3];
    XCBSetupReq out;
    struct iovec parts[6];
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
    parts[count].iov_len = sizeof(XCBSetupReq);
    parts[count++].iov_base = &out;
    parts[count].iov_len = XCB_PAD(sizeof(XCBSetupReq));
    parts[count++].iov_base = (char *) pad;

    if(auth_info)
    {
        parts[count].iov_len = out.authorization_protocol_name_len = auth_info->namelen;
        parts[count++].iov_base = auth_info->name;
        parts[count].iov_len = XCB_PAD(out.authorization_protocol_name_len);
        parts[count++].iov_base = (char *) pad;
        parts[count].iov_len = out.authorization_protocol_data_len = auth_info->datalen;
        parts[count++].iov_base = auth_info->data;
        parts[count].iov_len = XCB_PAD(out.authorization_protocol_data_len);
        parts[count++].iov_base = (char *) pad;
    }
    assert(count <= sizeof(parts) / sizeof(*parts));

    pthread_mutex_lock(&c->iolock);
    {
        struct iovec *parts_ptr = parts;
        ret = _xcb_out_send(c, &parts_ptr, &count);
    }
    pthread_mutex_unlock(&c->iolock);
    return ret;
}

static int read_setup(XCBConnection *c)
{
    /* Read the server response */
    c->setup = malloc(sizeof(XCBSetupGeneric));
    if(!c->setup)
        return 0;

    if(_xcb_in_read_block(c, c->setup, sizeof(XCBSetupGeneric)) != sizeof(XCBSetupGeneric))
        return 0;

    {
        void *tmp = realloc(c->setup, c->setup->length * 4 + sizeof(XCBSetupGeneric));
        if(!tmp)
            return 0;
        c->setup = tmp;
    }

    if(_xcb_in_read_block(c, (char *) c->setup + sizeof(XCBSetupGeneric), c->setup->length * 4) <= 0)
        return 0;

    /* 0 = failed, 2 = authenticate, 1 = success */
    switch(c->setup->status)
    {
    case 0: /* failed */
        {
            XCBSetupFailed *setup = (XCBSetupFailed *) c->setup;
            write(STDERR_FILENO, XCBSetupFailedReason(setup), XCBSetupFailedReasonLength(setup));
            return 0;
        }

    case 2: /* authenticate */
        {
            XCBSetupAuthenticate *setup = (XCBSetupAuthenticate *) c->setup;
            write(STDERR_FILENO, XCBSetupAuthenticateReason(setup), XCBSetupAuthenticateReasonLength(setup));
            return 0;
        }
    }

    return 1;
}

/* precondition: there must be something for us to write. */
static int write_vec(XCBConnection *c, struct iovec **vector, int *count)
{
    int n;
    assert(!c->out.queue_len);
    n = writev(c->fd, *vector, *count);
    if(n < 0 && errno == EAGAIN)
        return 1;
    if(n <= 0)
        return 0;

    for(; *count; --*count, ++*vector)
    {
        int cur = (*vector)->iov_len;
        if(cur > n)
            cur = n;
        (*vector)->iov_len -= cur;
        (*vector)->iov_base = (char *) (*vector)->iov_base + cur;
        n -= cur;
        if((*vector)->iov_len)
            break;
    }
    if(!*count)
        *vector = 0;
    assert(n == 0);
    return 1;
}

/* Public interface */

const XCBSetup *XCBGetSetup(XCBConnection *c)
{
    /* doesn't need locking because it's never written to. */
    return c->setup;
}

int XCBGetFileDescriptor(XCBConnection *c)
{
    /* doesn't need locking because it's never written to. */
    return c->fd;
}

int XCBConnectionHasError(XCBConnection *c)
{
    /* doesn't need locking because it's read and written atomically. */
    return c->has_error;
}

XCBConnection *XCBConnectToFD(int fd, XCBAuthInfo *auth_info)
{
    XCBConnection* c;

    c = calloc(1, sizeof(XCBConnection));
    if(!c)
        return (XCBConnection *) &error_connection;

    c->fd = fd;

    if(!(
        set_fd_flags(fd) &&
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
        return (XCBConnection *) &error_connection;
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

int _xcb_conn_wait(XCBConnection *c, pthread_cond_t *cond, struct iovec **vector, int *count)
{
    int ret;
    fd_set rfds, wfds;

    /* If the thing I should be doing is already being done, wait for it. */
    if(count ? c->out.writing : c->in.reading)
    {
        pthread_cond_wait(cond, &c->iolock);
        return 1;
    }

    FD_ZERO(&rfds);
    FD_SET(c->fd, &rfds);
    ++c->in.reading;

    FD_ZERO(&wfds);
    if(count)
    {
        FD_SET(c->fd, &wfds);
        ++c->out.writing;
    }

    pthread_mutex_unlock(&c->iolock);
    do {
	ret = select(c->fd + 1, &rfds, &wfds, 0, 0);
    } while (ret == -1 && errno == EINTR);
    if (ret < 0)
	ret = 0;
    pthread_mutex_lock(&c->iolock);

    if(ret)
    {
        if(FD_ISSET(c->fd, &rfds))
            ret = ret && _xcb_in_read(c);

        if(FD_ISSET(c->fd, &wfds))
            ret = ret && write_vec(c, vector, count);
    }

    if(count)
        --c->out.writing;
    --c->in.reading;

    return ret;
}
