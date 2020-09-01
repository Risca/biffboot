
#include "munge.h"

unsigned char MUNGE(unsigned long o, unsigned char b)
{

	if (o<7) {
		return b ^ 0xff;
	} else if (o>99 && o<150) {
		return b ^ 0xff;
	} else if (o>268 && o<800) {
		return b ^ 0xff;
	} else if (o == 0x1540) {
		return b ^ 0xff;
	} else if (o == 0x2fa6) {
		return b ^ 0xff;
	} else if (o == 0x3000) {
		return b ^ 0xff;
	}

	return b;
}
