#ifndef PET_EVENT_QUEUE_H
#define PET_EVENT_QUEUE_H

#include <stdbool.h>
#include <stddef.h>

#include "core/pet_event.h"

#define PET_EVENT_QUEUE_CAPACITY 16U

typedef struct {
    pet_event_t events[PET_EVENT_QUEUE_CAPACITY];
    size_t head;
    size_t tail;
    size_t count;
} pet_event_queue_t;

void pet_event_queue_init(pet_event_queue_t *queue);
bool pet_event_queue_push(pet_event_queue_t *queue, const pet_event_t *event);
bool pet_event_queue_pop(pet_event_queue_t *queue, pet_event_t *event);
size_t pet_event_queue_size(const pet_event_queue_t *queue);

#endif
