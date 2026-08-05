#ifndef BYTE_RING_BUFFER_H
#define BYTE_RING_BUFFER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint8_t *storage;
    uint16_t storage_size;
    volatile uint16_t head;
    volatile uint16_t tail;
} byte_ring_buffer_t;

/*
 * The implementation reserves one slot to distinguish full from empty.
 * Therefore a storage array of N bytes can hold at most N - 1 bytes.
 *
 * Concurrency contract:
 * - exactly one producer calls push/is_full;
 * - exactly one consumer calls pop/is_empty.
 *
 * This matches UART RX (ISR producer, main consumer) and UART TX
 * (main producer, ISR consumer).
 */
bool byte_ring_buffer_init(
    byte_ring_buffer_t *ring,
    uint8_t *storage,
    uint16_t storage_size);

bool byte_ring_buffer_push(
    byte_ring_buffer_t *ring,
    uint8_t byte);

bool byte_ring_buffer_pop(
    byte_ring_buffer_t *ring,
    uint8_t *byte);

bool byte_ring_buffer_is_empty(
    const byte_ring_buffer_t *ring);

bool byte_ring_buffer_is_full(
    const byte_ring_buffer_t *ring);

#endif /* BYTE_RING_BUFFER_H */
