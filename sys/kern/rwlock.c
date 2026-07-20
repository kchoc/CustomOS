#include "rwlock.h"
#include "process.h"

// For now rwlock is implemented using a spinlock to edit its state. This is not optimal but it
// works for now. In the future we can implement a more efficient rwlock using futexes or other
// mechanisms.

void rwlock_read_lock(rwlock_t* rw)
{
    for (;;) {

        spin_lock(&rw->interlock);

        if (!rw->writer && rw->waiting_writers == 0) {
            rw->readers++;
            spin_unlock(&rw->interlock);
            return;
        }

        rw->waiting_readers++;

        spin_unlock(&rw->interlock);

        yield();

        spin_lock(&rw->interlock);
        rw->waiting_readers--;
        spin_unlock(&rw->interlock);
    }
}

void rwlock_read_unlock(rwlock_t* rw)
{
    spin_lock(&rw->interlock);

    rw->readers--;

    spin_unlock(&rw->interlock);
}

void rwlock_write_lock(rwlock_t* rw)
{
    for (;;) {

        spin_lock(&rw->interlock);

        if (!rw->writer && rw->readers == 0) {

            rw->writer = true;

            spin_unlock(&rw->interlock);
            return;
        }

        rw->waiting_writers++;

        spin_unlock(&rw->interlock);

        yield();

        spin_lock(&rw->interlock);
        rw->waiting_writers--;
        spin_unlock(&rw->interlock);
    }
}

void rwlock_write_unlock(rwlock_t* rw)
{
    spin_lock(&rw->interlock);

    rw->writer = false;

    spin_unlock(&rw->interlock);
}

void _rwlock_read_cleanup(rwlock_t** lock)
{
    if (lock && *lock)
        rwlock_read_unlock(*lock);
}

void _rwlock_write_cleanup(rwlock_t** lock)
{
    if (lock && *lock)
        rwlock_write_unlock(*lock);
}
