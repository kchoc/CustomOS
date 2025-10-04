#include "kernel/filesystem/path.h"
#include "types/string.h"

int format_filename_83(const char* input, char out[11]) {
    if (!input) return -1;

    memset(out, ' ', 11);
    const char* dot = strchr(input, '.');

    uint8_t name_len;
    uint8_t ext_len = 0;

    // If it is a file
    if (dot) {
        name_len = (dot - input);
        ext_len = strlen(dot + 1);

        if (name_len > 8) name_len = 8;
        if (ext_len > 3) ext_len = 3;
    } else {
        name_len = strlen(input);
    }

    memcpy(out, input, name_len);
    memcpy(out + 8, dot + 1, ext_len);

    strntoupper(out, 11);

    return 0; // Success
}
