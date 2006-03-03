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

/* Stuff that sends stuff to the server. */

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "xcb.h"
#include "xcbext.h"
#include "xcbint.h"
#include "extensions/bigreq.h"

static int force_sequence_wrap(XCBConnection *c)
{
    int ret = 1;
    if((c->out.request - c->in.request_read) > 65530)
    {
        pthread_mutex_unlock(&c->iolock);
        ret = XCBSync(c, 0);
        pthread_mutex_lock(&c->iolock);
    }
    return ret;
}

/* Public interface */

CARD32 XCBGetMaximumRequestLength(XCBConnection *c)
{
    pthread_mutex_lock(&c->out.reqlenlock);
    if(!c->out.maximum_request_length)
    {
        const XCBQueryExtensionRep *ext;
        c->out.maximum_request_length = c->setup->maximum_request_length;
        ext = XCBGetExtensionData(c, &XCBBigRequestsId);
        if(ext && ext->present)
        {
            XCBBigRequestsEnableRep *r = XCBBigRequestsEnableReply(c, XCBBigRequestsEnable(c), 0);
            c->out.maximum_request_length = r->maximum_request_length;
            free(r);
        }
    }
    pthread_mutex_unlock(&c->out.reqlenlock);
    return c->out.maximum_request_length;
}

int XCBSendRequest(XCBConnection *c, unsigned int *request, int flags, struct iovec *vector, const XCBProtocolRequest *req)
{
    int ret;
    CARD32 prefix[2];
    int veclen = req->count;
    enum workarounds workaround = WORKAROUND_NONE;

    assert(c != 0);
    assert(request != 0);
    assert(vector != 0);
    assert(req->count > 0);

    if(!(flags & XCB_REQUEST_RAW))
    {
        static const char pad[3];
        int i;
        CARD16 shortlen = 0;
        size_t longlen = 0;
        assert(vector[0].iov_len >= 4);
        /* set the major opcode, and the minor opcode for extensions */
        if(req->ext)
        {
            const XCBQueryExtensionRep *extension = XCBGetExtensionData(c, req->ext);
            /* TODO: better error handling here, please! */
            assert(extension && extension->present);
            ((CARD8 *) vector[0].iov_base)[0] = extension->major_opcode;
            ((CARD8 *) vector[0].iov_base)[1] = req->opcode;
        }
        else
            ((CARD8 *) vector[0].iov_base)[0] = req->opcode;

        /* put together the length field, possibly using BIGREQUESTS */
        for(i = 0; i < req->count; ++i)
        {
            longlen += vector[i].iov_len;
            if(!vector[i].iov_base)
            {
                vector[i].iov_base = (caddr_t) pad;
                assert(vector[i].iov_len <= sizeof(pad));
            }
        }
        assert((longlen & 3) == 0);
        longlen >>= 2;

        if(longlen <= c->setup->maximum_request_length)
        {
            /* we don't need BIGREQUESTS. */
            shortlen = longlen;
            longlen = 0;
        }
        else if(longlen > XCBGetMaximumRequestLength(c))
            return 0; /* server can't take this; maybe need BIGREQUESTS? */

        /* set the length field. */
        ((CARD16 *) vector[0].iov_base)[1] = shortlen;
        if(!shortlen)
        {
            --vector, ++veclen;
            vector[0].iov_base = prefix;
            vector[0].iov_len = sizeof(prefix);
            prefix[0] = ((CARD32 *) vector[0].iov_base)[0];
            prefix[1] = ++longlen;
            vector[1].iov_base = ((char *) vector[1].iov_base) + sizeof(CARD32);
            vector[1].iov_len -= sizeof(CARD32);
        }
    }
    flags &= ~XCB_REQUEST_RAW;

    /* do we need to work around the X server bug described in glx.xml? */
    if(req->ext && !req->isvoid && strcmp(req->ext->name, "GLX") &&
            ((req->opcode == 17 && ((CARD32 *) vector[0].iov_base)[0] == 0x10004) ||
             req->opcode == 21))
        workaround = WORKAROUND_GLX_GET_FB_CONFIGS_BUG;

    /* get a sequence number and arrange for delivery. */
    pthread_mutex_lock(&c->iolock);
    if(req->isvoid && !force_sequence_wrap(c))
    {
        pthread_mutex_unlock(&c->iolock);
        return -1;
    }

    /* wait for other writing threads to get out of my way. */
    while(c->out.writing)
        pthread_cond_wait(&c->out.cond, &c->iolock);

    *request = ++c->out.request;

    _xcb_in_expect_reply(c, *request, workaround, flags);

    ret = _xcb_out_write_block(c, vector, veclen);
    pthread_mutex_unlock(&c->iolock);
    return ret;
}

