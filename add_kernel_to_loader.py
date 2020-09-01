#!/usr/bin/env python

"""
   Adds a kernel to Biffboot so it can be booted.
   
   bios.bin = biffboot.bin + bzImage
"""

import struct, sys, hashlib

# Set the flash (BIOS) size - can be 1, 4, or 8.



def GetDeviceSize():
  "Load from setup.h the device details."
  s = "FLASHMAP_DEVICE_SIZE"
  val = 8
  kmax = 0x20
  if val==1:
    kmax = 0xf
  return val*0x100000, kmax


def GetConfig(kmax):
  "Return 8K config block with correct md5"

  # int version ==1 for this version.
  # Byte bootsource (0 = flash, 1=MMC)
  # Byte console (1 = serial console enable, 0=disable)
  # Byte nic (0 = net console disabled, doesn't work with Qemu anyhow!)
  
  boottype = 2
  # Byte boottype (1 = linux, 0 = flat bin, 2 = multiboot/ELF, 3 = coreboot payload)
  # unsigned long loadaddress (0x400000 default for Linux)

  config = struct.pack("<iBBBBL", 1,0,1,0,boottype, 0x400000)
 

  # Console for Linux  
  #cmdline = "console=uart,io,0x3f8"
  
  # Console for Linux
  cmdline = "console=uart,io,0x3f8 "
  
  while len(cmdline)<1024:
    cmdline += "\x00"
  block = config + cmdline
  
  # kernelmax is a 2-byte value
  block += struct.pack("<H", kmax)
    
  while len(block)<(0x2000-0x10):
    block += "\xff"
  
  block += hashlib.md5(block).digest()
  return block

  

if __name__ == "__main__":

  if not sys.argv[2:]:
    print  "usage: add_kernel_to_loader <kernel> <output>"
    sys.exit(0)
    
  bzimage = sys.argv[1]
  output = sys.argv[2]

  kern = file(bzimage).read()

  size,kmax = GetDeviceSize()
  
  ktop = kmax*0x10000

  if len(kern) > (ktop-0x2000):
    raise IOError("Kernel too large for this flash size")

  # First Pad up to just before config block we'll assume out test kernel
  while len(kern) < (ktop-0x2000):
    kern += "\xff"
    
  kern += GetConfig(kmax)
  
  # Now kernel is in logical order, have to 'wrap it'
    
  # The last bit goes at the bottom.
  bottom_chunk = kern[-0x6000:]

  # The first bit at the top.
  top_chunk = kern[:ktop-0x6000]

  firmware = bottom_chunk+top_chunk
  
  while len(firmware) < (size-0x10000):
    firmware += "\xff"

  file(output,"wb").write(firmware)

