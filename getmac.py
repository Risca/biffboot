#!/usr/bin/env python

import pod
import sys, binascii
from StringIO import StringIO

def ConvertMac(dotted):
  if dotted.find(":") == -1:
    str = binascii.unhexlify(dotted)
  else:
    str = "".join([chr(eval("0x"+i)) for i in dotted.split(":")])
  
  if len(str) != 6:
    raise ValueError("Not a MAC address")
  return str



def Help():
  print "Usage: getmac.py"
  print "Copies mac to mac.bin"
  sys.exit(-1)


if __name__ == "__main__":

  p = pod.Pod("turbo")
  p.GetMac()
  p.Close()


