; NASM definitions for buffer strength control registers

%define ROMCSn_DRIVE_4mA          0010b << 12
%define ROMCSn_DRIVE_8mA          0110b << 12
%define ROMCSn_DRIVE_12mA         1010b << 12
%define ROMCSn_DRIVE_16mA         1110b << 12
%define ROMCSn_SLEW_FAST          0010b << 12
%define ROMCSn_SLEW_SLOW          0011b << 12

%define SDRAM_CTRL_2mA            0000b << 8
%define SDRAM_CTRL_4mA            0010b << 8
%define SDRAM_CTRL_6mA            0100b << 8
%define SDRAM_CTRL_8mA            0110b << 8
%define SDRAM_CTRL_10mA           1000b << 8
%define SDRAM_CTRL_12mA           1010b << 8
%define SDRAM_CTRL_14mA           1100b << 8
%define SDRAM_CTRL_16mA           1110b << 8
%define SDRAM_CTRL_SLEW_FAST      0000b << 8
%define SDRAM_CTRL_SLEW_SLOW      0001b << 8

%define SDRAM_DRIVE_MA_4mA        0000b << 4
%define SDRAM_DRIVE_MA_8mA        0100b << 4
%define SDRAM_DRIVE_MA_12mA       1000b << 4
%define SDRAM_DRIVE_MA_16mA       1100b << 4
%define SDRAM_SLEW_MA_FAST        0000b << 4
%define SDRAM_SLEW_MA_SLOW        0001b << 4

%define SDRAM_DRIVE_MD_2mA_4mA    0000b
%define SDRAM_DRIVE_MD_4mA_4mA    0010b
%define SDRAM_DRIVE_MD_6mA_8mA    0100b
%define SDRAM_DRIVE_MD_8mA_8mA    0110b
%define SDRAM_DRIVE_MD_10mA_12mA  1000b
%define SDRAM_DRIVE_MD_12mA_12mA  1010b
%define SDRAM_DRIVE_MD_14mA_16mA  1100b
%define SDRAM_DRIVE_MD_16mA_16mA  1110b

%define SDRAM_SLEW_MD_FAST        0000b
%define SDRAM_SLEW_MD_SLOW        0001b


; 0x2313 works for etron
; 0x2315 works for old sdram
%define DRAM_BUFFER_STRENGTH                                     \
		ROMCSn_DRIVE_4mA       | ROMCSn_SLEW_FAST |      \
		SDRAM_CTRL_4mA         | SDRAM_CTRL_SLEW_SLOW |  \
		SDRAM_DRIVE_MA_4mA     | SDRAM_SLEW_MA_SLOW |    \
		SDRAM_DRIVE_MD_4mA_4mA | SDRAM_SLEW_MD_SLOW      \

