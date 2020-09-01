#!/usr/bin/env python
# -*- coding: iso-8859-1 -*-

import os

files = """
    main.o iolib.o flash.o md5.o string.o bootlinux.o r6040.o stdlib.o
    gpio.o config.o button.o led.o mmc.o timer.o netconsole.o transit.o
    loader.o flashmap.o bootp.o ether.o udp.o arp.o tftp.o ohci.o
    bootmulti.o elfutils.o bootcoreboot.o oldconsole.o history.o
	 ubifs.o crc32.o idt.o gdt.o isr.o icmp.o udpconsole.o
"""

for i in files.split():
  d = i.replace(".o",".cpp")
  s = i.replace(".o",".c")
  cmnd = "mv %s %s" % (s,d)
  print cmnd
  os.system(cmnd)

