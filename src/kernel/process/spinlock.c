#include "kernel/process/spinlock.h"

static inline void spin_lock(spinlock_t *lock) {
	while (__sync_lock_test_and_set(lock, 1)) {
		while (*lock) asm volatile("pause");
	}
}

static inline void spin_unlock(spinlock_t *lock) {
	__sync_lock_release(lock);
}
