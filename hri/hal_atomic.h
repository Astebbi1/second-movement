/* Minimal hal_atomic.h shim for gossamer-based builds.
 * Replaces the ASF4 version; uses CMSIS intrinsics already provided by gossamer. */
#ifndef _HAL_ATOMIC_H_INCLUDED
#define _HAL_ATOMIC_H_INCLUDED

#include <stdint.h>

typedef uint32_t hal_atomic_t;

#define CRITICAL_SECTION_ENTER() \
    { \
        volatile hal_atomic_t __atomic = __get_PRIMASK(); \
        __disable_irq();

#define CRITICAL_SECTION_LEAVE() \
        __set_PRIMASK(__atomic); \
    }

#endif /* _HAL_ATOMIC_H_INCLUDED */
