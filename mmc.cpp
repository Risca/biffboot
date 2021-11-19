/*-----------------------------------------------------------------------*/
/* MMC/SDSC/SDHC (in SPI mode) control module  (C)ChaN, 2007             */
/*-----------------------------------------------------------------------*/
/* Only rcvr_spi(), xmit_spi(), disk_timerproc() and some macros are     */
/* platform dependent.                                                   */
/*-----------------------------------------------------------------------*/

#include "mmc.h"


#include "io.h"
#include "unistd.h"
#include "gpio.h"

#include "iolib.h"
#include <stdlib.h>
#include <stdio.h>
#include "assem.h"

/* Results of Disk Functions */
typedef enum {
	RES_OK = 0,		/* 0: Successful */
	RES_ERROR,		/* 1: R/W Error */
	RES_WRPRT,		/* 2: Write Protected */
	RES_NOTRDY,		/* 3: Not Ready */
	RES_PARERR		/* 4: Invalid Parameter */
} DRESULT;

/* Disk Status Bits (int) */
#define STA_NOINIT		0x01	/* Drive not initialized */
#define STA_NODISK		0x02	/* No medium in the drive */
#define STA_PROTECT		0x04	/* Write protected */

/* Command code for disk_ioctrl() */

/* Generic command */
#define CTRL_SYNC		0	/* Mandatory for write functions */
/* MMC/SDC command */
#define MMC_GET_TYPE		10
#define MMC_GET_CSD		11
#define MMC_GET_CID		12
#define MMC_GET_OCR		13
#define MMC_GET_SDSTAT		14
/* ATA/CF command */
#define ATA_GET_REV		20
#define ATA_GET_MODEL		21
#define ATA_GET_SN		22




/* MMC/SD command (in SPI) */
#define CMD0	(0x40+0)	/* GO_IDLE_STATE */
#define CMD1	(0x40+1)	/* SEND_OP_COND */
#define	ACMD41	(0xC0+41)	/* SEND_OP_COND (SDC) */
#define CMD8	(0x40+8)	/* SEND_IF_COND */
#define CMD9	(0x40+9)	/* SEND_CSD */
#define CMD10	(0x40+10)	/* SEND_CID */
#define CMD12	(0x40+12)	/* STOP_TRANSMISSION */
#define ACMD13	(0xC0+13)	/* SD_STATUS (SDC) */
#define CMD16	(0x40+16)	/* SET_BLOCKLEN */
#define CMD17	(0x40+17)	/* READ_SINGLE_BLOCK */
#define CMD18	(0x40+18)	/* READ_MULTIPLE_BLOCK */
#define CMD23	(0x40+23)	/* SET_BLOCK_COUNT */
#define	ACMD23	(0xC0+23)	/* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24	(0x40+24)	/* WRITE_BLOCK */
#define CMD25	(0x40+25)	/* WRITE_MULTIPLE_BLOCK */
#define CMD41	(0x40+41)	/* SEND_OP_COND (ACMD) */
#define CMD55	(0x40+55)	/* APP_CMD */
#define CMD58	(0x40+58)	/* READ_OCR */


//GPIO_CLK = 1 << 11   # pin 2
//GPIO_MISO = 1 << 13   # pin 3
//GPIO_CS = 1 << 9    # pin 4
//GPIO_MOSI = 1 << 12   # pin 5

#define RDC_CONTROL 0x80003848
#define RDC_DATA    0x8000384c


static unsigned long last_gpio;


inline void fast_gpio_set_value(unsigned long mask, int value)
{
	if (value) {
        	last_gpio |= mask;                
        	fast_outl(last_gpio);
	} else {
        	last_gpio &= ~mask;                               
        	fast_outl(last_gpio);
	}	
}

// raise and lower clock
inline void CLOCK()  
{
  fast_clock(last_gpio);
}

inline void SELECT() { fast_gpio_set_value(GPIO_JTAG4, 0);  }
inline void DESELECT() { fast_gpio_set_value(GPIO_JTAG4, 1); }
// MOSI
inline void MMDI(int val) 
{ 
  fast_gpio_set_value(GPIO_JTAG5,val); 
}
// MISO
inline int MMDO() {
  return (fast_inl() & GPIO_JTAG3);
}

