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
#include <errno.h>

#include "xcb.h"
#include "xcbext.h"
#include "xcbint.h"

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

typedef struct XCBReplyData {
    unsigned int request;
    pthread_cond_t *data;
} XCBReplyData;

static int match_reply(const void *request, const void *data)
{
    return ((XCBReplyData *) data)->request == *(unsigned int *) request;
}

static void wake_up_next_reader(XCBConnection *c)
{
    XCBReplyData *cur = _xcb_list_peek_head(c->in.readers);
    int pthreadret;
    if(cur)
        pthreadret = pthread_cond_signal(cur->data);
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
    if((genrep.response_type & 0x7f) != KeymapNotify)
    {
        int lastread = c->in.request_read;
        c->in.request_read = (lastread & 0xffff0000) | genrep.sequence;
        if(c->in.request_read != lastread)
        {
            pending_reply *oldpend = c->in.pending_replies;
            if(oldpend && oldpend->request == lastread)
            {
                c->in.pending_replies = oldpend->next;
                if(!oldpend->next)
                    c->in.pending_replies_tail = &c->in.pending_replies;
                free(oldpend);
            }
            if(c->in.current_reply)
            {
                _xcb_map_put(c->in.replies, lastread, c->in.current_reply);
                c->in.current_reply = 0;
                c->in.current_reply_tail = &c->in.current_reply;
            }
        }
        if(c->in.request_read < lastread)
            c->in.request_read += 0x10000;
    }

    if(genrep.response_type == 0 || genrep.response_type == 1)
    {
        pend = c->in.pending_replies;
        if(pend && pend->request != c->in.request_read)
            pend = 0;
    }

    /* For reply packets, check that the entire packet is available. */
    if(genrep.response_type == 1)
    {
        if(pend && pend->workaround == WORKAROUND_GLX_GET_FB_CONFIGS_BUG)
        {
            CARD32 *p = (CARD32 *) c->in.queue;
            genrep.length = p[2] * p[3] * 2;
        }
        length += genrep.length * 4;
    }

    buf = malloc(length);
    if(!buf)
        return 0;
    if(_xcb_in_read_block(c, buf, length) <= 0)
    {
        free(buf);
        return 0;
    }

    /* reply, or checked error */
    if(genrep.response_type == 1 || (genrep.response_type == 0 && pend && (pend->flags & XCB_REQUEST_CHECKED)))
    {
        XCBReplyData *reader = _xcb_list_find(c->in.readers, match_reply, &c->in.request_read);
        struct reply_list *cur = malloc(sizeof(struct reply_list));
        if(!cur)
            return 0;
        cur->reply = buf;
        cur->next = 0;
        *c->in.current_reply_tail = cur;
        c->in.current_reply_tail = &cur->next;
        if(reader)
            pthread_cond_signal(reader->data);
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
            ret = select(fd + 1, &fds, 0, 0, 0);
        }
        if(ret <= 0)
            return ret;
    }
    return len;
}

/* Public interface */

void *XCBWaitForReply(XCBConnection *c, unsigned int request, XCBGenericError **e)
{
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    XCBReplyData reader;
    struct reply_list *head;
    void *ret = 0;
    if(e)
        *e = 0;

    pthread_mutex_lock(&c->iolock);

    /* If this request has not been written yet, write it. */
    if((signed int) (c->out.request_written - request) < 0)
        if(!_xcb_out_flush(c))
            goto done; /* error */

    if(_xcb_list_find(c->in.readers, match_reply, &request))
        goto done; /* error */

    reader.request = request;
    reader.data = &cond;
    _xcb_list_append(c->in.readers, &reader);

    /* If this request has not been read yet, wait for it. */
    while(((signed int) (c->in.request_read - request) < 0 ||
            (c->in.request_read == request && !c->in.current_reply)))
        if(!_xcb_conn_wait(c, /*should_write*/ 0, &cond))
            goto done;

    if(c->in.request_read != request)
    {
        head = _xcb_map_remove(c->in.replies, request);
        if(head && head->next)
            _xcb_map_put(c->in.replies, request, head->next);
    }
    else
    {
        head = c->in.current_reply;
        if(head)
        {
            c->in.current_reply = head->next;
            if(!head->next)
                c->in.current_reply_tail = &c->in.current_reply;
        }
    }

    if(head)
    {
        ret = head->reply;
        free(head);

        if(((XCBGenericRep *) ret)->response_type == 0) /* X error */
        {
            if(e)
                *e = ret;
            else
                free(ret);
            ret = 0;
        }
    }

done:
    _xcb_list_remove(c->in.readers, match_reply, &request);
    pthread_cond_destroy(&cond);

    wake_up_next_reader(c);
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
        if(!_xcb_conn_wait(c, /*should_write*/ 0, &c->in.event_cond))
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

    in->replies = _xcb_map_new();
    in->readers = _xcb_list_new();
    if(!in->replies || !in->readers)
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
    _xcb_list_delete(in->readers, 0);
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
    if(workaround != WORKAROUND_NONE || flags != 0)
    {
        pending_reply *pend = malloc(sizeof(pending_reply));
        if(!pend)
            return 0;
        pend->request = request;
        pend->workaround = workaround;
        pend->flags = flags;
        pend->next = 0;
        *c->in.pending_replies_tail = pend;
        c->in.pending_replies_tail = &pend->next;
    }
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
