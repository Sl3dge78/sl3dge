#pragma once

enum event_type {
    EVENT_TYPE_QUIT
};

typedef u32 EventType;

typedef struct EventQueue {
    EventType *queue;
    u32 size;
    u32 capacity;
} EventQueue;

void EventPush(EventQueue *event_queue, EventType new);
bool EventConsume(EventQueue *event_queue, EventType *result);
