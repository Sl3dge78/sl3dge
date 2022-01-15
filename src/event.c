#include "event.h"

internal void EventPush(EventQueue *event_queue, EventType new) {
    if(event_queue->size == event_queue->capacity) {
        u32 new_capacity = event_queue->capacity;
        void *new_queue;
        if(event_queue->capacity == 0) {
            new_capacity = 2;
            new_queue = sCalloc(new_capacity, sizeof(EventType));
        } else {
            new_capacity *= 2;
            new_queue = sRealloc(event_queue->queue, sizeof(EventType) * new_capacity);
        }
        if(new_queue) {
            event_queue->queue = new_queue;
            event_queue->capacity = new_capacity;
        } else {
            sError("Unable to resize the event queue");
            return;
        }
    }
    
    event_queue->queue[event_queue->size++] = new;
}

bool EventConsume(EventQueue *event_queue, EventType *result) {
    if(event_queue->size > 0) {
        *result = event_queue->queue[--event_queue->size];
        return true;
    } else
        return false;
}

