#include "filesystem.h"
#include "mount.h"

file_system_type_t vfat_fs_type = {.name = "vfat", .fs_ops = &vfat_mount_ops};
