#ifndef x86_FEATURES_H
#define x86_FEATURES_H

#include <kern/types.h>

bool x86_cpu_has_feature(const char* feature);
bool x86_has_pae();

#endif // x86_FEATURES_H