/*--------------------------------------------------------------------------

   Module Private Functions

---------------------------------------------------------------------------*/

static volatile
int Stat = STA_NOINIT;	/* Disk status */


static
u8 CardType;			/* b0:MMC, b1:SDv1, b2:SDv2, b3:Block addressing */


/*-----------------------------------------------------------------------*/
/* Transmit a byte to MMC via SPI  (Platform dependent)                  */
/*-----------------------------------------------------------------------*/


inline void xmit_spi (u8 dat)
{
	MMDI( (dat & 0x80) ? 1 : 0 );
	CLOCK();
	MMDI( (dat & 0x40) ? 1 : 0 );
	CLOCK();
	MMDI( (dat & 0x20) ? 1 : 0 );
	CLOCK();
	MMDI( (dat & 0x10) ? 1 : 0 );
	CLOCK();
	MMDI( (dat & 0x08) ? 1 : 0 );
	CLOCK();
	MMDI( (dat & 0x04) ? 1 : 0 );
	CLOCK();
	MMDI( (dat & 0x02) ? 1 : 0 );
	CLOCK();
	MMDI( (dat & 0x01) ? 1 : 0 );
	CLOCK();
}


/*-----------------------------------------------------------------------*/
/* Receive a byte from MMC via SPI (Platform dependent)                  */
/*-----------------------------------------------------------------------*/

inline u8 rcvr_spi (void)
{
	MMDI(1);	
	return fast_rcvr_spi(last_gpio);	
}


/*-----------------------------------------------------------------------*/
/* Wait for card ready                                                   */
/*-----------------------------------------------------------------------*/

static
u8 wait_ready (void)
{
	u8 res;

	rcvr_spi();
	do
		res = rcvr_spi();
	while (res != 0xFF);
	return res;
}


/*-----------------------------------------------------------------------*/
/* Deselect the card and release SPI bus                                 */
/*-----------------------------------------------------------------------*/

static
void release_spi (void)
{
	DESELECT();
	rcvr_spi();
}

static 
void power_on(void)
{
  	SELECT();
}

static 
void power_off(void)
{
  	DESELECT();
}


/*-----------------------------------------------------------------------*/
/* Receive a data packet from MMC                                        */
/*-----------------------------------------------------------------------*/

static
int rcvr_datablock (
	u8 *buff,			/* Data buffer to store received data */
	u32 btr			/* Byte count (must be even number) */
)
{
	u8 token;

	do {							/* Wait for data packet in timeout of 200ms */
		token = rcvr_spi();
	} while (token == 0xFF);
	if(token != 0xFE) return 0;	/* If not valid data token, retutn with error */

	do {							/* Receive the data block into buffer */
		*buff++ = rcvr_spi();
		*buff++ = rcvr_spi();
	} while (btr -= 2);
	rcvr_spi();						/* Discard CRC */
	rcvr_spi();

	return 1;					/* Return with success */
}


/*-----------------------------------------------------------------------*/
/* Send a command packet to MMC                                          */
/*-----------------------------------------------------------------------*/

static 
u8 send_cmd (
	u8 cmd,		/* Command byte */
	u32 arg		/* Argument */
)
{
	u8 n, res;


	if (cmd & 0x80) {	/* ACMD<n> is the command sequense of CMD55-CMD<n> */
		cmd &= 0x7F;
		res = send_cmd(CMD55, 0);
		if (res > 1) return res;
	}

	/* Select the card and wait for ready */
	DESELECT();
	SELECT();
	if (wait_ready() != 0xFF) return 0xFF;

	/* Send command packet */
	xmit_spi(cmd);						/* Start + Command index */
	xmit_spi((u8)(arg >> 24));		/* Argument[31..24] */
	xmit_spi((u8)(arg >> 16));		/* Argument[23..16] */
	xmit_spi((u8)(arg >> 8));			/* Argument[15..8] */
	xmit_spi((u8)arg);				/* Argument[7..0] */
	n = 0x01;							/* Dummy CRC + Stop */
	if (cmd == CMD0) n = 0x95;			/* Valid CRC for CMD0(0) */
	if (cmd == CMD8) n = 0x87;			/* Valid CRC for CMD8(0x1AA) */
	xmit_spi(n);

	/* Receive command response */
	if (cmd == CMD12) rcvr_spi();		/* Skip a stuff byte when stop reading */
	n = 10;								/* Wait for a valid response in timeout of 10 attempts */
	do
		res = rcvr_spi();
	while ((res & 0x80) && --n);

	return res;			/* Return with the response value */
}


