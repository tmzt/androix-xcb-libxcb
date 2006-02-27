/*
 * Copyright (C) 2001-2004 Bart Massey and Jamey Sharp.
 * All Rights Reserved.
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

#ifndef __XCBINT_H
#define __XCBINT_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

enum workarounds {
    WORKAROUND_NONE,
    WORKAROUND_GLX_GET_FB_CONFIGS_BUG
};

#define XCB_PAD(i) (-(i) & 3)

/* xcb_list.c */

typedef struct _xcb_list _xcb_list;
typedef void (*XCBListFreeFunc)(void *);

_xcb_list *_xcb_list_new(void);
void _xcb_list_delete(_xcb_list *list, XCBListFreeFunc do_free);
int _xcb_list_insert(_xcb_list *list, void *data);
int _xcb_list_append(_xcb_list *list, void *data);
void *_xcb_list_peek_head(_xcb_list *list);
void *_xcb_list_remove_head(_xcb_list *list);
void *_xcb_list_remove(_xcb_list *list, int (*cmp)(const void *, const void *), const void *data);
void *_xcb_list_find(_xcb_list *list, int (*cmp)(const void *, const void *), const void *data);

typedef _xcb_list _xcb_map;

_xcb_map *_xcb_map_new(void);
void _xcb_map_delete(_xcb_map *q, XCBListFreeFunc do_free);
int _xcb_map_put(_xcb_map *q, unsigned int key, void *data);
void *_xcb_map_get(_xcb_map *q, unsigned int key);
void *_xcb_map_remove(_xcb_map *q, unsigned int key);


/* xcb_out.c */

typedef struct _xcb_out {
    pthread_cond_t cond;
    int writing;

    char queue[4096];
    int queue_len;
    struct iovec *vec;
    int vec_len;

    unsigned int request;
    unsigned int request_written;

    pthread_mutex_t reqlenlock;
    CARD32 maximum_request_length;
} _xcb_out;

int _xcb_out_init(_xcb_out *out);
void _xcb_out_destroy(_xcb_out *out);

int _xcb_out_write(XCBConnection *c);
int _xcb_out_write_block(XCBConnection *c, struct iovec *vector, size_t count);
int _xcb_out_flush(XCBConnection *c);


/* xcb_in.c */

typedef struct _xcb_in {
    pthread_cond_t event_cond;
    int reading;

    char queue[4096];
    int queue_len;

    unsigned int request_read;
    struct reply_list *current_reply;
    struct reply_list **current_reply_tail;

    _xcb_map *replies;
    struct event_list *events;
    struct event_list **events_tail;
    _xcb_list *readers;

    struct pending_reply *pending_replies;
    struct pending_reply **pending_replies_tail;
} _xcb_in;

int _xcb_in_init(_xcb_in *in);
void _xcb_in_destroy(_xcb_in *in);

int _xcb_in_expect_reply(XCBConnection *c, unsigned int request, enum workarounds workaround, int flags);

int _xcb_in_read(XCBConnection *c);
int _xcb_in_read_block(XCBConnection *c, void *buf, int nread);


/* xcb_xid.c */

typedef struct _xcb_xid {
    pthread_mutex_t lock;
    CARD32 last;
    CARD32 base;
    CARD32 max;
    CARD32 inc;
} _xcb_xid;

int _xcb_xid_init(XCBConnection *c);
void _xcb_xid_destroy(XCBConnection *c);


/* xcb_ext.c */

typedef struct _xcb_ext {
    pthread_mutex_t lock;
    struct lazyreply *extensions;
    int extensions_size;
} _xcb_ext;

int _xcb_ext_init(XCBConnection *c);
void _xcb_ext_destroy(XCBConnection *c);


/* xcb_conn.c */

struct XCBConnection {
    /* constant data */
    XCBConnSetupSuccessRep *setup;
    int fd;

    /* I/O data */
    pthread_mutex_t iolock;
    _xcb_in in;
    _xcb_out out;

    /* misc data */
    _xcb_ext ext;
    _xcb_xid xid;
};

int _xcb_conn_wait(XCBConnection *c, const int should_write, pthread_cond_t *cond);
#endif
