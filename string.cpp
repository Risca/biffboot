
#include <string.h>


void* memcpy(void* dest, const void *src, size_t n)
{
  char* d=(char*)dest;
  char* s=(char*)src;
  while(n--)
  {
    *d = *s;
    d++;
    s++;
  }
  return dest;
}


int memcmp_fast(const unsigned int* s1, const unsigned int* s2, size_t n)
{
  while (n--)
  {
    if ((*s1) != (*s2)) return ((*s1) - (*s2));
    s1++;
    s2++;
  }
  return 0;
}


int memcmp(const void* s1, const void* s2, size_t n)
{
  if (n%4==0)
  {
    return memcmp_fast((unsigned int*)s1, (unsigned int*)s2, n/4);
  }

  unsigned char* s1_c = (unsigned char*)s1;
  unsigned char* s2_c = (unsigned char*)s2;
  while (n--)
  {
    if ((*s1_c) != (*s2_c)) return ((*s1_c) - (*s2_c));
    s1_c++;
    s2_c++;
  }
  return 0;
}


static void memclear_fast(unsigned int *s, size_t n)
{
  while (n--)
  {
    *s = 0;
    s++;
  }
}


void* memset(void *s, int c, size_t n)
{
  // Optimised version
  if ((n%4==0) && (c==0))
  {
    memclear_fast((unsigned int*)s, n/4);
    return s;
  }
  
  unsigned char* sc = (unsigned char*)s;
  while (n--)
  {
    *sc = c;
    sc++;
  }
  return s;
}


char* strcpy(char* dest, const char* src)
{
  char* tmp=dest;
  while (*src)
  {
     *dest = *src;
     dest++;
     src++;
  }
  *dest = *src;
  return tmp;
}


char* strncpy(char* dest, const char* src, size_t n)
{
  char* tmp=dest;
  while ((*src) && n)
  {
     *dest = *src;
     dest++;
     src++;
     n--;
  }
  if (n)
    *dest = *src;
  return tmp;
}


char* strcat(char* dest, const char* src)
{
  char* tmp = dest;
  while (*tmp) tmp++;
  strcpy(tmp, src);
  return dest;
}


size_t strlen(const char* txt)
{
  size_t count = 0;
  while (*txt)
  {
     count++;
     txt++;
  }
  return count;
}


int strcmp(const char *s1, const char *s2)
{
  while ((*s1) || (*s2))
  {
    if ((*s1) != (*s2)) return ((*s1) - (*s2));
    s1++;
    s2++;
  }
  return 0;
}

int strncmp(const char* s1, const char* s2, size_t len)
{
  while (((*s1) || (*s2)) && len)
  {
    if ((*s1) != (*s2)) return ((*s1) - (*s2));
    s1++;
    s2++;
    len--;
  }
  return 0;
}




