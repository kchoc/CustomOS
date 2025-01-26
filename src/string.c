#include "string.h"
#include "kernel/memory/kmalloc.h"

#include <stdint.h>
#include <stddef.h>

void strcpy(char *dest, const char *src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

void strncpy(char *dest, const char *src, unsigned n) {
    while (*src && n--) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

int strcmp(const char *str1, const char *str2) {
    while (*str1 && *str2) {
        if (*str1 != *str2)
            return *str1 - *str2;
        str1++;
        str2++;
    }
    return *str1 - *str2;
}

int strncmp(const char *a, const char *b, unsigned n) {
    while (*a && *b && n--) {
        if (*a != *b)
            return *a - *b;
        a++;
        b++;
    }
    return *a - *b;
}

unsigned strlen(const char *s) {
    unsigned len = 0;
    while (*s++) {
        len++;
    }
    return len;
}

char *strcat(const char *s, char c) {
    while (*s) {
        if (*s == c)
            return (char *)s;
        s++;
    }
    return NULL;
}

char *uint_to_string(uint32_t u, char *str) {
    char *start = str;
    char *end;
    char temp;

    // Convert number to string in reverse order
    do {
        *str++ = (u % 10) + '0';
        u /= 10;
    } while (u);

    // Null-terminate the string
    *str = '\0';

    // Reverse the string
    end = str - 1;
    while (start < end) {
        temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
    }

    return str;
}

char *strrev(char *str) {
    char *start = str;
    char *end;
    char temp;

    // Find the end of the string
    end = str;
    while (*end++) { }
    end--;

    // Reverse the string
    while (start < end) {
        temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
    }

    return str;
}

char *strtok(char *str, const char *delim) {
    static char *last = NULL;
    char *start;
    const char *d;

    // If str is NULL, use the last token as the start
    if (str == NULL) {
        str = last;
    }

    // Skip leading delimiters
    while (*str) {
        for (d = delim; *d; d++) {
            if (*str == *d)
                break;
        }
        if (!*d)
            break;
        str++;
    }

    // If we're at the end of the string, return NULL
    if (!*str) {
        last = NULL;
        return NULL;
    }

    // Save the start of the token
    start = str;

    // Find the end of the token
    while (*str) {
        for (d = delim; *d; d++) {
            if (*str == *d)
                break;
        }
        if (*d)
            break;
        str++;
    }

    // Null-terminate the token
    if (*str) {
        *str = '\0';
        str++;
    }

    // Save the last token
    last = str;

    return start;
}

char *strdup(const char *s) {
    char *new = kmalloc(strlen(s) + 1);
    if (new)
        strcpy(new, s);
    return new;
}

char *strndup(const char *s, unsigned n) {
    char *new = kmalloc(n + 1);
    if (new)
        strncpy(new, s, n);
    return new;
}

void *strtoupper(char *str) {
    while (*str) {
        if (*str >= 'a' && *str <= 'z')
            *str -= 32;
        str++;
    }
}

void *strtolower(char *str) {
    while (*str) {
        if (*str >= 'A' && *str <= 'Z')
            *str += 32;
        str++;
    }
}

int str2int(const char *str) {
    int result = 0;
    int sign = 1;

    // Skip leading whitespace
    while (*str == ' ' || *str == '\t')
        str++;

    // Check for a sign
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    // Parse the number
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }

    return result * sign;
}

const char *strchr(const char *s, int c) {
    while (*s) {
        if (*s == c)
            return s;
        s++;
    }
    return NULL;
}

void memset(void *dest, char val, unsigned len) {
    char *d = dest;
    while (len--) {
        *d++ = val;
    }
}

void memcpy(void *dest, const void *src, unsigned len) {
    char *d = dest;
    const char *s = src;
    while (len--) {
        *d++ = *s++;
    }
}
