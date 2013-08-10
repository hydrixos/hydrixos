#
# HydrixOS Init Makefile
#
# by Friedrich Gr�ter
#

CC = gcc
CPP = g++
AS = as
LD = ld

CINCLUDES = -I../include

CCFLAGS = -c -finline-functions -nostdlib -nodefaultlibs -nostartfiles \
	  -nostdinc $(CINCLUDES) -std=gnu99 \
	  -fno-builtin -ffreestanding\
	  -fexec-charset=utf8 -finput-charset=iso-8859-15\
	  -Wall -Wextra -Wnested-externs -Wmissing-prototypes \
	  -Wcast-align -Wshadow -Wsign-compare -Wwrite-strings -Winline\
	  -fno-pack-struct -fno-strict-aliasing \
 	  -fno-zero-initialized-in-bss -O3 -m32

LDFLAGS = -Tinit.ld --oformat binary --print-map -melf_i386 >hyinit.map
LIBS = --library-path=./../../lib -lhybase -lhycoredebug
ASFLAGS = --32

CRT = ../../lib/crtbx86.o
OBJS = 	main.o			iprintf.o	startup.o	fork.o\
	coredbg/debugger.o	coredbg/console.o	coredbg/shell.o\
	coredbg/client.o	coredbg/trace.o		coredbg/variable.o\
	coredbg/tracecmd.o	coredbg/help.o		coredbg/analyze.o\
	coredbg/info.o		coredbg/cterm.o	

.cpp.o:
	$(CPP)	$(CPPFLAGS) -o $@ $<

.c.o:
	$(CC) $(CCFLAGS) -o $@ $<

.s.o:
	$(AS) $(ASFLAGS) -o $@ $<

#
# All directive
#
all:
	make ../../bin/hyinit.bin --no-print-directory

#
# HKRNL directive
#
../../bin/hyinit.bin:	$(OBJS)
			$(LD) $(LDFLAGS) -o ../../bin/hyinit.bin $(CRT) $(OBJS) $(LIBS)

#
# Cleanall directive
#
cleanall:
	rm -f ./*.o
	rm -f ./coredbg/*.o
	

#############################################################
#
# Init modules
#
#############################################################
#
# i386
#
fork.o:		fork.s
main.o:		main.c
iprintf.o:	iprintf.c
startup.o:	startup.c

#
# Debugger Modules
#
coredbg/debugger.o:	coredbg/debugger.c
coredbg/console.o:	coredbg/console.c
coredbg/shell.o:	coredbg/shell.c
coredbg/client.o:	coredbg/client.c
coredbg/trace.o:	coredbg/trace.c
coredbg/variable.o:	coredbg/variable.c
coredbg/tracecmd.o:	coredbg/tracecmd.c
coredbg/help.o:		coredbg/help.c
coredbg/analyze.o:	coredbg/analyze.c
coredbg/info.o:		coredbg/info.c
coredbg/cterm.o:	coredbg/cterm.c
