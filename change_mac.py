#!/usr/bin/env python

import sys

if sys.argv[1:]:
  mac_bits = sys.argv[1].split(":")
  mac_num = "".join([ chr(eval("0x" + i)) for i in mac_bits ])
  dat = file("biffboot.bin","rb").read()

  out = dat[:0xfff8] + mac_num + dat[-2:]
  file("biffboot.bin","wb").write(out)
  print "biffboot.bin updated"
else:
  print "Usage: change_mac.py <mac>"


