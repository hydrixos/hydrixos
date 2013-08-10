#
# LibHyLinux Makefile
#

CC = gcc
AS = as
LD = ld
AR = ar

CINCLUDE = -I../../include -I/usr/include

CCFLAGS = -c -nostdlib -nodefaultlibs -nostartfiles \
	  -nostdinc $(CINCLUDE) -fpack-struct \
	  -std=gnu99  -Wall \
	  -fno-strict-aliasing -fno-pack-struct \
	  -finline-functions -iwithprefix include \
 	  -fno-zero-initialized-in-bss

LDFLAGS = 
ASFLAGS =
ARFLAGS = rsv


OBJS = 	./hybaselib.o 


.c.o:
	$(CC) $(CCFLAGS) -o $@ $<

.s.o:
	$(AS) $(ASFLAGS) -o $@ $<

./libhylinux.a:		$(OBJS)
			$(AR) $(ARFLAGS) $@ $(OBJS)

all:
	./libhylinux.a

cleanall:		
			rm -f ./*.o

#############################################################
#
# Library modules
#
#############################################################

#
# Modules
#
./hybaselib.o:		./hybaselib.c
./hyvfslib.o:		./hyvfslib.c
