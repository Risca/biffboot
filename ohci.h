#pragma once
#ifdef __cplusplus
extern "C" {
#endif 

#define OHCI_PACK __attribute__((__packed__))

typedef struct _hcca {
  unsigned long   int_table[32];
  unsigned short  frame_no;               
  unsigned short  pad1;                   
  unsigned long   done_head;              
  unsigned char   reserved_for_hc[116];
} OHCI_PACK _hcca_t;

typedef struct _ohci_regs {
  unsigned long revision;
  unsigned long control;
  unsigned long command_status;
  unsigned long interrupt_status;
  unsigned long interrupt_enable;
  unsigned long interrupt_disable;
  _hcca_t* hcca;
  unsigned long period_current_ed;
  unsigned long control_head_ed;
  unsigned long control_current_ed;
  unsigned long bulk_head_ed;
  unsigned long bulk_current_ed;
  unsigned long done_head;
  unsigned long fm_interval;
  unsigned long fm_remaining;
  unsigned long fm_number;
  unsigned long periodic_start;
  unsigned long ls_threshold;
  unsigned long rh_descriptor_a;
  unsigned long rh_descriptor_b;
  unsigned long rh_status;
  unsigned long rh_port_status_1;
  unsigned long rh_port_status_2;
} OHCI_PACK _ohci_regs_t;


void ohci_init();

#ifdef __cplusplus
}
#endif
