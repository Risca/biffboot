#pragma once
#ifdef __cplusplus
extern "C" {
#endif 

void transit_init();
void transit_reset();
int transit_add(unsigned char * buffer, size_t length);
void transit_digest(unsigned char* digest);
unsigned char* transit_payload_ptr();
unsigned long transit_payload_length();

#ifdef __cplusplus
}
#endif
