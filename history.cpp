
#include <string.h>
#include "iolib.h"
#include "history.h"


#define ENTRY_MAX 9

typedef struct _entry
{
  char buffer[60];
} _entry_t;

static struct _entry g_table[ENTRY_MAX+1];

#define ENTRY_SIZE (sizeof(g_table)/sizeof(_entry_t))


static u32 g_latest;
static u32 g_pullfrom;

void history_init()
{
  u32 i;
  g_latest = 0;
  g_pullfrom = 0;
  for (i=0;i<ENTRY_SIZE;i++)
  {
    g_table[i].buffer[0] = 0;
  }
  history_append("");
}


static u32 plus_one(u32 current)
{
  current ++;
  if (current > ENTRY_MAX)
    current = 0;
  return current;
}


static u32 minus_one(u32 current)
{
  if (current ==0)
  {
    current = ENTRY_MAX;
  }
  else
  {
    current--;
  }
  return current;
}


// last successfully executed command
void history_append(const char* command)
{
  strncpy(g_table[g_latest].buffer, command, sizeof(_entry_t));
  g_latest = plus_one(g_latest);
  g_pullfrom = g_latest;   // reset current cursor pointer
}


// return the text after 'up-arrow'
const char* history_updown(u32 up)
{
  u32 test = up ? minus_one(g_pullfrom) : plus_one(g_pullfrom);
  if (g_table[test].buffer[0])
  {
    g_pullfrom = test;
    return g_table[test].buffer;
  }
  return NULL_STRING;
}




