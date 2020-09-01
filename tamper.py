#!/usr/bin/env python


if __name__ == "__main__":

  a = file("biffboot.bin", "rb").read()
  m = a[:0xfff8]
  t = a[0xfffe:]
  
  mac = "\x00\x01\x02\x03\x04\x05"
  
  file("biffboot_tampered.bin","wb").write(m + mac + t)
  