#include "stdio.h"
#include "string.h"
#include "syscalls.h"

int main(int argc, char* argv[])
{
    char dirname[256] = "/";

    if (argc == 2) {
        strncpy(dirname, argv[1], sizeof(dirname) - 1);
        dirname[sizeof(dirname) - 1] = '\0'; // Ensure null-termination
    }

    int fd = open(dirname, 0, 0);
    if (fd < 0) {
        printf("Failed to open directory: %s\n", dirname);
        exit(1);
    }

    char buf[256];
    int  res = getdirent(fd, buf, sizeof(buf), 0);
    if (res < 0) {
        printf("Failed to read directory entries\n");
        close(fd);
        exit(1);
    }

    size_t offset = 0;
    while (offset < (size_t)res) {
        char*  name     = buf + offset;
        size_t name_len = strlen(name);
        if (name_len == 0)
            break; // End of entries
        printf("  %s", name);
        offset += name_len + 1; // Move to next entry (null-terminated)
    }
    printf("\n");

    return 0;
}
