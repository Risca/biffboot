#!/usr/bin/env python


import binascii, os, glob


g_macs_done = """
00B3F6006EB1  
00B3F6006EAF  
00B3F6006E88
00B3F6008B5C
00B3F6008B60 
00B3F6008B64
00B3F6008B65
00B3F6008B66
00B3F6008B6D
00B3F6008B6E
00B3F6008B6F
00B3F6008B71
00B3F6008B72
00B3F6008B73
00B3F6008B74
00B3F6008B75
00B3F6008B78
00B3F6008B8C
00B3F6008B8D
00B3F6008C8D
"""


g_macs = """
00B3F6005F5D
00B3F6006055
00B3F6006E2D 
00B3F6006E2E 
00B3F6006E30 
00B3F6006E33 
00B3F6006E70 
00B3F6006E73
00B3F6006E83
00B3F6006EA6
00B3F6006EB9 
00B3F6006EBB 
00B3F6008C8B
"""


def GetList():
  out = []
  for j in g_macs.split("\n"):
    i = j.strip()
    if i != "":
      print repr(i)
      a = binascii.unhexlify(i)
      out.append(":".join(["%02x"%ord(k) for k in a]))
      
  return out


def Build():
  for i in GetList():
    os.system("make clean")
    os.system("./set_mac.py %s" % i)
    os.system("make")





if __name__ == "__main__":

  Build()  
  #for i in glob.glob("biffboot-*.bin"):
  #  os.system("hexdump -Cv %s > %s" % (i,i+".txt"))
    
  