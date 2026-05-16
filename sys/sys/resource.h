#ifndef SYS_RESOURCE_H
#define SYS_RESOURCE_H

#include <inttypes.h>
#include <stddef.h>

typedef enum resource_type {
    RES_TYPE_IOPORT = 0,
    RES_TYPE_MMIO,
    RES_TYPE_IRQ,
    RES_TYPE_DMA
} resource_type_t;

typedef struct resource {
    resource_type_t type;
    uintptr_t       start;
    size_t          size;
    uint32_t        flags; // Flags specific to the resource type (e.g., read/write permissions for MMIO)

    void*           mapped_addr; // For MMIO resources, this will point to the mapped address
} resource_t;

#endif // SYS_RESOURCE_H
