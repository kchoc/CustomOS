#ifndef VIRTIO_VQUEUE_H
#define VIRTIO_VQUEUE_H

#include <inttypes.h>
#include <kern/compiler.h>

#define VIRTIO_DESCRIPTOR_F_NEXT       1
#define VIRTIO_DESCRIPTOR_F_WRITE      2

typedef struct vqueue_descriptor {
	uint64_t addr;
	uint32_t len;
	uint16_t flags;
	uint16_t next;
} __packed vqueue_descriptor_t;

typedef struct vqueue_avail {
	uint16_t flags;
	uint16_t idx;
	uint16_t ring[];  // size = queue_size
	// uint16_t used_event; // Only if VIRTIO_F_EVENT_IDX
} __packed vqueue_avail_t;

typedef struct vqueue_used_elem {
	uint32_t id;
	uint32_t len;
} __packed vqueue_used_elem_t;

typedef struct vqueue_used {
	uint16_t flags;
	uint16_t idx;
	vqueue_used_elem_t ring[]; // size = queue_size
	// uint16_t avail_event; // Only if VIRTIO_F_EVENT_IDX
} __packed vqueue_used_t;

typedef struct vqueue {
	uint16_t qsize;
	
	vqueue_descriptor_t* desc;
	vqueue_avail_t* avail;
	vqueue_used_t* used;

	void* vqueue_memory;
	uint16_t last_used_idx;
} vqueue_t;

#endif // VIRTIO_VQUEUE_H