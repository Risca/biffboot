#pragma once
#ifdef __cplusplus
extern "C" {
#endif 

u32 loader_load(void);

u32 loader_copy_from_network(u32 dest);

#ifdef __cplusplus
}
#endif
