 
 This Bootloader is copyright (c) 2014 Bifferos.com.

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this Bootloader to deal in the Bootloader without restriction, including
 the rights to copy, publish, distribute sublicense, and/or sell copies of the
 Bootloader and to permit persons to whom the Bootloader is furnished to do so,
 subject to the following conditions:

 THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHOR OR COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


 Intro

 Biffboot is an x86 bootloader used for booting RDC x86 chips from NOR
 flash.  There are now other bootloaders that are more featureful than this, so 
 this is kept here for reference.

 Features

  - Command-line interface with completion, and history
  - Capable of flashing over serial or ethernet 
  - Self decompressing to save flash space
  - Boots Linux or multiboot executables.

