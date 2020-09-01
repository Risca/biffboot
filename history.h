#pragma once
#ifdef __cplusplus
extern "C" {
#endif 


// reset the buffer
void history_init();

// last successfully executed command
void history_append(const char* command);

// Return the saved command
const char* history_updown(u32 up);

#ifdef __cplusplus
}
#endif
