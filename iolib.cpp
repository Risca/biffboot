
#include "io.h"
#include <string.h>
#include <stdio.h>
#include "iolib.h"
#include "timer.h"
#include "history.h"
#include "config.h"



int print_biffboot(const char* p)
{
  return printf("BIFFBOOT: %s", p);
}



// Canonical style of dump.
void hex_dump(void* buffer, int length)
{
  int nl = 0;
  u8* j = (u8*)buffer;
  while (length--)
  {
      printf("%02x", *j);
      j++;
      if (nl++ > 32)
      {
        printf("\n");
        nl=0;
      }
  }
}


// no overflow checking
u32 hex_to_u32(char* buffer, u32 length)
{
  u32 out=0;
  while (length--)
  {
    if (*buffer)
    {
      if (*buffer > 47 && *buffer < 58)
        out |= (*buffer - 48);
      if (*buffer > 96 && *buffer < 103)
        out |= (*buffer - 87);
    }
    if (length)
      out <<= 4;
    buffer++;
  }
  return out;
}


// Indicate key has been pressed.
int keyready()
{
  return (inb(0x3f8+5) & 1) ? 1 : 0;
}
          

typedef struct _escape_code
{
  u8 value;
  const char* code;
} _escape_code_t;


static struct _escape_code g_escape_codes[] = 
{
  { IOLIB_CURSOR_UP, "[A"},
  { IOLIB_CURSOR_DOWN, "[B"},
  { IOLIB_CURSOR_RIGHT, "[C"},
  { IOLIB_CURSOR_LEFT, "[D"},
  {0, (const char*)NULL}
};


// escape code captures 
static u8 g_escape_string[8];

u8 get_escape()
{
  _escape_code_t* esc = g_escape_codes;
  u32 remain=sizeof(g_escape_string)-1;
  u8* index=g_escape_string;
  // @115200 baud, characters appear 10 per millisecond, so 1mS should be
  // enough time for the escape code to fill the 16-byte fifo.
  timer_mdelay10();
  while (keyready() && remain--)
  {
    *index = inb(0x3f8);
    index++;
  }
  *index = 0;
  
  // map the escape code onto some random non-ascii values, to pass it back to the 
  // command processor.
  while (esc->code)
  {
    if (strcmp(esc->code, (char*)g_escape_string)==0)
    {
      return esc->value;
    }
    esc++;
  }
  return 27;   // it's just the plain old escape key.
}


// 100mS period
int getkey(int timeout)
{
  u8 key = 0;
  if (timeout) {
    while (timeout--)
    {
      if (keyready())
      {
        key = inb(0x3f8);
        break;
      }
      timer_mdelay10();
    }
  }
  else
  {
    if (keyready())
      key = inb(0x3f8);
  }
  if (key == 27)
    key = get_escape();
  return key;
}




u32 getint(int digits)
{
  int val=0;
  int k;
  int i=digits;
  int mult=1;
  char buffer[20];
  if (digits > 19) return 0;
  while (i--)
  {
    k = 0;
    while ((k<48) || (k>57))
    {
      k = getkey(0);
    }
    putchar(k);
    buffer[i] = k-48;
  }
  for (i=0;i<digits;i++)
  {
    val += buffer[i] * mult;
    mult *= 10;
  }
  return val;
}


// No abort, we wait for the packet forever.
void get_serial_data(u8* pkt, u32 length)
{
  while (length--)
  {
    while ((inb(0x3f8+5) & 1)==0);
    *pkt = inb(0x3f8);
    pkt++;
  }
}


static int is_hex(char k)
{
  if ((k>47 && k<58) || (k>96 && k<103)) return 1;
  return 0;
}


static char* backspace(u32 count, char* ptr)
{
  while (count--)
  {
      putchar(8);   // move cursor back
      putchar(' ');   // print over to erase
      putchar(8);   // move cursor back
      ptr --;
      *ptr = 0;   // move back termination
  }
  return ptr;
}


// erase the last token, 
static char* erase_word(u32 count, char* ptr)
{
  while (count--)
  {
    if (*(ptr-1)==' ') break;
    ptr = backspace(1,ptr);
  }
  return ptr;
}


