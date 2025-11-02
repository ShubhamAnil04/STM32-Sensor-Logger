#include "buffer.h"
#include "cmn_1.h"
#include <string.h>     
#include <stdint.h>
#include <stdbool.h>

static SensorData buffer[BUFFER_SIZE];
static volatile uint32_t head = 0; /* next write index (modified by producer/ISR) */
static volatile uint32_t tail = 0; /* next read index  (modified by consumer/main) */

/* helper: compute next index (wrap-around) */
static inline uint32_t next_idx(uint32_t i)
{
    uint32_t nxt = i + 1u;
    if (nxt >= BUFFER_SIZE) nxt = 0u;
    return nxt;
}

void buffer_init(void)
{
    /* clear indices and memory */
    disable_irq(); /* protect init in case called while interrupts enabled */
    head = 0u;
    tail = 0u;
    memset((void*)buffer, 0, sizeof(buffer));
    enable_irq();
}

bool buffer_is_empty(void)
{
    /* safe to read both volatile indices without lock for single-producer/single-consumer */
    return head == tail;
}

bool buffer_is_full(void)
{
    uint32_t nxt = next_idx(head);
    return nxt == tail;
}

/* push data into buffer
   - intended to be called from ISR (producer)
   - does not disable interrupts
   - returns false if buffer was full (data dropped)
*/
bool buffer_push(SensorData data)
{
    uint32_t nxt = next_idx(head);
    if (nxt == tail) {
        /* full - drop sample */
        return false;
    }
    buffer[head] = data;
    /* publish new head */
    head = nxt;
    return true;
}
bool buffer_pop(SensorData *data)
{
    /* short critical section while we check head and update tail */
    disable_irq();
    if (head == tail) {
        /* empty */
        enable_irq();
        return false;
    }
    *data = buffer[tail];
    tail = next_idx(tail);
    enable_irq();
    return true;
}
