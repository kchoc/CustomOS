#include "types.h"

#include <kern/errno.h>

int op_disallowed(void* arg, ...)
{
    return -ENOSYS; // Operation not implemented
}
