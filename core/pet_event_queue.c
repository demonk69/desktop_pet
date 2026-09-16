#include "core/pet_event_queue.h"

void pet_event_queue_init(pet_event_queue_t *queue)
{
    if (queue == NULL) {
        return;
    }
    queue->head = 0U;
    queue->tail = 0U;
    queue->count = 0U;
}

bool pet_event_queue_push(pet_event_queue_t *queue, const pet_event_t *event)
{
    if (queue == NULL || event == NULL || queue->count >= PET_EVENT_QUEUE_CAPACITY) {
        return false;
    }
    queue->events[queue->tail] = *event;
    queue->tail = (queue->tail + 1U) % PET_EVENT_QUEUE_CAPACITY;
    queue->count++;
    return true;
}

bool pet_event_queue_pop(pet_event_queue_t *queue, pet_event_t *event)
{
    if (queue == NULL || event == NULL || queue->count == 0U) {
        return false;
    }
    *event = queue->events[queue->head];
    queue->head = (queue->head + 1U) % PET_EVENT_QUEUE_CAPACITY;
    queue->count--;
    return true;
}

size_t pet_event_queue_size(const pet_event_queue_t *queue)
{
    return queue == NULL ? 0U : queue->count;
}
