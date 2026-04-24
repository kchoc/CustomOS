#ifndef SYS_BUS_H
#define SYS_BUS_H

typedef enum bus_type { BUS_TYPE_NONE = 0, BUS_TYPE_PCI, BUS_TYPE_USB, BUS_TYPE_OTHER } bus_type_t;

struct device;
struct driver;

typedef struct bus {
    const char* name;

    int (*enumerate)(struct bus* bus);
    int (*match)(struct device* dev, struct driver* drv);
    int (*probe)(struct device* dev, struct driver* drv);
    int (*remove)(struct device* dev);
} bus_t;

#define DECLARE_BUS(name)                                                                          \
    extern bus_t bus_##name;                                                                       \
    int          bus_##name##_enumerate(bus_t* bus);                                               \
    int          bus_##name##_match(struct device* dev, struct driver* drv);                       \
    int          bus_##name##_probe(struct device* dev, struct driver* drv);                       \
    int          bus_##name##_remove(struct device* dev);

DECLARE_BUS(none);

#endif // SYS_BUS_H