/*--------------------------------------------------------------------------

   Public Functions

---------------------------------------------------------------------------*/

static int disk_initialize ()
{
	u8 n, ty, cmd, ocr[4];
	
	// Extra init with enable high
	rcvr_spi();
	
	for (n = 10; n; n--) rcvr_spi();	/* 80 dummy clocks */
	power_on();		/* Enable */
	ty = 0;
	if (send_cmd(CMD0, 0) == 1) {			/* Enter Idle state */
//		Timer1 = 100;						/* Initialization timeout of 1000 msec */
		if (send_cmd(CMD8, 0x1AA) == 1) {	/* SDHC */
        		printf("SDHC");
			for (n = 0; n < 4; n++) ocr[n] = rcvr_spi();		/* Get trailing return value of R7 resp */
			if (ocr[2] == 0x01 && ocr[3] == 0xAA) {				/* The card can work at vdd range of 2.7-3.6V */
				while (send_cmd(ACMD41, 1UL << 30));	/* Wait for leaving idle state (ACMD41 with HCS bit) */
				if (send_cmd(CMD58, 0) == 0) {		/* Check CCS bit in the OCR */
					for (n = 0; n < 4; n++) ocr[n] = rcvr_spi();
					ty = (ocr[0] & 0x40) ? 12 : 4;
				}
			}
		} else {							/* SDSC or MMC */
			if (send_cmd(ACMD41, 0) <= 1) 	{
                		printf("SDSC");
				ty = 2; cmd = ACMD41;	/* SDSC */
			} else {
                		printf("MMC");
				ty = 1; cmd = CMD1;		/* MMC */
			}
			while (send_cmd(cmd, 0));			/* Wait for leaving idle state */
			if (send_cmd(CMD16, 512) != 0)	/* Set R/W block length to 512 */
				ty = 0;
		}
	}
	CardType = ty;
	release_spi();

	if (ty) {			/* Initialization succeded */
	        printf(" initialised.\n");
		Stat &= ~STA_NOINIT;		/* Clear STA_NOINIT */
	} else {			/* Initialization failed */
		power_off();
	}

	return Stat;
}


/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

int disk_status (
)
{
	return Stat;
}


/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

int mmc_read (
	unsigned char *buff,			/* Pointer to the data buffer to store read data */
	unsigned long sector,		/* Start sector number (LBA) */
	unsigned char count			/* Sector count (1..255) */
)
{
	if (!count) return RES_PARERR;
	if (Stat & STA_NOINIT) return RES_NOTRDY;

	if (!(CardType & 8)) sector *= 512;	/* Convert to byte address if needed */

	if (count == 1) {	/* Single block read */
		if ((send_cmd(CMD17, sector) == 0)	/* READ_SINGLE_BLOCK */
			&& rcvr_datablock(buff, 512))
			count = 0;
	}
	else {				/* Multiple block read */
		if (send_cmd(CMD18, sector) == 0) {	/* READ_MULTIPLE_BLOCK */
			do {
				if (!rcvr_datablock(buff, 512)) break;
				buff += 512;
			} while (--count);
			send_cmd(CMD12, 0);				/* STOP_TRANSMISSION */
		}
	}
	release_spi();

	return count ? RES_ERROR : RES_OK;
}


int mmc_init(void)
{
  gpio_output(GPIO_JTAG2,0);   // CLK
  gpio_input(GPIO_JTAG3);      // MISO    
  gpio_output(GPIO_JTAG4,1);   // CS
  gpio_output(GPIO_JTAG5,1);   // MOSI
  
  outl(RDC_DATA, 0xcf8);   // Select data register.
  last_gpio = fast_inl();         

  return disk_initialize();
}

