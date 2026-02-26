#include "virtio_gpu.h"
#include "vm/kmalloc.h"

// gpu_ops_t virtio_gpu_ops = {
// 	.init 				= virtio_gpu_init,
// 	.shutdown 			= virtio_gpu_shutdown,
// 	.set_display_mode 	= virtio_gpu_set_display_mode,
// 	.get_display_mode 	= virtio_gpu_get_display_mode,
// 	.create_resource 	= virtio_gpu_create_resource,
// 	.destroy_resource 	= virtio_gpu_destroy_resource,
// 	.map_resource 		= virtio_gpu_map_resource,
// 	.unmap_resource 	= virtio_gpu_unmap_resource,
// 	.blit 				= virtio_gpu_blit,
// 	.present 			= virtio_gpu_present,
// 	.set_cursor 		= virtio_gpu_set_cursor,
// 	.move_cursor		= virtio_gpu_move_cursor,
// };

static int virtio_gpu_submit_command(gpu_device_t* gpu_dev, void* cmd, size_t cmd_size, void* resp, size_t resp_size) {
	// v_queue_t* tx_queue = virtio_get_tx_queue((pci_device_t*)gpu_dev->device_ctx);
	return 0;
}



gpu_device_t* virtio_gpu_create(pci_device_t* pci_dev) {
	// Allocate GPU device structure
	gpu_device_t* gpu_dev = (gpu_device_t*)kmalloc(sizeof(gpu_device_t));
	if (!gpu_dev) return NULL;

	gpu_dev->device_ctx = (void*)pci_dev;  // Store PCI device context
	// gpu_dev->ops = &virtio_gpu_ops;

	return gpu_dev;
}

gpu_result_t virtio_gpu_init(gpu_device_t* gpu_dev) {
	// Initialize VirtIO GPU device
	return GPU_OK;
}

gpu_result_t virtio_gpu_shutdown(gpu_device_t* gpu_dev) {
	// Shutdown VirtIO GPU device
	return GPU_OK;
}

gpu_result_t virtio_gpu_set_display_mode(gpu_device_t* gpu_dev, gpu_display_mode_t* mode) {
	// Set display mode on VirtIO GPU
	return GPU_OK;
}
