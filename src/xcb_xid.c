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

/* XID allocators. */

#include <stdlib.h>
#include "xcb.h"
#include "xcbext.h"
#include "xcbint.h"
#include "extensions/xc_misc.h"

/* Public interface */

CARD32 XCBGenerateID(XCBConnection *c)
{
    CARD32 ret;
    pthread_mutex_lock(&c->xid.lock);
    if(c->xid.last == c->xid.max)
    {
	    XCBXCMiscGetXIDRangeRep *range;
	    range = XCBXCMiscGetXIDRangeReply(c, XCBXCMiscGetXIDRange(c), 0);
	    c->xid.last = range->start_id;
	    c->xid.max = range->start_id + (range->count - 1) * c->xid.inc;
	    free(range);
    }
    ret = c->xid.last | c->xid.base;
    c->xid.last += c->xid.inc;
    pthread_mutex_unlock(&c->xid.lock);
    return ret;
}

/* Private interface */

int _xcb_xid_init(XCBConnection *c)
{
    if(pthread_mutex_init(&c->xid.lock, 0))
        return 0;
    c->xid.last = 0;
    c->xid.base = c->setup->resource_id_base;
    c->xid.max = c->setup->resource_id_mask;
    c->xid.inc = c->setup->resource_id_mask & -(c->setup->resource_id_mask);
    return 1;
}

void _xcb_xid_destroy(XCBConnection *c)
{
    pthread_mutex_destroy(&c->xid.lock);
}
