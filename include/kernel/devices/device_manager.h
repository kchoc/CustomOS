#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include "types/list.h"
#define MAX_DEVICES 128

typedef struct device device_t;

void discover_devices(void);

device_t* get_device_by_name(const char* name);
device_t* get_device_by_id(int dev_id);

#endif // DEVICE_MANAGER_H
