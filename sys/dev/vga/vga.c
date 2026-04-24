#include "vga.h"

#include <fs/vfs.h>

#include <vm/kmalloc.h>
#include <vm/vm_map.h>

#include <kern/errno.h>
#include <kern/terminal.h>

device_ops_t vga_ops = {
    .read  = vga_read,
    .write = vga_write,
};

driver_t vga_driver = {
    .name        = "vga",
    .vendor_id   = 0x0000, // VGA doesn't have a specific vendor/device ID
    .device_id   = 0x0000,
    .device_type = DEV_TYPE_CHAR,
    .probe       = vga_probe,
};

console_t vga_console = {
    .putc  = vga_console_putc,
    .clear = vga_console_clear,
    .dev   = NULL, // Will be set when the VGA device is probed
};

int vga_init(void)
{
    int res = driver_register(&vga_driver);
    if (res)
        return res;

    // For simplicity, we directly create and register the VGA device here
    device_t* vga_dev;
    res = device_misc_create(&vga_driver, &vga_ops, &vga_dev);
    if (res)
        return res;

    printf("VGA device created successfully\n");

    res = device_register(vga_dev);
    if (res) {
        kfree(vga_dev);
        return res;
    }
    printf("VGA device initialized and registered successfully\n");
    res = vfs_register_device(vga_dev);
    if (res) {
        kfree(vga_dev);
        return res;
    }

    vga_console.dev = vga_dev; // Set the console's device to the VGA device

    return 0; // Successfully initialized the VGA device
}

int vga_probe(device_t* dev)
{
    // Initialize the VGA device data
    vga_device_data_t* vga_data = kmalloc(sizeof(vga_device_data_t));
    if (!vga_data)
        return -ENOMEM;

    // Set up the VGA framebuffer and dimensions (for simplicity, we use fixed values)
    vga_data->framebuffer =
        vm_map_device(0xB8000, 0x1000, VM_PROT_READ | VM_PROT_WRITE, VM_REG_F_NONE);
    vga_data->width  = 80; // VGA text mode width
    vga_data->height = 25; // VGA text mode height

    dev->ops_data    = vga_data;
    dev->driver      = &vga_driver;
    dev->driver_data = vga_data;

    return 0; // Successfully probed the VGA device
}

int vga_read(device_t* dev, uint64_t offset, uint32_t size, uint8_t* buffer)
{
    // VGA is a character device, so we can implement reading from the framebuffer
    vga_device_data_t* vga_data = (vga_device_data_t*)dev->ops_data;
    if (offset >= vga_data->width * vga_data->height * sizeof(uint16_t))
        return -EINVAL; // Invalid offset

    uint16_t* fb = vga_data->framebuffer;
    for (uint32_t i = 0; i < size / sizeof(uint16_t); i++) {
        ((uint16_t*)buffer)[i] = fb[(offset / sizeof(uint16_t)) + i];
    }
    return size; // Return the number of bytes read
}

int vga_write(device_t* dev, uint64_t offset, uint32_t size, const uint8_t* data)
{
    // VGA is a character device, so we can implement writing to the framebuffer
    vga_device_data_t* vga_data = (vga_device_data_t*)dev->ops_data;
    if (offset >= vga_data->width * vga_data->height * sizeof(uint16_t))
        return -EINVAL; // Invalid offset

    if (offset + size > vga_data->width * vga_data->height * sizeof(uint16_t))
        return -EINVAL; // Write exceeds framebuffer size

    uint16_t* fb = vga_data->framebuffer;
    for (uint32_t i = 0; i < size / sizeof(uint16_t); i++) {
        fb[(offset / sizeof(uint16_t)) + i] = ((uint16_t*)data)[i];
    }

    return size; // Return the number of bytes written
}

void vga_console_putc(console_t* con, char c)
{
    vga_device_data_t* vga = con->dev->driver_data;

    if (c == '\n') {
        for (size_t i = vga->x; i < vga->width; i++)
            vga->framebuffer[vga->y * vga->width + i] = (0x07 << 8) | ' ';
        vga->x = 0;
        vga->y++;
        return;
    }

    vga->framebuffer[vga->y * vga->width + vga->x] = (0x07 << 8) | c;

    vga->x++;
    if (vga->x >= vga->width) {
        vga->x = 0;
        vga->y++;
    }

    if (vga->y >= vga->height) {
        vga->y = 0;
    }
}

void vga_console_clear(console_t* con)
{
    vga_device_data_t* vga = con->dev->driver_data;

    for (size_t i = 0; i < vga->width * vga->height; i++)
        vga->framebuffer[i] = (0x07 << 8) | ' ';

    vga->x = vga->y = 0;
}
