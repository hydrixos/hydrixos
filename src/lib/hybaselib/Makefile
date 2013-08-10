#
# hyBaseLib Makefile
#
# by Friedrich Gräter
#

CC = gcc
AS = as
LD = ld
AR = ar

CINCLUDES = -I../../include

CCFLAGS = -c -finline-functions -nostdlib -nodefaultlibs -nostartfiles \
	  -nostdinc $(CINCLUDES) -std=gnu99 \
	  -fno-builtin -ffreestanding \
	  -fexec-charset=utf8 -finput-charset=iso-8859-15\
	  -Wall -Wextra -Wnested-externs -Wmissing-prototypes \
	  -Wcast-align -Wshadow -Wsign-compare -Wwrite-strings -Winline\
	  -fno-pack-struct -fno-strict-aliasing \
 	  -fno-zero-initialized-in-bss -O3 -m32
	  
	  #
	  # -fno-zero-initialized-in-bss
	  # 
	  # is needed for gcc 3.3 (later we will set it as the
	  # standard compiler for the kernel)
	  #

LDFLAGS = --oformat elf32-i386 -melf_i386 -m32 --print-map >./hybaselib.map
ASFLAGS = --32
ARFLAGS = rsv

OBJS = 	arch/x86/crt0.o 	arch/x86/syscall.o 		arch/x86/tls.o \
	arch/x86/mutex.o 	arch/x86/blthrd-arch.o\
	\
	libinit.o 	buffers.o 	region.o 	heap.o \
	memalloc.o	stack.o		blthrd.o	pmap.o \
	spxml.o
	
.c.o:
	$(CC) $(CCFLAGS) -o $@ $<

.s.o:
	$(AS) $(ASFLAGS) -o $@ $<


../../../lib/libhybase.a:	$(OBJS)
				$(AR) $(ARFLAGS) $@ $(OBJS)

../../../lib/crtbx86.o:		
				$(AS) $(ASFLAGS) arch/x86/crtbx86.s -o $@

../../../lib/crtnx86.o:		
				$(AS) $(ASFLAGS) arch/x86/crtnx86.s -o $@


#
# All directive
#
all:
	make ../../../lib/libhybase.a --no-print-directory 
	make ../../../lib/crtbx86.o --no-print-directory 
	make ../../../lib/crtnx86.o --no-print-directory 

cleanall:		
	rm -f ./*.o
	rm -f ./arch/x86/*.o

#############################################################
#
# hyBaseLib
#
#############################################################
arch/x86/crt0.o:		arch/x86/crt0.s
arch/x86/syscall.o:		arch/x86/syscall.c
arch/x86/tls.o:			arch/x86/tls.c
arch/x86/mutex.o:		arch/x86/mutex.c
arch/x86/blthrd-arch.o:		arch/x86/blthrd-arch.c

#
# Generic Code
#
libinit.o:			libinit.c
buffers.o:			buffers.c
region.o:			region.c
heap.o:				heap.c
memalloc.o:			memalloc.c
stack.o:			stack.c
blthrd.o:			blthrd.c
pmap.o:				pmap.c
spxml.o:			spxml.c
