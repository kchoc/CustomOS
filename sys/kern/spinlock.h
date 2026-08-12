#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <inttypes.h>

#define WITH_SPINLOCK(lock)                                                                        \
    for (spinlock_t * _spinlock_cleanup_var                                                        \
             __attribute__((cleanup(_spinlock_cleanup))) = (spin_lock(&(lock)), &(lock)),          \
             *_spinlock_once                             = _spinlock_cleanup_var;                  \
         _spinlock_once; _spinlock_once                  = NULL)

#define SPINLOCK_INITIALIZER 0

typedef volatile uint32_t spinlock_t;

void spin_lock(spinlock_t* l);
void spin_unlock(spinlock_t* l);
int  spin_trylock(spinlock_t* l);
void _spinlock_cleanup(spinlock_t** lock);

#endif // SPINLOCK_H
