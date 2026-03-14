#include "spinlock.h"

/*
 * Acquire a spinlock
 */
void spin_lock(spinlock_t *l) {
    uint32_t expected;

    for (;;) {
        expected = 0;
        asm volatile (
            "lock cmpxchg %1, %2"
            : "=a"(expected)
            : "r"(1U), "m"(*l), "a"(expected)
            : "memory"
        );
        if (expected == 0)
            return;     // acquired
        asm volatile("pause");  // reduce contention
    }
}

/*
 * Release a spinlock
 */
void spin_unlock(spinlock_t *l) {
    asm volatile("" ::: "memory");
    *l = 0;
}

/*
 * Try to acquire (non-blocking)
 * Returns 0 on success, non-zero if lock is already held
 */
int spin_trylock(spinlock_t *l) {
    uint32_t expected = 0;
    asm volatile (
        "lock cmpxchg %1, %2"
        : "=a"(expected)
        : "r"(1U), "m"(*l), "a"(expected)
        : "memory"
    );
    return expected; // 0 if acquired, non-zero if already held
}
