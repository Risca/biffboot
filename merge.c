//
// Combine BIOS image
// 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "version.h"
#include "setup.h"



size_t ReadFile(char* name, unsigned char* buffer, size_t max)
{
  FILE *fp;
  size_t got =0;
  fp = fopen(name, "rb");
  if (fp == NULL) {
	perror(name);
	exit(1);
  }
  got = fread(buffer, 1, max, fp);
  if (!got)
  {
    fprintf(stderr, "No data in file %s", name);
    exit(1);
  }
  fclose(fp);
  return got;
}


void WriteFile(char* name, const unsigned char* buffer, size_t max)
{
  FILE *fp;
  size_t got;
  fp = fopen(name, "wb");
  if (fp == NULL) {
	perror(name);
	exit(1);
  }
  got = fwrite(buffer, 1, max, fp);
  if (got !=max)
  {
    fprintf(stderr, "Couldn't write all bytes to %s", name);
    exit(1);
  }
  fclose(fp);
}


void WriteFile2Mac(unsigned char* bin, const unsigned char* buffer, size_t max)
{
  char name[1024];
  char version[100];
  char* p = version;
  strcpy(version, BIFFBOOT_VERSION);
//#ifdef COMMUNITY_EDITION
  strcat(version, " community");
//#endif
  while (*p)
  {
  	if ((*p) == ' ') *p = '_';
  	if ((*p) == '.') *p = '_';
  	p++;
  }
  sprintf(name, "biffboot-%02x%02x%02x%02x%02x%02x-%s.bin", 
  	bin[0], bin[1], bin[2], bin[3], bin[4], bin[5], version);
  WriteFile(name, buffer, max);
}



int main(int argc, char *argv[])
{
	static unsigned char inbuf[0x10000];
	static unsigned char head[0x10000];
	static unsigned char mac[6];
	unsigned char* ptr = inbuf;
	size_t inlen = 0;
	size_t headlen = 0;

	// ./merge.py lz.bin main.bin.z head.bin
	if (argc != 6) {
		fprintf(stderr,
			"Merge parts of biffboot together\n"
			"Usage: ./merge <lz.bin> <main.bin.z> <head.bin> <mac.bin> <output>\n");
		return 1;
	}
	
	inlen += ReadFile(argv[1], inbuf, sizeof(inbuf));
	ptr += inlen;

	inlen += ReadFile(argv[2], ptr, sizeof(inbuf)-inlen);
	ptr += inlen;

	if (inlen > 0xf000)
	{
	    fprintf(stderr, "main BIOS above max size (0xf000)");
	    return 1;
	}

	headlen = ReadFile(argv[3], head, sizeof(head));
	if (headlen!= 0x10000)
	{
	    fprintf(stderr, "head length should be 0x10000 bytes");
	    return 1;	    
	}
	
	memcpy((char*)head, (char*)inbuf, inlen);

	// Add copyright information
	sprintf((char*)(head+0xff00), "Biffboot %s, Copyright (c) Bifferos.com. "
		" Redistribution prohibited, all rights reserved. "
		" Contact: sales@bifferos.com", BIFFBOOT_VERSION);

	ReadFile(argv[4], mac, sizeof(mac));
	memcpy((char*)(head+0xfff8), mac, sizeof(mac));
	head[0xfffe] = 0;  // 1, 4 or 8.  0 = auto-detect

	WriteFile(argv[5], head, sizeof(head));
	WriteFile2Mac(mac, head, sizeof(head));
	return 0;
}
