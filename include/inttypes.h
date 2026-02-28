#ifndef INTTYPES_H
#define INTTYPES_H

typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;
typedef signed char         int8_t;
typedef signed short        int16_t;
typedef signed int          int32_t;
typedef signed long long    int64_t;
typedef signed int          intptr_t;
typedef unsigned int		uintptr_t;

#if defined(__i386__)
	#define UINTPTR_MAX 0xFFFFFFFF
#elif defined(__amd64__)
	#define UINTPTR_MAX 0xFFFFFFFFFFFFFFFF
#else
	#error "Unsupported architecture"
#endif

#endif // INTTYPES_H
