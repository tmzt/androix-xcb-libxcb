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

/* A cache for QueryExtension results. */

#include <stdlib.h>
#include <string.h>

#include "xcb.h"
#include "xcbext.h"
#include "xcbint.h"

typedef struct {
    enum { LAZY_COOKIE, LAZY_FORCED } tag;
    union {
        XCBQueryExtensionCookie cookie;
        XCBQueryExtensionRep *reply;
    } value;
} lazyreply;

static lazyreply *get_lazyreply(XCBConnection *c, XCBExtension *ext)
{
    static pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;
    static int next_global_id;

    lazyreply *data;

    pthread_mutex_lock(&global_lock);
    if(!ext->global_id)
        ext->global_id = ++next_global_id;
    pthread_mutex_unlock(&global_lock);

    data = _xcb_map_get(c->ext.extensions, ext->global_id);
    if(!data)
    {
        /* cache miss: query the server */
        data = malloc(sizeof(lazyreply));
        if(!data)
            return 0;
        data->tag = LAZY_COOKIE;
        data->value.cookie = XCBQueryExtension(c, strlen(ext->name), ext->name);
        _xcb_map_put(c->ext.extensions, ext->global_id, data);
    }
    return data;
}

static void free_lazyreply(void *p)
{
    lazyreply *data = p;
    if(data->tag == LAZY_FORCED)
        free(data->value.reply);
    free(data);
}

/* Public interface */

/* Do not free the returned XCBQueryExtensionRep - on return, it's aliased
 * from the cache. */
const XCBQueryExtensionRep *XCBGetExtensionData(XCBConnection *c, XCBExtension *ext)
{
    lazyreply *data;

    pthread_mutex_lock(&c->ext.lock);
    data = get_lazyreply(c, ext);
    if(data && data->tag == LAZY_COOKIE)
    {
        data->tag = LAZY_FORCED;
        data->value.reply = XCBQueryExtensionReply(c, data->value.cookie, 0);
    }
    pthread_mutex_unlock(&c->ext.lock);

    return data ? data->value.reply : 0;
}

void XCBPrefetchExtensionData(XCBConnection *c, XCBExtension *ext)
{
    pthread_mutex_lock(&c->ext.lock);
    get_lazyreply(c, ext);
    pthread_mutex_unlock(&c->ext.lock);
}

/* Private interface */

int _xcb_ext_init(XCBConnection *c)
{
    if(pthread_mutex_init(&c->ext.lock, 0))
        return 0;

    c->ext.extensions = _xcb_map_new();
    if(!c->ext.extensions)
        return 0;

    return 1;
}

void _xcb_ext_destroy(XCBConnection *c)
{
    pthread_mutex_destroy(&c->ext.lock);
    _xcb_map_delete(c->ext.extensions, free_lazyreply);
}
