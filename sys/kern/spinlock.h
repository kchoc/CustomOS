#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <inttypes.h>

#define WITH_SPINLOCK(lock)                                                                        \
    do {                                                                                           \
        spin_lock(&lock);                                                                          \
        __attribute__((cleanup(_spinlock_cleanup))) spinlock_t* _spinlock_cleanup_var = &lock;

#define END_WITH_SPINLOCK                                                                          \
    }                                                                                              \
    while (0)                                                                                      \
        ;

#define SPINLOCK_INITIALIZER 0

typedef volatile uint32_t spinlock_t;

void spin_lock(spinlock_t* l);
void spin_unlock(spinlock_t* l);
int  spin_trylock(spinlock_t* l);
void _spinlock_cleanup(spinlock_t** lock);

#endif // SPINLOCK_H
