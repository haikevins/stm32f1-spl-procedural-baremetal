#include "byte_ring_buffer.h"

#include <stddef.h>

#include "compiler.h"

static uint16_t next_index(
    const byte_ring_buffer_t *ring,
    uint16_t index)
{
    ++index;

    if (index >= ring->storage_size)
    {
        index = 0U;
    }

    return index;
}

bool byte_ring_buffer_init(
    byte_ring_buffer_t *ring,
    uint8_t *storage,
    uint16_t storage_size)
{
    if ((ring == NULL) ||
        (storage == NULL) ||
        (storage_size < 2U))
    {
        return false;
    }

    ring->storage = storage;
    ring->storage_size = storage_size;
    ring->head = 0U;
    ring->tail = 0U;

    return true;
}

bool byte_ring_buffer_push(
    byte_ring_buffer_t *ring,
    uint8_t byte)
{
    uint16_t head;
    uint16_t next;

    if (ring == NULL)
    {
        return false;
    }

    head = ring->head;
    next = next_index(ring, head);

    if (next == ring->tail)
    {
        return false;
    }

    ring->storage[head] = byte;

    /*
     * Publish the stored byte before publishing the new head index.
     */
    COMPILER_MEMORY_BARRIER();
    ring->head = next;

    return true;
}

bool byte_ring_buffer_pop(
    byte_ring_buffer_t *ring,
    uint8_t *byte)
{
    uint16_t tail;

    if ((ring == NULL) || (byte == NULL))
    {
        return false;
    }

    tail = ring->tail;

    if (tail == ring->head)
    {
        return false;
    }

    /*
     * Observe the producer's head update before reading the stored byte.
     */
    COMPILER_MEMORY_BARRIER();
    *byte = ring->storage[tail];

    /*
     * Finish reading the slot before publishing it as free to the producer.
     */
    COMPILER_MEMORY_BARRIER();
    ring->tail = next_index(ring, tail);

    return true;
}

bool byte_ring_buffer_is_empty(
    const byte_ring_buffer_t *ring)
{
    if (ring == NULL)
    {
        return true;
    }

    return ring->head == ring->tail;
}

bool byte_ring_buffer_is_full(
    const byte_ring_buffer_t *ring)
{
    if (ring == NULL)
    {
        return true;
    }

    return next_index(ring, ring->head) == ring->tail;
}
