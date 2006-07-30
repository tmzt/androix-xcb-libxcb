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

/* Stuff that reads stuff from the server. */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/select.h>
#include <errno.h>

#include "xcb.h"
#include "xcbext.h"
#include "xcbint.h"

#define XCBError 0
#define XCBReply 1

struct event_list {
    XCBGenericEvent *event;
    struct event_list *next;
};

struct reply_list {
    void *reply;
    struct reply_list *next;
};

typedef struct pending_reply {
    unsigned int request;
    enum workarounds workaround;
    int flags;
    struct pending_reply *next;
} pending_reply;

typedef struct reader_list {
    unsigned int request;
    pthread_cond_t *data;
    struct reader_list *next;
} reader_list;

static void wake_up_next_reader(XCBConnection *c)
{
    int pthreadret;
    if(c->in.readers)
        pthreadret = pthread_cond_signal(c->in.readers->data);
    else
        pthreadret = pthread_cond_signal(&c->in.event_cond);
    assert(pthreadret == 0);
}

static int read_packet(XCBConnection *c)
{
    XCBGenericRep genrep;
    int length = 32;
    void *buf;
    pending_reply *pend = 0;
    struct event_list *event;

    /* Wait for there to be enough data for us to read a whole packet */
    if(c->in.queue_len < length)
        return 0;

    /* Get the response type, length, and sequence number. */
    memcpy(&genrep, c->in.queue, sizeof(genrep));

    /* Compute 32-bit sequence number of this packet. */
    if((genrep.response_type & 0x7f) != XCBKeymapNotify)
    {
        unsigned int lastread = c->in.request_read;
        c->in.request_read = (lastread & 0xffff0000) | genrep.sequence;
        if(XCB_SEQUENCE_COMPARE(c->in.request_read, <, lastread))
            c->in.request_read += 0x10000;
        if(XCB_SEQUENCE_COMPARE(c->in.request_read, >, c->in.request_expected))
            c->in.request_expected = c->in.request_read;

        if(c->in.request_read != lastread)
        {
            if(c->in.current_reply)
            {
                _xcb_map_put(c->in.replies, lastread, c->in.current_reply);
                c->in.current_reply = 0;
                c->in.current_reply_tail = &c->in.current_reply;
            }
            c->in.request_completed = c->in.request_read - 1;
        }
        if(genrep.response_type != XCBReply) /* error or event */
            c->in.request_completed = c->in.request_read; /* XXX: does event/error imply no more replies? */

        while(c->in.pending_replies && 
	      XCB_SEQUENCE_COMPARE (c->in.pending_replies->request, <=, c->in.request_completed))
        {
            pending_reply *oldpend = c->in.pending_replies;
            c->in.pending_replies = oldpend->next;
            if(!oldpend->next)
                c->in.pending_replies_tail = &c->in.pending_replies;
            free(oldpend);
        }
    }

    if(genrep.response_type == XCBError || genrep.response_type == XCBReply)
    {
        pend = c->in.pending_replies;
        if(pend && pend->request != c->in.request_read)
            pend = 0;
    }

    /* For reply packets, check that the entire packet is available. */
    if(genrep.response_type == XCBReply)
    {
        if(pend && pend->workaround == WORKAROUND_GLX_GET_FB_CONFIGS_BUG)
        {
            CARD32 *p = (CARD32 *) c->in.queue;
            genrep.length = p[2] * p[3] * 2;
        }
        length += genrep.length * 4;
    }

    buf = malloc(length + (genrep.response_type == XCBReply ? 0 : sizeof(CARD32)));
    if(!buf)
        return 0;
    if(_xcb_in_read_block(c, buf, length) <= 0)
    {
        free(buf);
        return 0;
    }
    if(pend && (pend->flags & XCB_REQUEST_DISCARD_REPLY))
    {
        free(buf);
        return 1;
    }

    if(genrep.response_type != XCBReply)
        ((XCBGenericEvent *) buf)->full_sequence = c->in.request_read;

    /* reply, or checked error */
    if( genrep.response_type == XCBReply ||
       (genrep.response_type == XCBError && pend && (pend->flags & XCB_REQUEST_CHECKED)))
    {
        reader_list *reader;
        struct reply_list *cur = malloc(sizeof(struct reply_list));
        if(!cur)
            return 0;
        cur->reply = buf;
        cur->next = 0;
        *c->in.current_reply_tail = cur;
        c->in.current_reply_tail = &cur->next;
        for(reader = c->in.readers; 
	    reader && 
	    XCB_SEQUENCE_COMPARE(reader->request, <=, c->in.request_read);
	    reader = reader->next)
	{
            if(reader->request == c->in.request_read)
            {
                pthread_cond_signal(reader->data);
                break;
            }
	}
        return 1;
    }

    /* event, or unchecked error */
    event = malloc(sizeof(struct event_list));
    if(!event)
    {
        free(buf);
        return 0;
    }
    event->event = buf;
    event->next = 0;
    *c->in.events_tail = event;
    c->in.events_tail = &event->next;
    pthread_cond_signal(&c->in.event_cond);
    return 1; /* I have something for you... */
}

