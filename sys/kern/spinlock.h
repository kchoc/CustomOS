#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <inttypes.h>

typedef volatile uint32_t spinlock_t;

void spin_lock(spinlock_t *l);
void spin_unlock(spinlock_t *l);
int spin_trylock(spinlock_t *l);

#endif // SPINLOCK_H
