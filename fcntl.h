#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "stddef.h"
int open(const char *pathname, int flags);

#define O_RDWR 1

#ifdef __cplusplus
}
#endif

