#pragma once
#ifdef __cplusplus
extern "C" {
#endif 

#include "stdlib.h"

int r6040_get_mac_suffix(char* buffer, u32 len);
void r6040_print_common_mac();
void r6040_set_mac(void);
//void r6040_set_community_mac(void);


// setup descriptors, start packet reception
void r6040_init(void);

void r6040_rx_enable(void);

// Disable packet reception.
void r6040_rx_disable(void);

// queue packet for transmission
void r6040_tx(unsigned char* pkt, size_t length);

// Returns size of pkt, or zero if none received.
size_t r6040_rx(unsigned char* pkt, size_t max_len);

unsigned short r6040_mdio_read(int, int);

void r6040_phy_print();

int r6040_wait_linkup(int delay, int* uptime);


#define R6040_RX_DESCRIPTORS 32

#ifdef __cplusplus
}
#endif
