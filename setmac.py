#!/usr/bin/env python

# writes a MAC address to the mac.bin file.

import sys, binascii
from StringIO import StringIO

def GetMac(dotted):
  if dotted.find(":") == -1:
    str = binascii.unhexlify(dotted)
  else:
    str = "".join([chr(eval("0x"+i)) for i in dotted.split(":")])
  
  if len(str) != 6:
    raise ValueError("Not a MAC address")
  return str

if __name__ == "__main__":
  if sys.argv[1:]:
    mac = GetMac(sys.argv[1])
    # pad to 8-byte boundary
    mac += "\x00\x00"
    file("mac.bin","wb").write(mac)
  else:
    bin = file("mac.bin","rb").read()[:6]
    print ":".join([binascii.hexlify(i) for i in bin])
    