static XCBGenericEvent *get_event(XCBConnection *c)
{
    struct event_list *cur = c->in.events;
    XCBGenericEvent *ret;
    if(!c->in.events)
        return 0;
    ret = cur->event;
    c->in.events = cur->next;
    if(!cur->next)
        c->in.events_tail = &c->in.events;
    free(cur);
    return ret;
}

static void free_reply_list(struct reply_list *head)
{
    while(head)
    {
        struct reply_list *cur = head;
        head = cur->next;
        free(cur->reply);
        free(cur);
    }
}

static int read_block(const int fd, void *buf, const size_t len)
{
    int done = 0;
    while(done < len)
    {
        int ret = read(fd, ((char *) buf) + done, len - done);
        if(ret > 0)
            done += ret;
        if(ret < 0 && errno == EAGAIN)
        {
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(fd, &fds);
	    do {
		ret = select(fd + 1, &fds, 0, 0, 0);
	    } while (ret == -1 && errno == EINTR);
        }
        if(ret <= 0)
            return ret;
    }
    return len;
}

static int poll_for_reply(XCBConnection *c, unsigned int request, void **reply, XCBGenericError **error)
{
    struct reply_list *head;

    /* If an error occurred when issuing the request, fail immediately. */
    if(!request)
        head = 0;
    /* We've read requests past the one we want, so if it has replies we have
     * them all and they're in the replies map. */
    else if(XCB_SEQUENCE_COMPARE(request, <, c->in.request_read))
    {
        head = _xcb_map_remove(c->in.replies, request);
        if(head && head->next)
            _xcb_map_put(c->in.replies, request, head->next);
    }
    /* We're currently processing the responses to the request we want, and we
     * have a reply ready to return. So just return it without blocking. */
    else if(request == c->in.request_read && c->in.current_reply)
    {
        head = c->in.current_reply;
        c->in.current_reply = head->next;
        if(!head->next)
            c->in.current_reply_tail = &c->in.current_reply;
    }
    /* We know this request can't have any more replies, and we've already
     * established it doesn't have a reply now. Don't bother blocking. */
    else if(request == c->in.request_completed)
        head = 0;
    /* We may have more replies on the way for this request: block until we're
     * sure. */
    else
        return 0;

    if(error)
        *error = 0;
    *reply = 0;

    if(head)
    {
        if(((XCBGenericRep *) head->reply)->response_type == XCBError)
        {
            if(error)
                *error = head->reply;
            else
                free(head->reply);
        }
        else
            *reply = head->reply;

        free(head);
    }

    return 1;
}

/* Public interface */

void *XCBWaitForReply(XCBConnection *c, unsigned int request, XCBGenericError **e)
{
    void *ret = 0;
    if(e)
        *e = 0;

    pthread_mutex_lock(&c->iolock);

    /* If this request has not been written yet, write it. */
    if(_xcb_out_flush_to(c, request))
    {
        pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
        reader_list reader;
        reader_list **prev_reader;

        for(prev_reader = &c->in.readers; 
	    *prev_reader && 
	    XCB_SEQUENCE_COMPARE ((*prev_reader)->request, <=, request);
	    prev_reader = &(*prev_reader)->next)
	{
            /* empty */;
	}
        reader.request = request;
        reader.data = &cond;
        reader.next = *prev_reader;
        *prev_reader = &reader;

        while(!poll_for_reply(c, request, &ret, e))
            if(!_xcb_conn_wait(c, &cond, 0, 0))
                break;

        for(prev_reader = &c->in.readers;
	    *prev_reader && 
	    XCB_SEQUENCE_COMPARE((*prev_reader)->request, <=, request);
	    prev_reader = &(*prev_reader)->next)
	{
            if(*prev_reader == &reader)
            {
                *prev_reader = (*prev_reader)->next;
                break;
            }
	}
        pthread_cond_destroy(&cond);
    }

    wake_up_next_reader(c);
    pthread_mutex_unlock(&c->iolock);
    return ret;
}

int XCBPollForReply(XCBConnection *c, unsigned int request, void **reply, XCBGenericError **error)
{
    int ret;
    assert(reply != 0);
    pthread_mutex_lock(&c->iolock);
    ret = poll_for_reply(c, request, reply, error);
    pthread_mutex_unlock(&c->iolock);
    return ret;
}

