#include "driver.h"
#include "bus.h"

list_t driver_lists[] = {
    [bus_type_any]   = LIST_INIT,
    [bus_type_root] = LIST_INIT,
    [bus_type_pci]  = LIST_INIT,
    [bus_type_usb]  = LIST_INIT,
    [bus_type_virtio] = LIST_INIT,
};

void drivers_init()
{
    for (driver_t* drv = __drivers_start; drv < __drivers_end; drv++) {
        list_push_head(&driver_lists[drv->bus_type], &drv->bus_node); 
    }
}


