#
# HydrixOS Mikrokernel Makefile
#
# by Friedrich Gräter
#

CC = gcc
AS = as
LD = ld

CINCLUDES = -I../include \
	    -I./include/x86 

CCFLAGS = -c -finline-functions -nostdlib -nodefaultlibs -nostartfiles \
	  -nostdinc $(CINCLUDES) -std=gnu99 \
	  -fno-builtin -ffreestanding \
	  -Wall -Wextra -Wnested-externs -Wmissing-prototypes \
	  -Wcast-align -Wshadow -Wsign-compare -Wwrite-strings -Winline \
	  -fno-pack-struct -fno-strict-aliasing \
 	  -fno-zero-initialized-in-bss -O3 -m32
	  
	  #
	  # -fno-zero-initialized-in-bss
	  # 
	  # is needed for gcc 3.3 (later we will set it as the
	  # standard compiler for the kernel)
	  #

LDFLAGS = -Ttext=0x100000 --oformat elf32-i386 --print-map -melf_i386 >kmap
ASFLAGS = --32


OBJS = 	x86/start.o	x86/init.o	x86/kprintf.o\
	x86/string.o	x86/modules.o	x86/meminit.o\
	x86/page.o	x86/alloc.o	x86/info.o\
	x86/tss.o	x86/frame.o	x86/irq.o\
	x86/intr.o	x86/subject.o	x86/schedule.o\
	x86/current.o	x86/sysc.o	x86/paged.o\
	x86/security.o	x86/map.o	x86/sync.o\
	x86/io.o	x86/remote.o	x86/timeout.o

.c.o:
	$(CC) $(CCFLAGS) -o $@ $<

.s.o:
	$(AS) $(ASFLAGS) -o $@ $<

#
# All directive
#
all:
	make ../../bin/hymk.bin --no-print-directory

#
# HKRNL directive
#
../../bin/hymk.bin:	$(OBJS)
			$(LD) $(LDFLAGS) -o ../../bin/hymk.bin $(OBJS)

#
# Cleanall directive
#
cleanall:
			rm -f ./*.o
			rm -f ./x86/*.o
			rm -f ./kernel/*.o


#############################################################
#
# Kernel modules
#
#############################################################
#
# i386
#
x86/alloc.o:		x86/alloc.c
x86/current.o:		x86/current.c
x86/frame.o:		x86/frame.c
x86/init.o:		x86/init.c
x86/info.o:		x86/info.c
x86/intr.o:		x86/intr.c
x86/io.o:		x86/io.c
x86/irq.o:		x86/irq.s
x86/kprintf.o:		x86/kprintf.c
x86/map.o:		x86/map.c
x86/modules.o:		x86/modules.c
x86/meminit.o:		x86/meminit.c
x86/page.o:		x86/page.c
x86/paged.o:		x86/paged.c
x86/process.o:		x86/process.c
x86/remote.o:		x86/remote.c
x86/schedule.o:		x86/schedule.c
x86/security.o:		x86/security.c
x86/start.o:		x86/start.s
x86/string.o:		x86/string.c
x86/subject.o:		x86/subject.c
x86/sync.o:		x86/sync.c
x86/sysc.o:		x86/sysc.s
x86/timeout.o:		x86/timeout.c
x86/tss.o:		x86/tss.c
