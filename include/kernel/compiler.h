#ifndef COMPILER_H
#define COMPILER_H

#define __user
#define __kernel

#define __cleanup(x) __attribute__((cleanup(x)))

#define __EXPORT_SYMBOL(sym, gpl) \
    extern typeof(sym) sym;

#define EXPORT_SYMBOL(sym) __EXPORT_SYMBOL(sym, "")

#endif // COMPILER_H
