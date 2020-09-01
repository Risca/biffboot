
TOOLCHAIN_PREFIX = /home/biff/bb/openwrt/staging_dir/toolchain-i386_gcc-4.5-linaro_uClibc-0.9.32/bin/i486-openwrt-linux
GPP = $(TOOLCHAIN_PREFIX)-g++
LD = $(TOOLCHAIN_PREFIX)-ld
CC = $(TOOLCHAIN_PREFIX)-gcc

all: biffboot.bin  


setup.i : setup.py
	./setup.py
		
setup.h : setup.py
	./setup.py

version.h : setup.py
	./setup.py


RELEASE=../release/


.PHONY: upload run see clean all off on mac programconfig dumpconfig release


run: biffboot.bin flash.bin
	@echo "Ctrl-A - X to quit emulation"
#	qemu -cpu 486 -m 64 -L ./ -hda /dev/zero
	../../bb/qemu/i386-softmmu/qemu -cpu 486 -m 32 -nographic -usb -no-acpi -L ./  \
		-hda /dev/zero -vga none -bios biffboot.bin


upload: biffboot.bin
	./upload.py $<
	
off: 
	echo 1 > /sys/class/gpio/gpio7/value

on: 
	echo 0 > /sys/class/gpio/gpio7/value

reset: 
	echo 1 > /sys/class/gpio/gpio7/value
	echo 0 > /sys/class/gpio/gpio7/value


flash.bin: bzImage add_kernel_to_loader.py
	./add_kernel_to_loader.py bzImage $@


/tmp/public/biffboot.bin : biffboot.bin
	cp biffboot.bin /tmp/public/biffboot.bin


merge: merge.c version.h
	gcc -o merge -Wall merge.c


# Combine the head with DRAM init, with the compressed bootloader
biffboot.bin: lz.bin main.bin.z head.bin merge
	./merge lz.bin main.bin.z head.bin mac.bin $@


head.bin : head.S setup.i buffer_control.i
	nasm -o head.bin head.S
#	hexdump -Cv fw.bin| less

assem.o : assem.S setup.i
	nasm -f elf -o assem.o assem.S

interrupt.o : interrupt.S
	nasm -f elf -o interrupt.o interrupt.S


CFLAGS += -Wall -nostdinc -I.
objects = main.o iolib.o flash.o md5.o string.o bootlinux.o r6040.o stdlib.o \
	 gpio.o config.o button.o led.o mmc.o timer.o netconsole.o transit.o \
	 loader.o flashmap.o bootp.o ether.o udp.o arp.o tftp.o ohci.o \
	 bootmulti.o elfutils.o bootcoreboot.o oldconsole.o history.o \
	 idt.o gdt.o isr.o icmp.o udpconsole.o stdio.o \
	 fcntl.o


MALFLAGS = $(CFLAGS) -DLACKS_SYS_TYPES_H -DLACKS_ERRNO_H \
	-DLACKS_STRING_H -DLACKS_SYS_MMAN_H -DLACKS_UNISTD_H \
	-DLACKS_SYS_PARAM_H

#malloc.o : malloc.c malloc.h
#	$(CC) -c $(MALFLAGS) $< -o $@


$(objects): %.o: %.cpp assem.h flash.h flashmap.h version.h
	$(GPP) -c $(CFLAGS) $< -o $@


main.bin: $(objects) assem.o interrupt.o main_head.S setup.i
	nasm -f elf -o main_head.o main_head.S
	$(LD) -o main.bin -Tmain_link.ld $(objects) assem.o \
	interrupt.o main_head.o

mac:
	../jtag/biffjtag mac

programconfig:
	../jtag/biffjtag programconfig


expanded.bin : main.bin mac.bin
	cp mac.bin expanded.bin
	cat main.bin >> expanded.bin


main.bin.z : expanded.bin deflate mac.bin
	./deflate expanded.bin main.bin.z


define lz_bin_compile
	$(CC) -c $(CFLAGS) tinflate.c
	nasm -f elf -o lz_head.o lz_head.S
	$(LD) -o lz.bin -Tlz_link.ld tinflate.o lz_head.o
endef


lz.bin: tinflate.c munge.c munge.h check_stub_size.py lz_head.S lz_link.ld setup.i \
			setup.h
	$(lz_bin_compile)
	./check_stub_size.py
	$(lz_bin_compile)


deflate: deflate.c munge.h munge.c
	gcc -o deflate deflate.c -lz


tinflate: tinflate.c munge.h munge.c
	gcc -o tinflate -DTINF_STANDALONE tinflate.c


see:
	hexdump -Cv /tmp/public/biffboot.bin | less


# Release of this version of the bootloader, including all tools
release:
	cp merge $(RELEASE)
	cp deflate $(RELEASE)
	cp lz.bin $(RELEASE)
	cp main.bin $(RELEASE)
	cp head.bin $(RELEASE)
		


clean:
	rm -f /tmp/public/biffboot.bin main.bin biffboot.bin \
	bios.bin head.bin main.elf *.o *~ main.bin.z deflate \
	lz.bin expanded.bin tinflate merge typescript f


spotless: clean
	rm -f setup.i setup.h version.h biffboot-*.bin DEADJOE
	