#ifndef STRING_H
#define STRING_H

#include <stdint.h>

// Copy a string
void strcpy(char *dest, const char *src);
// Copy n characters of a string
void strncpy(char *dest, const char *src, unsigned n);
// Compare two strings
int strcmp(const char *str1, const char *str2);
// Compare n characters of two strings
int strncmp(const char *a, const char *b, unsigned n);
// Get the length of a string
unsigned strlen(const char *s);
// Concatenate a string and a character
char *strcat(const char *s, char c);
// Convert an unsigned integer to a string
char *uint_to_string(uint32_t u, char *str);
// Reverse a string
char *strrev(char *str);

// Tokenize a string 
char *strtok(char *str, const char *delim);
// Duplicate a string
char *strdup(const char *s);
// Duplicate n characters of a string
char *strndup(const char *s, unsigned n);
// Convert a string to uppercase
void *strtoupper(char *str);
// Convert a string to lowercase
void *strtolower(char *str);

// Convert a string to an integer
int str2int(const char *str);

// Find the first occurrence of a character in a string
const char *strchr(const char *s, int c);

// Fill a block of memory with a value
void memset(void *dest, char val, unsigned len);
// Copy a block of memory
void memcpy(void *dest, const void *src, unsigned len);

#endif // STRING_H