int XCBFlush(XCBConnection *c)
{
    int ret;
    pthread_mutex_lock(&c->iolock);
    ret = _xcb_out_flush(c);
    pthread_mutex_unlock(&c->iolock);
    return ret;
}

/* Private interface */

int _xcb_out_init(_xcb_out *out)
{
    if(pthread_cond_init(&out->cond, 0))
        return 0;
    out->writing = 0;

    out->queue_len = 0;
    out->vec = 0;
    out->vec_len = 0;

    out->request = 0;
    out->request_written = 0;

    if(pthread_mutex_init(&out->reqlenlock, 0))
        return 0;
    out->maximum_request_length = 0;

    return 1;
}

void _xcb_out_destroy(_xcb_out *out)
{
    pthread_cond_destroy(&out->cond);
    pthread_mutex_destroy(&out->reqlenlock);
}

/* precondition: there must be something for us to write. */
int _xcb_out_write(XCBConnection *c)
{
    int n;
    assert(!c->out.queue_len);
    n = writev(c->fd, c->out.vec, c->out.vec_len);
    if(n < 0 && errno == EAGAIN)
        return 1;
    if(n <= 0)
        return 0;

    for(; c->out.vec_len; --c->out.vec_len, ++c->out.vec)
    {
        int cur = c->out.vec->iov_len;
        if(cur > n)
            cur = n;
        c->out.vec->iov_len -= cur;
        c->out.vec->iov_base = (char *) c->out.vec->iov_base + cur;
        n -= cur;
        if(c->out.vec->iov_len)
            break;
    }
    if(!c->out.vec_len)
        c->out.vec = 0;
    assert(n == 0);
    return 1;
}

int _xcb_out_write_block(XCBConnection *c, struct iovec *vector, size_t count)
{
    assert(!c->out.vec && !c->out.vec_len);
    while(count && c->out.queue_len + vector[0].iov_len < sizeof(c->out.queue))
    {
        memcpy(c->out.queue + c->out.queue_len, vector[0].iov_base, vector[0].iov_len);
        c->out.queue_len += vector[0].iov_len;
        vector[0].iov_base = (char *) vector[0].iov_base + vector[0].iov_len;
        vector[0].iov_len = 0;
        ++vector, --count;
    }
    if(!count)
        return 1;

    --vector, ++count;
    vector[0].iov_base = c->out.queue;
    vector[0].iov_len = c->out.queue_len;
    c->out.queue_len = 0;

    c->out.vec_len = count;
    c->out.vec = vector;
    return _xcb_out_flush(c);
}

int _xcb_out_flush(XCBConnection *c)
{
    int ret = 1;
    struct iovec vec;
    if(c->out.queue_len)
    {
        assert(!c->out.vec && !c->out.vec_len);
        vec.iov_base = c->out.queue;
        vec.iov_len = c->out.queue_len;
        c->out.vec = &vec;
        c->out.vec_len = 1;
        c->out.queue_len = 0;
    }
    while(ret && c->out.vec_len)
        ret = _xcb_conn_wait(c, /*should_write*/ 1, &c->out.cond);
    c->out.request_written = c->out.request;
    pthread_cond_broadcast(&c->out.cond);
    return ret;
}
