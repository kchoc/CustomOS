#include "virtio_pci.h"
#include "vm/vm_map.h"
#include "vqueue.h"
#include <dev/pci/pci.h>
#include <vm/kmalloc.h>
#include <vm/types.h>
#include <machine/pmap.h>
#include <string.h>

void virtio_pci_set_status(pci_device_t* dev, uint8_t status) {
	uint32_t st = pci_dev_config_read32(dev, VIRTIO_PCI_STATUS_OFFSET & ~3);
	uint8_t cur = (uint8_t)(st & 0xFF);
	cur |= status;
	st = (st & ~0xFF) | cur;
	pci_dev_config_write32(dev, VIRTIO_PCI_STATUS_OFFSET & ~3, st);
}

void virtio_pci_reset_device(pci_device_t* dev) {
	uint32_t st = pci_dev_config_read32(dev, VIRTIO_PCI_STATUS_OFFSET & ~3);
	st = (st & ~0xFF); // Clear status
	pci_dev_config_write32(dev, VIRTIO_PCI_STATUS_OFFSET & ~3, st);
}

vqueue_t* virtio_pci_alloc_vqueue(pci_device_t* dev, uint16_t queue_size) {
	if (queue_size == 0) return NULL;

	size_t desc_size = sizeof(vqueue_descriptor_t) * queue_size;
	size_t avail_size = sizeof(vqueue_avail_t) + sizeof(uint16_t) * queue_size;
	size_t used_size = sizeof(vqueue_used_t) + sizeof(vqueue_used_elem_t) * queue_size;
	// Align each section to 16 bytes
	size_t total_size = ((desc_size + 0x0F) & ~0x0F) + 
	                    ((avail_size + 0x0F) & ~0x0F) + 
	                    ((used_size + 0x0F) & ~0x0F);

	

	void* mem = kmalloc_aligned(total_size, 4096);
	if (!mem) return NULL;

	vqueue_t* queue = kmalloc(sizeof(vqueue_t));
	if (!queue) {
		kfree(mem);
		return NULL;
	}

	queue->qsize = queue_size;
	queue->vqueue_memory = mem;
	uint8_t* ptr = (uint8_t*)mem;
	queue->desc = (vqueue_descriptor_t*)mem;
	ptr += (desc_size + 0x0F) & ~0x0F;
	queue->avail = (vqueue_avail_t*)ptr;
	ptr += (avail_size + 0x0F) & ~0x0F;
	queue->used = (vqueue_used_t*)ptr;
	queue->last_used_idx = 0;

	return queue;
}

int virtio_pci_setup_queue(pci_device_t *dev, vqueue_t *queue, uint16_t queue_index) {
	// Select the queue
	pci_dev_config_write32(dev, VIRTIO_PCI_QUEUE_SEL & ~3, queue_index);

	// Check if the device supports this queue
	uint32_t qsize = pci_dev_config_read32(dev, VIRTIO_PCI_QUEUE_NUM & ~3);
	if (qsize == 0 || (qsize & 0xFFFF) < queue->qsize) {
		return -1; // Queue not supported
	}

	// Get PFN (address >> 12)
	paddr_t pfn = pmap_extract(kernel_vm_space->arch, (vaddr_t)queue->vqueue_memory) >> 12;
	pci_dev_config_write32(dev, VIRTIO_PCI_QUEUE_PFN & ~3, pfn);

	return 0;
}

void virtio_pci_free_vqueue(vqueue_t* queue) {
	if (!queue) return;
	kfree(queue->vqueue_memory);
	kfree(queue);
}