XCBGenericEvent *XCBWaitEvent(XCBConnection *c)
{
    return XCBWaitForEvent(c);
}

XCBGenericEvent *XCBWaitForEvent(XCBConnection *c)
{
    XCBGenericEvent *ret;
    pthread_mutex_lock(&c->iolock);
    /* get_event returns 0 on empty list. */
    while(!(ret = get_event(c)))
        if(!_xcb_conn_wait(c, &c->in.event_cond, 0, 0))
            break;

    wake_up_next_reader(c);
    pthread_mutex_unlock(&c->iolock);
    return ret;
}

XCBGenericEvent *XCBPollForEvent(XCBConnection *c, int *error)
{
    XCBGenericEvent *ret = 0;
    pthread_mutex_lock(&c->iolock);
    if(error)
        *error = 0;
    /* FIXME: follow X meets Z architecture changes. */
    if(_xcb_in_read(c))
        ret = get_event(c);
    else if(error)
        *error = -1;
    else
    {
        fprintf(stderr, "XCBPollForEvent: I/O error occured, but no handler provided.\n");
        abort();
    }
    pthread_mutex_unlock(&c->iolock);
    return ret;
}

XCBGenericError *XCBRequestCheck(XCBConnection *c, XCBVoidCookie cookie)
{
    /* FIXME: this could hold the lock to avoid syncing unnecessarily, but
     * that would require factoring the locking out of XCBGetInputFocus,
     * XCBGetInputFocusReply, and XCBWaitForReply. */
    XCBGenericError *ret;
    void *reply;
    if(XCB_SEQUENCE_COMPARE(cookie.request,>,c->in.request_expected)
       && XCB_SEQUENCE_COMPARE(cookie.request,>,c->in.request_completed))
    {
        free(XCBGetInputFocusReply(c, XCBGetInputFocus(c), &ret));
        assert(!ret);
    }
    reply = XCBWaitForReply(c, cookie.request, &ret);
    assert(!reply);
    return ret;
}

unsigned int XCBGetRequestRead(XCBConnection *c)
{
    unsigned int ret;
    pthread_mutex_lock(&c->iolock);
    /* FIXME: follow X meets Z architecture changes. */
    _xcb_in_read(c);
    ret = c->in.request_read;
    pthread_mutex_unlock(&c->iolock);
    return ret;
}

/* Private interface */

int _xcb_in_init(_xcb_in *in)
{
    if(pthread_cond_init(&in->event_cond, 0))
        return 0;
    in->reading = 0;

    in->queue_len = 0;

    in->request_read = 0;
    in->request_completed = 0;

    in->replies = _xcb_map_new();
    if(!in->replies)
        return 0;

    in->current_reply_tail = &in->current_reply;
    in->events_tail = &in->events;
    in->pending_replies_tail = &in->pending_replies;

    return 1;
}

void _xcb_in_destroy(_xcb_in *in)
{
    pthread_cond_destroy(&in->event_cond);
    free_reply_list(in->current_reply);
    _xcb_map_delete(in->replies, (void (*)(void *)) free_reply_list);
    while(in->events)
    {
        struct event_list *e = in->events;
        in->events = e->next;
        free(e->event);
        free(e);
    }
    while(in->pending_replies)
    {
        pending_reply *pend = in->pending_replies;
        in->pending_replies = pend->next;
        free(pend);
    }
}

int _xcb_in_expect_reply(XCBConnection *c, unsigned int request, enum workarounds workaround, int flags)
{
    pending_reply *pend = malloc(sizeof(pending_reply));
    assert(workaround != WORKAROUND_NONE || flags != 0);
    if(!pend)
        return 0;
    pend->request = request;
    pend->workaround = workaround;
    pend->flags = flags;
    pend->next = 0;
    *c->in.pending_replies_tail = pend;
    c->in.pending_replies_tail = &pend->next;
    return 1;
}

int _xcb_in_read(XCBConnection *c)
{
    int n = read(c->fd, c->in.queue + c->in.queue_len, sizeof(c->in.queue) - c->in.queue_len);
    if(n > 0)
        c->in.queue_len += n;
    while(read_packet(c))
        /* empty */;
    return (n > 0) || (n < 0 && errno == EAGAIN);
}

int _xcb_in_read_block(XCBConnection *c, void *buf, int len)
{
    int done = c->in.queue_len;
    if(len < done)
        done = len;

    memcpy(buf, c->in.queue, done);
    c->in.queue_len -= done;
    memmove(c->in.queue, c->in.queue + done, c->in.queue_len);

    if(len > done)
    {
        int ret = read_block(c->fd, (char *) buf + done, len - done);
        if(ret <= 0)
            return ret;
    }

    return len;
}
