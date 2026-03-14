#ifndef _STDDEF_H_
#define _STDDEF_H_

#define NULL ((void *)0)

typedef unsigned int size_t;
typedef int ssize_t;

#ifndef offsetof
#define offsetof(type, member) ((size_t) &((type *)0)->member)
#endif

#endif // _STDDEF_H_
