#pragma once
#ifdef __cplusplus
extern "C" {
#endif 

// Deal with ping requests return 0 if the packet was processed
int icmp_check(unsigned char* pkt);

#ifdef __cplusplus
}
#endif