static char* replace_with(char* buffer, const char* new_text)
{
   strcpy(buffer, new_text);
   printf(buffer);
   return buffer + strlen(buffer);  
}


// Enter characters until the enter key is pressed.  First key is an initial
// key to add to the input buffer.
char get_ascii_data(char* buffer, u32 max, char first_key, int hexonly, int history, match_fn completion)
{
  u8 tmp;
  char* pkt = buffer;
  *pkt = 0;
  if ((!max) || (!pkt)) return 27;
  
  while (1)  // space for null
  {

    if (first_key)
    {
      tmp = first_key;
      first_key = 0;
    }
    else
    {
      // Get the character
      tmp = getkey(0);
    }

    // Exit conditions
    if ((tmp == 27) || (tmp == '\r') || (tmp == '\n')) break;
    if (tmp==8)  // backspace
    {
      if (pkt==buffer) continue;  // do nothing if string empty.
      pkt = backspace(1, pkt);
      continue;
    }
    if (completion)
    {
      if (tmp == 9)
      {
        const char* ptr = completion(buffer);
        if (ptr)
        {
          pkt = erase_word(pkt-buffer, pkt);
          strcpy(pkt, ptr);
          printf(ptr);
          pkt += strlen(ptr);
        }
      }
    }
    if (history)
    {
      switch (tmp)
      {
        const char* ptr;
        case IOLIB_CURSOR_UP:
        case IOLIB_CURSOR_DOWN:
          ptr = history_updown((tmp==IOLIB_CURSOR_UP)?1:0);
          if (ptr)
          {
            backspace(pkt-buffer, pkt);
            pkt = replace_with(buffer, ptr);
          }
          else
          {
            backspace(pkt-buffer, pkt);
            pkt = replace_with(buffer, "");
          }
          break;
      }
    }
    // Otherwise Ignore if not printable.
    if ((tmp < 32) || (tmp>126)) continue;
    if (hexonly)
    {
      if (!is_hex(tmp)) continue;
    }
    // refuse to add any extra characters
    if (u32(pkt-buffer)>=(max-1)) continue;
    putchar(tmp);
    *pkt = tmp;
    *(pkt+1) = 0;  // terminate
    pkt++;
  }
  return tmp;
}



void fatal(const char* msg)
{
  printf(msg);
  printf("\n");
  while (1);
}


void prompt()
{
  if (config_get_changed())
  {
    printf("* ");
  }
  printf("BIFFBOOT> ");
}


int are_you_sure()
{
  int key;
  printf(" are you sure? (y/n): ");
  while (1)
  {
    key = getkey(0);
    if ((key=='n') || (key=='N'))
    {
      putchar(key);
      printf("\n");
      return 0;
    }
    if ((key=='y') || (key=='Y')) break;
  }
  putchar(key);  printf("\n");
  return 1;
}


void print_erase(int count)
{
  while (count--)
  {
    printf("\b");
  }
}


int is_whitespace(char ptr)
{
  if (ptr ==' ') return 1;
  if (ptr =='\t') return 1;
  return 0;
}


// Skip over any whitespace
const char* skip_whitespace(const char* ptr)
{
  while (is_whitespace(*ptr))
    ptr++;
  return ptr;
}


// search the string for the token, return pointer following
// token or null if no match.
const char* eat_token(const char* str, const char *tok)
{
  u32 len;
  const char* ptr = str;
  // skip preceding whitespace
  ptr = skip_whitespace(ptr);
  len = strlen(tok);
  if (strncmp(tok, ptr, len))
    return (const char*)NULL;   // no match at all
  // skip over matched token
  ptr += len;
  
  if (*ptr)
  {
    // if there's more check for extra chars after match
    if (!is_whitespace(*ptr)) return (const char*)NULL;
  }
  // skip any whitespace remaining
  return skip_whitespace(ptr);
}


void stream_write(char** ptr, const char* val)
{
  strcpy(*ptr, val);
  *ptr += strlen(val) + 1;
}

void stream_write_char(char** ptr, const char val)
{
  **ptr = val;
  (*ptr) ++;
}


