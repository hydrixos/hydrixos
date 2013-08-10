# HydrixOS

This repository contains the latest source code of HydrixOS. I've published it for anyone interested in low-level programming. 

*Please be careful:* I've started it as a pure hobbyist project when I was 16 years old and stopped it in 2006. *So please:* don't consider it as an example for high quality C code or well operating systems design ;).

## Licensing

All sources are published under the GNU General Public License 2.0 (you will find it at the file "copying") or the GNU Lesser General Public License 2.0 (you will find it at the file "copying.library"). The license used in the different source files is noted at the beginning of it.

HydrixOS is a project in an early alpha development state. We are just working on the basical system. Currently we have released a microkernel and some basical libraries. For more informations, visit our homepage:

http://www.hydrixos.org/

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the source code licenses for more details.


## Package contents

The package contains the HydrixOS kernel together with a small init application. The init application provides some demo code and a simple debugger allowing you to inspect running processes.

To give you a short overview on the package contents:

- `bin/`: contains the compiled sources and an executable disk image
- `doc/`: contains a full documentation of the kernel and the runtime library. It is written in German using LyX.
- `lib/`: contains the compiled runtime libraries
- `src/`: All source files
- `grub.con`: A GRUB configuration you may use to boot up HydrixOS from an ext file system

- `src/hymk/`: containt the HydrixOS microkernel
- `src/hyinit/`: contains the source of the HydrixOS init process started after booting the kernel
- `src/hybaselib/`: the C runtime library of HydrixOS
- `src/hycoredebuglib/`: parts of the debugger
- `src/libhylinux`: an incomplete implementation of the base APIs of HydrixOS on top of Linux

## Running the disk image

You may directly use the file `bin/disk144.img` to boot up HydrixOS in your favourite virtualization software.


## Building the sources

To build the sources you need at least GNU gcc 3.4.6, GNU binutils 2.16.1 and GNU make 3.80.

To delete all object files, just enter

`./cleanit`

To build the sources, just enter

`./makeits`

The script will automatically install the script on the disk image disk144.img.


## Configuring GRUB for booting from hard disk

If you have your HydrixOS directory stored at `(hd1,0)` you just have to write the following lines into the configuration file of GRUB "menu.lst":

`title=HydrixOS root (hd1,0) kernel --type=multiboot (hd1,0)/bin/hymk.bin module (hd1,0)/bin/hyinit.bin`

