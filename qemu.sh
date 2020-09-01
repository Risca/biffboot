#!/bin/sh

./add_kernel_to_loader.py ../buildroot/linux-2.6.27.5/arch/x86/boot/bzImage biffboot.bin

#./add_kernel_to_loader.py ../bb/kernel/initrd_fits_just.img ../bb/bios/biffboot.bin

qemu -cpu 486 -m 32 -nographic -usb -no-acpi -L ./ -serial stdio -hda /dev/zero

# to collect information.
#qemu -cpu 486 -m 32 -nographic -usb -no-acpi -serial stdio -hda /dev/zero
