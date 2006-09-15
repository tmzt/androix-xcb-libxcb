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

#include "xcb.h"
#include "xcbext.h"
#include "xcbint.h"
#include "extensions/bigreq.h"

static int write_block(XCBConnection *c, struct iovec *vector, int count)
{
    while(count && c->out.queue_len + vector[0].iov_len <= sizeof(c->out.queue))
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
    return _xcb_out_send(c, &vector, &count);
}

/* Public interface */

CARD32 XCBGetMaximumRequestLength(XCBConnection *c)
{
    if(c->has_error)
        return 0;
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

unsigned int XCBSendRequest(XCBConnection *c, int flags, struct iovec *vector, const XCBProtocolRequest *req)
{
    static const union {
        struct {
            CARD8 major;
            CARD8 pad;
            CARD16 len;
        } fields;
        CARD32 packet;
    } sync = { { /* GetInputFocus */ 43, 0, 1 } };
    unsigned int request;
    CARD32 prefix[3] = { 0 };
    int veclen = req->count;
    enum workarounds workaround = WORKAROUND_NONE;

    if(c->has_error)
        return 0;

    assert(c != 0);
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
            if(!(extension && extension->present))
                return 0;
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
                vector[i].iov_base = (char *) pad;
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
            prefix[2] = ++longlen;
    }
    flags &= ~XCB_REQUEST_RAW;

    /* do we need to work around the X server bug described in glx.xml? */
    /* XXX: GetFBConfigs won't use BIG-REQUESTS in any sane
     * configuration, but that should be handled here anyway. */
    if(req->ext && !req->isvoid && !strcmp(req->ext->name, "GLX") &&
            ((req->opcode == 17 && ((CARD32 *) vector[0].iov_base)[1] == 0x10004) ||
             req->opcode == 21))
        workaround = WORKAROUND_GLX_GET_FB_CONFIGS_BUG;

    /* get a sequence number and arrange for delivery. */
    pthread_mutex_lock(&c->iolock);
    /* wait for other writing threads to get out of my way. */
    while(c->out.writing)
        pthread_cond_wait(&c->out.cond, &c->iolock);

    request = ++c->out.request;
    /* send GetInputFocus (sync) when 64k-2 requests have been sent without
     * a reply.
     * Also send sync (could use NoOp) at 32-bit wrap to avoid having
     * applications see sequence 0 as that is used to indicate
     * an error in sending the request */
    while((req->isvoid &&
	c->out.request == c->in.request_expected + (1 << 16) - 1) ||
       request == 0)
    {
        prefix[0] = sync.packet;
        _xcb_in_expect_reply(c, request, WORKAROUND_NONE, XCB_REQUEST_DISCARD_REPLY);
        c->in.request_expected = c->out.request;
	request = ++c->out.request;
    }

    if(workaround != WORKAROUND_NONE || flags != 0)
        _xcb_in_expect_reply(c, request, workaround, flags);
    if(!req->isvoid)
        c->in.request_expected = c->out.request;

    if(prefix[0] || prefix[2])
    {
        --vector, ++veclen;
        if(prefix[2])
        {
            prefix[1] = ((CARD32 *) vector[1].iov_base)[0];
            vector[1].iov_base = (CARD32 *) vector[1].iov_base + 1;
            vector[1].iov_len -= sizeof(CARD32);
        }
        vector[0].iov_len = sizeof(CARD32) * (prefix[0] ? 1 : 0 | prefix[2] ? 2 : 0);
        vector[0].iov_base = prefix + !prefix[0];
    }

    if(!write_block(c, vector, veclen))
        request = 0;
    pthread_mutex_unlock(&c->iolock);
    return request;
}

int XCBFlush(XCBConnection *c)
{
    int ret;
    if(c->has_error)
        return 0;
    pthread_mutex_lock(&c->iolock);
    ret = _xcb_out_flush_to(c, c->out.request);
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

int _xcb_out_send(XCBConnection *c, struct iovec **vector, int *count)
{
    int ret = 1;
    while(ret && *count)
        ret = _xcb_conn_wait(c, &c->out.cond, vector, count);
    c->out.request_written = c->out.request;
    pthread_cond_broadcast(&c->out.cond);
    return ret;
}

int _xcb_out_flush_to(XCBConnection *c, unsigned int request)
{
    assert(XCB_SEQUENCE_COMPARE(request, <=, c->out.request));
    if(XCB_SEQUENCE_COMPARE(c->out.request_written, >=, request))
        return 1;
    if(c->out.queue_len)
    {
        struct iovec vec, *vec_ptr = &vec;
        int count = 1;
        vec.iov_base = c->out.queue;
        vec.iov_len = c->out.queue_len;
        c->out.queue_len = 0;
        return _xcb_out_send(c, &vec_ptr, &count);
    }
    while(c->out.writing)
        pthread_cond_wait(&c->out.cond, &c->iolock);
    assert(XCB_SEQUENCE_COMPARE(c->out.request_written, >=, request));
    return 1;
}
