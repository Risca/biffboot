#!/usr/bin/env python

import os, stat, sys

tinflate_dot_h = """
#define STUB_SIZE %d
"""

if __name__ == "__main__":
	tmp = os.stat("lz.bin").st_size
	file("tinflate.h","wb").write(tinflate_dot_h % tmp)

