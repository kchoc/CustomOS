#ifndef virtIO_GPU_H
#define virtIO_GPU_H

#include <dev/pci/pci.h>
#include <kern/compiler.h>

#include <inttypes.h>
#include <stddef.h>

#define VIRTIO_PCI_VENDOR_ID 0x1AF4
#define VIRTIO_PCI_DEVICE_ID_GPU 0x1050

#define VIRTIO_GPU_CMD_RESOURCE_CREATE_2D			0x0100
#define VIRTIO_GPU_CMD_RESOURCE_UNREF 				0x0101
#define VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING 		0x0102
#define VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D 			0x0103
#define VIRTIO_GPU_CMD_RESOURCE_COPY 				0x0104
#define VIRTIO_GPU_CMD_RESOURCE_FLUSH 				0x0105
#define VIRTIO_GPU_CMD_SET_SCANOUT 					0x0200

#define MAX_VIRTIO_GPU_RESOURCES 256

/* =====================
 * VirtIO GPU Structures
 * ===================== */

typedef struct virtio_gpu_ctrl_hdr {
	uint32_t type;
	uint32_t flags;
	uint64_t fence_id;
	uint32_t context_id;
	uint32_t padding; // Align to 16 bytes	
} virtio_gpu_ctrl_hdr_t;

typedef struct virtio_gpu_resource_create_2d {
	virtio_gpu_ctrl_hdr_t hdr;
	uint32_t resource_id;
	uint32_t width;
	uint32_t height;
	uint32_t format;
} virtio_gpu_resource_create_2d_t;

typedef struct virtio_gpu_resource_attach_backing {
	virtio_gpu_ctrl_hdr_t hdr;
	uint32_t resource_id;
	uint32_t num_entries;
	// Followed by array of entries
	// { uint64_t addr; uint32_t length; uint32_t padding; }
} virtio_gpu_resource_attach_backing_t;

typedef struct virtio_gpu_set_scanout {
	virtio_gpu_ctrl_hdr_t hdr;
	uint32_t scanout_id;
	uint32_t resource_id;
	uint32_t width;
	uint32_t height;
	uint32_t x;
	uint32_t y;
} virtio_gpu_set_scanout_t;

typedef struct virtio_gpu_transfer_to_host_2d {
	virtio_gpu_ctrl_hdr_t hdr;
	uint32_t resource_id;
	uint32_t x;
	uint32_t y;
	uint32_t width;
	uint32_t height;
} virtio_gpu_transfer_to_host_2d_t;

typedef struct virtio_gpu_resource_flush {
	virtio_gpu_ctrl_hdr_t hdr;
	uint32_t resource_id;
	uint32_t x;
	uint32_t y;
	uint32_t width;
	uint32_t height;
} virtio_gpu_resource_unref_t;

typedef struct gpu_res {
	uint32_t id;
	uint32_t width;
	uint32_t height;
	uint32_t format;
	size_t size;
	void* host_ptr; // Pointer to host memory if mapped
} gpu_res_t;

typedef enum {
	GPU_OK = 0,
	GPU_ERROR = -1,
	GPU_NOT_SUPPORTED = -2,
	GPU_INVALID_PARAM = -3,
} gpu_result_t;

typedef struct gpu_display_mode {
	uint32_t width;
	uint32_t height;
	uint32_t refresh_rate; // in Hz
} gpu_display_mode_t;

typedef struct gpu_device {
	void* device_ctx; // PCI device or VirtIO context
	// gpu_ops_t* ops;   // GPU operations
} gpu_device_t;



gpu_device_t *virtio_gpu_create(pci_device_t *pci_dev);

gpu_result_t virtio_gpu_init(gpu_device_t *gpu_dev);
gpu_result_t virtio_gpu_shutdown(gpu_device_t *gpu_dev);
gpu_result_t virtio_gpu_set_display_mode(gpu_device_t *gpu_dev, gpu_display_mode_t* mode);
gpu_result_t virtio_gpu_get_display_mode(gpu_device_t *gpu_dev, gpu_display_mode_t* mode);
gpu_result_t virtio_gpu_create_resource(gpu_device_t *gpu_dev, gpu_res_t* res);
gpu_result_t virtio_gpu_destroy_resource(gpu_device_t *gpu_dev, gpu_res_t* res);
gpu_result_t virtio_gpu_map_resource(gpu_device_t *gpu_dev, gpu_res_t* res, void** data_ptr);
gpu_result_t virtio_gpu_unmap_resource(gpu_device_t *gpu_dev, gpu_res_t* res);
gpu_result_t virtio_gpu_blit(gpu_device_t *gpu_dev, gpu_res_t* src, gpu_res_t* dst, uint32_t src_x, uint32_t src_y, uint32_t dst_x, uint32_t dst_y, uint32_t width, uint32_t height);
gpu_result_t virtio_gpu_present(gpu_device_t *gpu_dev, gpu_res_t* res);
gpu_result_t virtio_gpu_set_cursor(gpu_device_t *gpu_dev, gpu_res_t* cursor_res, int32_t x, int32_t y);
gpu_result_t virtio_gpu_move_cursor(gpu_device_t *gpu_dev, int32_t x, int32_t y);

#endif // virtIO_GPU_H
