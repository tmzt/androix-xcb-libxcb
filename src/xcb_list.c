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

/* A generic implementation of a list of void-pointers. */

#include <stdlib.h>

#include "xcb.h"
#include "xcbint.h"

typedef struct node {
    struct node *next;
    void *data;
} node;

struct _xcb_list {
    node *head;
    node **tail;
};

/* Private interface */

_xcb_list *_xcb_list_new()
{
    _xcb_list *list;
    list = malloc(sizeof(_xcb_list));
    if(!list)
        return 0;
    list->head = 0;
    list->tail = &list->head;
    return list;
}

static void _xcb_list_clear(_xcb_list *list, XCBListFreeFunc do_free)
{
    void *tmp;
    while((tmp = _xcb_list_remove_head(list)))
        if(do_free)
            do_free(tmp);
}

void _xcb_list_delete(_xcb_list *list, XCBListFreeFunc do_free)
{
    if(!list)
        return;
    _xcb_list_clear(list, do_free);
    free(list);
}

int _xcb_list_insert(_xcb_list *list, void *data)
{
    node *cur;
    cur = malloc(sizeof(node));
    if(!cur)
        return 0;
    cur->data = data;

    cur->next = list->head;
    list->head = cur;
    return 1;
}

int _xcb_list_append(_xcb_list *list, void *data)
{
    node *cur;
    cur = malloc(sizeof(node));
    if(!cur)
        return 0;
    cur->data = data;
    cur->next = 0;

    *list->tail = cur;
    list->tail = &cur->next;
    return 1;
}

void *_xcb_list_peek_head(_xcb_list *list)
{
    if(!list->head)
        return 0;
    return list->head->data;
}

void *_xcb_list_remove_head(_xcb_list *list)
{
    void *ret;
    node *tmp = list->head;
    if(!tmp)
        return 0;
    ret = tmp->data;
    list->head = tmp->next;
    if(!list->head)
        list->tail = &list->head;
    free(tmp);
    return ret;
}

void *_xcb_list_remove(_xcb_list *list, int (*cmp)(const void *, const void *), const void *data)
{
    node **cur;
    for(cur = &list->head; *cur; cur = &(*cur)->next)
        if(cmp(data, (*cur)->data))
        {
            node *tmp = *cur;
            void *ret = (*cur)->data;
            *cur = (*cur)->next;
            if(!*cur)
                list->tail = cur;

            free(tmp);
            return ret;
        }
    return 0;
}

void *_xcb_list_find(_xcb_list *list, int (*cmp)(const void *, const void *), const void *data)
{
    node *cur;
    for(cur = list->head; cur; cur = cur->next)
        if(cmp(data, cur->data))
            return cur->data;
    return 0;
}

_xcb_queue *_xcb_queue_new(void) __attribute__ ((alias ("_xcb_list_new")));
void _xcb_queue_delete(_xcb_queue *q, XCBListFreeFunc do_free) __attribute__ ((alias ("_xcb_list_delete")));
int _xcb_queue_enqueue(_xcb_queue *q, void *data) __attribute__ ((alias ("_xcb_list_append")));
void *_xcb_queue_dequeue(_xcb_queue *q) __attribute__ ((alias ("_xcb_list_remove_head")));

int _xcb_queue_is_empty(_xcb_queue *q)
{
    return q->head == 0;
}

typedef struct {
    unsigned int key;
    void *value;
} map_pair;

_xcb_map *_xcb_map_new(void) __attribute__ ((alias ("_xcb_list_new")));

void _xcb_map_delete(_xcb_map *q, XCBListFreeFunc do_free)
{
    map_pair *tmp;
    if(!q)
        return;
    while((tmp = _xcb_list_remove_head(q)))
    {
        if(do_free)
            do_free(tmp->value);
        free(tmp);
    }
    free(q);
}

int _xcb_map_put(_xcb_map *q, unsigned int key, void *data)
{
    map_pair *cur = malloc(sizeof(map_pair));
    if(!cur)
        return 0;
    cur->key = key;
    cur->value = data;
    if(!_xcb_list_append(q, cur))
    {
        free(cur);
        return 0;
    }
    return 1;
}

static int match_map_pair(const void *key, const void *pair)
{
    return ((map_pair *) pair)->key == *(unsigned int *) key;
}

void *_xcb_map_get(_xcb_map *q, unsigned int key)
{
    map_pair *cur = _xcb_list_find(q, match_map_pair, &key);
    if(!cur)
        return 0;
    return cur->value;
}

void *_xcb_map_remove(_xcb_map *q, unsigned int key)
{
    map_pair *cur = _xcb_list_remove(q, match_map_pair, &key);
    void *ret;
    if(!cur)
        return 0;
    ret = cur->value;
    free(cur);
    return ret;
}
