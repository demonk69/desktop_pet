#include <assert.h>
#include <stdio.h>

#include "core/pet_event_queue.h"

int main(void)
{
    pet_event_queue_t queue;
    pet_event_t input;
    pet_event_t output;
    size_t index;

    pet_event_queue_init(&queue);
    assert(pet_event_queue_size(&queue) == 0U);
    for (index = 0U; index < PET_EVENT_QUEUE_CAPACITY; index++) {
        input = (pet_event_t){ .type = PET_EVENT_SYSTEM, .data.code = (int32_t)index };
        assert(pet_event_queue_push(&queue, &input));
    }
    assert(!pet_event_queue_push(&queue, &input));
    for (index = 0U; index < PET_EVENT_QUEUE_CAPACITY; index++) {
        assert(pet_event_queue_pop(&queue, &output));
        assert(output.data.code == (int32_t)index);
    }
    assert(!pet_event_queue_pop(&queue, &output));

    (void)printf("test_event_queue: ok\n");
    return 0;
}
