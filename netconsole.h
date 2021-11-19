#pragma once
#ifdef __cplusplus
extern "C" {
#endif 

// Serve console requests.  On exit, continue boot sequence.
int netconsole_check(void);

int netconsole_ran();

#ifdef __cplusplus
}
#endif
