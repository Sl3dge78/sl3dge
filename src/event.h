#pragma once

#include <sl3dge-utils/sl3dge.h>

enum event_type {
    EVENT_TYPE_QUIT,
    EVENT_TYPE_RESTART,
    EVENT_TYPE_RELOADSHADERS,
};

typedef u32 EventType;

typedef struct EventQueue {
    EventType *queue;
    u32 size;
    u32 capacity;
} EventQueue;

void EventPush(EventQueue *event_queue, EventType new);
bool EventConsume(EventQueue *event_queue, EventType *result);
