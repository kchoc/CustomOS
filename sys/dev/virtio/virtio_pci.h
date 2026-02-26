#ifndef VIRTIO_PCI_H
#define VIRTIO_PCI_H

#include "vqueue.h"
#include <inttypes.h>
#include <dev/pci/pci.h>

#define VIRTIO_PCI_STATUS_OFFSET       	0x12
#define VIRTIO_PCI_DEVICE_FEATURES		0x00
#define VIRTIO_PCI_DRIVER_FEATURES		0x04
#define VIRTIO_PCI_QUEUE_PFN			0x08
#define VIRTIO_PCI_QUEUE_SEL			0x0C
#define VIRTIO_PCI_QUEUE_NUM			0x0E

#define VIRTIO_STATUS_ACKNOWLEDGE		0x1
#define VIRTIO_STATUS_DRIVER			0x2
#define VIRTIO_STATUS_DRIVER_OK			0x4
#define VIRTIO_STATUS_FEATURES_OK		0x8
#define VIRTIO_STATUS_NEEDS_RESET		0x40
#define VIRTIO_STATUS_FAILED			0x80

void virtio_pci_set_status(pci_device_t* dev, uint8_t status);
void virtio_pci_reset_device(pci_device_t* dev);

vqueue_t* virtio_pci_alloc_queue(pci_device_t* dev, uint16_t queue_size);
int virtio_pci_setup_queue(pci_device_t* dev, vqueue_t* queue, uint16_t queue_index);
void virtio_pci_free_queue(vqueue_t* queue);

#endif // VIRTIO_PCI_H
