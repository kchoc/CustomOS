#ifndef STDDEF_H
#define STDDEF_H

#define NULL ((void *)0)

typedef unsigned int size_t;
typedef int ssize_t;

#ifndef offsetof
#define offsetof(type, member) ((size_t) &((type *)0)->member)
#endif

#endif // STDDEF_H
