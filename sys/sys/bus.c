#include "bus.h"
#include "device.h"

bus_t bus_none = {
    .name      = "None Bus",
    .enumerate = bus_none_enumerate,
    .match     = bus_none_match,
    .probe     = bus_none_probe,
    .remove    = bus_none_remove,
};

int bus_none_enumerate(bus_t* bus)
{
    return 0; // No devices to enumerate
}

int bus_none_match(struct device* dev, struct driver* drv)
{
    return 1; // Always match since there are no devices
}

int bus_none_probe(struct device* dev, struct driver* drv)
{
    return drv->probe(dev);
}

int bus_none_remove(struct device* dev)
{
    return 0; // No devices to remove
}
