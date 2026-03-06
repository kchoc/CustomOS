#include "device.h"

#include <disk/mbr.h>
#include <dev/ata/ata.h>

// For now im doing to use a static list of drivers
driver_t drivers[] = {
	{
		.name = "PCI Mass Storage Driver",
		.vendor_id = 0x8086, // Intel
		.device_id = 0x7010, // PIIX4 IDE Controller
		.device_type = DEV_TYPE_BLOCK,
		.probe = ata_pci_probe_controller
	},
	{
		.name = "PCI ATA Driver",
		.vendor_id = 0x8086, // Intel
		.device_id = 0x2922, // 82801DB/DBM SATA Controller
		.device_type = DEV_TYPE_BLOCK,
		.probe = ata_pci_probe_controller
	}
};
static int num_drivers = sizeof(drivers) / sizeof(drivers[0]);

static int device_counter = 0;
static int device_id_counter = 0;

int device_register(device_t* dev) {
	for (int i = 0; i < num_drivers; ++i) {
		if (dev->bus->match(dev, &drivers[i])) {
			return dev->bus->probe(dev, &drivers[i]);
		}
	}
	return -1; // No matching driver found
}

int register_block_device(block_device_t* bdev) {
	return mbr_parse(bdev);
}
