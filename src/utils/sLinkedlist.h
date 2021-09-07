#ifndef SLINKEDLIST_H
#define SLINKEDLIST_H

#include <string.h>

#include "sTypes.h"
#include "sDebug.h"

#define ARRAY_INIT_SIZE 10

struct u32Vector {
    u32 *buffer;
    u32 count;
    u32 capacity;
};

void u32vector_init(u32Vector *v, u32 init_size) {
    v->buffer = (u32 *)sMalloc(sizeof(u32) * init_size);
    v->count = 0;
    v->capacity = init_size;
}

u32 u32vector_count(u32Vector *v) {
    return v->count;
}

void u32vector_destroy(u32Vector *v) {
    sfree(v->buffer);
    v->count = 0;
    v->capacity = 0;
}

internal void u32vector_resize(u32Vector *v, u32 new_capacity) {
    u32 *new_buffer = (u32 *)srealloc(v->buffer, new_capacity * sizeof(void *));
    if(new_buffer) {
        v->buffer = new_buffer;
        v->capacity = new_capacity;
    }
}

void u32vector_append(u32Vector *v, u32 item) {
    if(v->capacity == v->count) {
        u32vector_resize(v, v->capacity * 2);
    }
    v->buffer[v->count] = item;
    v->count++;
}

// @TODO : Test
struct ListItem;

struct List {
    ListItem *head;
    ListItem *end;
    u32 size;
};

struct ListItem {
    void *data;
    ListItem *next;
    ListItem *prev;
};

internal void lldeleteitem(ListItem *item) {
    //free(item->data);
    sfree(item);
}

void llpushback(List *ll, void *object, size_t size) {
    ListItem *item = (ListItem *)sMalloc(sizeof(ListItem));
    //item->data = malloc(size);
    //memcpy(ll->head, object, size);
    item->data = object;
    if(ll->end) {
        item->prev = ll->end;
    }

    if(!ll->head) {
        ll->head = item;
    } else {
        ll->end->next = item;
    }
    ll->end = item;
    ll->size++;
}

void llpopback(List *ll) {
    if(ll->head) {
        ListItem *last = ll->end->prev;
        lldeleteitem(ll->end);

        ll->size--;
        if(last) {
            ll->end = last;
        }
    }
}

void llpopfront(List *ll) {
    if(ll->head) {
        ListItem *first = ll->head->next;
        lldeleteitem(ll->head);
        ll->size--;
        if(first) {
            ll->head = first;
        }
    }
}

void llclear(List *ll) {
    ListItem *i = ll->head;
    while(i != NULL) {
        ListItem *n = i->next;
        lldeleteitem(i);
        i = n;
    }
}

#endif