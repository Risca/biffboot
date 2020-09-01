#pragma once
#ifdef __cplusplus
extern "C" {
#endif 

#include "types.h"



void* dma_malloc(size_t count);
void dma_free(void* ptr);
void dma_free_all();
void* malloc(size_t count);
void free(void* ptr);

void abort(void);


#ifdef __cplusplus
}
#endif
