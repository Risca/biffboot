#pragma once
#ifdef __cplusplus
extern "C" {
#endif 


// Set red LED on/off
void led_on();
void led_off();

void led_onoff(const char* arg);

const char* led_complete(const char* partial);

#ifdef __cplusplus
}
#endif


