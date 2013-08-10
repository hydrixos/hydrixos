#
# hyCoreDebug Makefile
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
	  -fno-builtin -ffreestanding\
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

LDFLAGS = --oformat elf32-i386 -melf_i386 -m32 --print-map >./hyvfslib.map
ASFLAGS = --32
ARFLAGS = rsv

OBJS = 	console.o	server.o	vsprintf.o	comm.o
	
.c.o:
	$(CC) $(CCFLAGS) -o $@ $<

.s.o:
	$(AS) $(ASFLAGS) -o $@ $<


../../../lib/libhycoredebug.a:	$(OBJS)
				$(AR) $(ARFLAGS) $@ $(OBJS)

#
# All directive
#
all:
	make ../../../lib/libhycoredebug.a --no-print-directory 

cleanall:		
	rm -f ./*.o

#############################################################
#
# hyCoreDebug
#
#############################################################
console.o:		console.c
comm.o:			comm.c
server.o:		server.c
vsprintf.o:		vsprintf.c
