#ifndef RWLOCK_H
#define RWLOCK_H

#include "spinlock.h"

#include <inttypes.h>
#include <stdbool.h>

/* LAYOUT:
 * 0-15:   Reader count
 * 16-30:  Writer waiting count
 * 31:     Writer active flag
 */

#define RWLOCK_READER_MASK   0x0000FFFF
#define RWLOCK_WRITER_MASK   0x7FFF0000
#define RWLOCK_WRITER_ACTIVE 0x80000000

#define RWLOCK_WRITER_SHIFT 16

typedef struct rwlock {
    spinlock_t interlock;

    uint32_t readers;
    uint32_t waiting_readers;
    uint32_t waiting_writers;

    bool writer;
} rwlock_t;

typedef void (*lock_func_t)(rwlock_t*);

#define RWLOCK_INITIALIZER                                                                         \
    (rwlock_t)                                                                                     \
    {                                                                                              \
        .interlock = SPINLOCK_INITIALIZER, .readers = 0, .waiting_readers = 0,                     \
        .waiting_writers = 0, .writer = false                                                      \
    }

#define WITH_READ_LOCK(lock)                                                                       \
    for (rwlock_t * _rwlock_read_cleanup_var __attribute__((cleanup(_rwlock_read_cleanup))) =      \
             (rwlock_read_lock(&(lock)), &(lock)),                                                 \
                                             *_rwlock_read_once = _rwlock_read_cleanup_var;        \
         _rwlock_read_once; _rwlock_read_once                   = NULL)

#define WITH_WRITE_LOCK(lock)                                                                      \
    for (rwlock_t * _rwlock_write_cleanup_var __attribute__((cleanup(_rwlock_write_cleanup))) =    \
             (rwlock_write_lock(&(lock)), &(lock)),                                                \
                                              *_rwlock_write_once = _rwlock_write_cleanup_var;     \
         _rwlock_write_once; _rwlock_write_once                   = NULL)

void rwlock_read_lock(rwlock_t* lock);
void rwlock_read_unlock(rwlock_t* lock);

void rwlock_write_lock(rwlock_t* lock);
void rwlock_write_unlock(rwlock_t* lock);

void _rwlock_read_cleanup(rwlock_t** lock);
void _rwlock_write_cleanup(rwlock_t** lock);

#endif // RWLOCK_H
