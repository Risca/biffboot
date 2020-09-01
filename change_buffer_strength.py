#!/usr/bin/env python

import sys, struct, binascii


BUFFER_STRENGTH_OFFSET = 0xfff4


def Help():
  print "Usage: change_buffer_strength.py <biffboot image> <register value>"
  print 
  sys.exit(1)


if __name__ == "__main__":

  if not sys.argv[1:]:
    Help()

  fname = sys.argv[1]
  buf_strength = sys.argv[2]
  val = eval(buf_strength)
  data = file(fname,"rb").read()
  new_buf = struct.pack("<H", val)
  print "Little-endian word will be written to buffer strength control config as hex: %s" % binascii.hexlify(new_buf)
  data = data[:BUFFER_STRENGTH_OFFSET] + new_buf + data[BUFFER_STRENGTH_OFFSET+2:]
  file(fname, "wb").write(data)
  print "File '%s' updated" % fname
  
  
  