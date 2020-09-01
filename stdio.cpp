
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <stdarg.h>
 

FILE* stdin;
FILE* stdout;
FILE* stderr;

static int g_enable=1;

void serial_enable(int en)
{
  g_enable = en;
}

static void put_serial(const char s)
{
  if (!g_enable) return;
  while ((inb(0x3f8+5) & (1<<5))==0);
  outb(s, 0x3f8);
}


int putchar(int c)
{
   if (c=='\n') put_serial('\r');
   put_serial(c);
   return 1;
}


int puts(const char *s)
{
 int count = 0;
 while (*s)
  {
    putchar(*s);
    s++;
    count++;
  }
  putchar('\n');
 return count;
}
       
       
static int print(const char* s)
{
  int count = 0;
  while (*s)
  {
    putchar(*s);
    s++;
    count++;
  }
  return count;
}



static int print_hex32(u32 a)
{
  int i;
  u8 j;
  static char buffer[20];
  for (i=0;i<8;i++)
  {
    j = ((a >> ((7-i)*4)) & 0xf);
    buffer[i] = j + ((j<10)?48:55);
  }
  buffer[8] = 0;
  buffer[9] = 0;
  return print(buffer);
}


static int print_hex16(u16 a)
{
  int i;
  u8 j;
  static char buffer[20];
  for (i=0;i<4;i++)
  {
    j = ((a >> ((3-i)*4)) & 0xf);
    buffer[i] = j + ((j<10)?48:55);
  }
  buffer[4] = 0;
  buffer[5] = 0;
  return printf(buffer);
}



int get_hex8(char* buffer, u32 len, u8 a)
{
  int i;
  u8 j;
  for (i=0;i<2;i++)
  {
    j = ((a >> ((1-i)*4)) & 0xf);
    buffer[i] = j + ((j<10)?48:55);
  }
  buffer[2] = 0;
  return 2;
}



static int print_hex8(u8 a)
{
  static char buffer[10];
  get_hex8(buffer, sizeof(buffer), a);
  return printf(buffer);
}


static int print_dec32(u32 a)
{
  static char buffer[20];
  buffer[19] = 0;
  char* ptr = &buffer[19];
  do
  {
    ptr--;
    // consume all digits.
    *ptr = (a % 10) + 48;
    a /= 10;
  } while (a);
  // Print the result
  return print(ptr);
}



int printf(const char* format, ...)
{
  int count = 0;  
  u32 u32_val;
  const char* char_val;
  va_list ap;
  va_start(ap, format); //Requires the last fixed parameter (to get the address)
  while (*format)
  {
    if (*format == '%')
    {
      format++;
      switch (*format)
      {
	case 0:
	  break;  // end of format string
	case '%': 
	  putchar('%');   // %%, put literal
	  count++;
	  format++;
	  break;
	case 's': 
	{	
	  char_val = va_arg(ap, char*);   // put the string verbatim
	  count += print(char_val);
	  format++;
	  break;
	}
	case 'x': 
	  u32_val = va_arg(ap, u32);   // assume 32-bit zero-padded hex
	  count += print_hex32(u32_val);
	  format++;
	  break;
	case 'd': 
	  u32_val = va_arg(ap, u32);   // decimal, no padding
	  count += print_dec32(u32_val);
	  format++;
	  break;
	default:
	  // more complicated types
	  if (strncmp(format, "04x", 3)==0)  // short, zero-padded
	  {
	    u32_val = va_arg(ap, u32);   
	    count += print_hex16(u32_val);
            format+=3;
	  }
	  else if (strncmp(format, "02x", 3)==0)
	  {
	    u32_val = va_arg(ap, u32);   // byte value, zero-padded
	    count += print_hex8(u32_val);
            format+=3;	    
	  }
	  else
	  {
	    putchar(*format);
	    count++;
	    format++;
	  }
      }
    }
    else  
    {
      putchar(*format);
      count++;
      format++;
    }
  }
  va_end(ap);
  return count;
}


int fprintf(FILE *stream, const char *format, ...)
{
//  printf(format, ...)
  return 0;
}

