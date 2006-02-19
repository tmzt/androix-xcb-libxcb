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

int XCBSendRequest(XCBConnection *c, unsigned int *request, struct iovec *vector, const XCBProtocolRequest *req)
{
    int ret;
    int i;
    struct iovec prefix[2];
    CARD16 shortlen = 0;
    CARD32 longlen = 0;

    assert(c != 0);
    assert(request != 0);
    assert(vector != 0);
    assert(req->count > 0);

    /* put together the length field, possibly using BIGREQUESTS */
    for(i = 0; i < req->count; ++i)
        longlen += XCB_CEIL(vector[i].iov_len) >> 2;

    if(longlen > c->setup->maximum_request_length)
    {
        if(longlen > XCBGetMaximumRequestLength(c))
            return 0; /* server can't take this; maybe need BIGREQUESTS? */
    }
    else
    {
        /* we don't need BIGREQUESTS. */
        shortlen = longlen;
        longlen = 0;
    }

    /* set the length field. */
    i = 0;
    prefix[i].iov_base = vector[0].iov_base;
    prefix[i].iov_len = sizeof(CARD32);
    vector[0].iov_base = ((char *) vector[0].iov_base) + sizeof(CARD32);
    vector[0].iov_len -= sizeof(CARD32);
    ((CARD16 *) prefix[i].iov_base)[1] = shortlen;
    ++i;
    if(!shortlen)
    {
        ++longlen;
        prefix[i].iov_base = &longlen;
        prefix[i].iov_len = sizeof(CARD32);
        ++i;
    }

    /* set the major opcode, and the minor opcode for extensions */
    if(req->ext)
    {
        const XCBQueryExtensionRep *extension = XCBGetExtensionData(c, req->ext);
        /* TODO: better error handling here, please! */
        assert(extension && extension->present);
        ((CARD8 *) prefix[0].iov_base)[0] = extension->major_opcode;
        ((CARD8 *) prefix[0].iov_base)[1] = req->opcode;
    }
    else
        ((CARD8 *) prefix[0].iov_base)[0] = req->opcode;

    /* get a sequence number and arrange for delivery. */
    pthread_mutex_lock(&c->iolock);
    if(req->isvoid && !force_sequence_wrap(c))
    {
        pthread_mutex_unlock(&c->iolock);
        return -1;
    }

    *request = ++c->out.request;

    if(!req->isvoid)
        _xcb_in_expect_reply(c, *request);

    ret = _xcb_out_write_block(c, prefix, i);
    if(ret > 0)
        ret = _xcb_out_write_block(c, vector, req->count);
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

    out->last_request = 0;
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
    free(out->vec);
}

int _xcb_out_write(XCBConnection *c)
{
    int n;
    if(c->out.vec_len)
        n = _xcb_writev(c->fd, c->out.vec, c->out.vec_len);
    else
        n = _xcb_write(c->fd, &c->out.queue, &c->out.queue_len);

    if(n < 0 && errno == EAGAIN)
        n = 1;

    if(c->out.vec_len)
    {
        int i;
        for(i = 0; i < c->out.vec_len; ++i)
            if(c->out.vec[i].iov_len)
                return n;
        c->out.vec_len = 0;
    }
    return n;
}

int _xcb_out_write_block(XCBConnection *c, struct iovec *vector, size_t count)
{
    static const char pad[3];
    int i;
    int len = 0;

    for(i = 0; i < count; ++i)
        len += XCB_CEIL(vector[i].iov_len);

    /* Is the queue about to overflow? */
    if(c->out.queue_len + len < sizeof(c->out.queue))
    {
        /* No, this will fit. */
        for(i = 0; i < count; ++i)
        {
            memcpy(c->out.queue + c->out.queue_len, vector[i].iov_base, vector[i].iov_len);
            if(vector[i].iov_len & 3)
                memset(c->out.queue + c->out.queue_len + vector[i].iov_len, 0, XCB_PAD(vector[i].iov_len));
            c->out.queue_len += XCB_CEIL(vector[i].iov_len);
        }
        return len;
    }

    assert(!c->out.vec_len);
    assert(!c->out.vec);
    c->out.vec = malloc(sizeof(struct iovec) * (1 + count * 2));
    if(!c->out.vec)
        return -1;
    if(c->out.queue_len)
    {
        c->out.vec[c->out.vec_len].iov_base = c->out.queue;
        c->out.vec[c->out.vec_len++].iov_len = c->out.queue_len;
        c->out.queue_len = 0;
    }
    for(i = 0; i < count; ++i)
    {
        if(!vector[i].iov_len)
            continue;
        c->out.vec[c->out.vec_len].iov_base = vector[i].iov_base;
        c->out.vec[c->out.vec_len++].iov_len = vector[i].iov_len;
        if(!XCB_PAD(vector[i].iov_len))
            continue;
        c->out.vec[c->out.vec_len].iov_base = (void *) pad;
        c->out.vec[c->out.vec_len++].iov_len = XCB_PAD(vector[i].iov_len);
    }
    if(_xcb_out_flush(c) <= 0)
        len = -1;
    free(c->out.vec);
    c->out.vec = 0;

    return len;
}

int _xcb_out_flush(XCBConnection *c)
{
    int ret = 1;
    while(ret > 0 && (c->out.queue_len || c->out.vec_len))
        ret = _xcb_conn_wait(c, /*should_write*/ 1, &c->out.cond);
    c->out.last_request = 0;
    c->out.request_written = c->out.request;
    pthread_cond_broadcast(&c->out.cond);
    return ret;
}
