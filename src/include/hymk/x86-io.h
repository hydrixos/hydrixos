/*
 *
 * hymk/x86-io.h
 *
 * (C)2004 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g.
 * in the file 'copying.library'. 
 *
 * Simple port I/O for x86 plattforms.
 *
 */
#ifndef _X86_IO_H
#define _X86_IO_H

#include <hydrixos/types.h>

/*
 * Output to x86 port
 *
 *  outb	Output one byte
 *  outw	Output two bytes
 *  outl	Output four bytes
 *
 *  ___v	Datas
 *  ___p	Port number
 *
 */
#define outb(___p, ___v) \
	__asm__ __volatile__("outb %%al,%%dx\n\t" :: "a" (___v), "d" (___p))
	
#define outw(___p, ___v) \
	__asm__ __volatile__("outw %%ax,%%dx\n\t" :: "a" (___v), "d" (___p))

#define outl(___p, ___v) \
	__asm__ __volatile__("outl %%eax,%%dx\n\t" :: "a" (___v), "d" (___p))

/*
 * Read from x86 port
 *
 *  inb		Read one byte
 *  inw		Read two bytes
 *  inl		Read four bytes
 *
 *  ___p	Port number
 *
 * Returns readed datas.
 *
 */	
#define inb(___p) \
({\
	uint8_t ___v;\
	\
	__asm __volatile__\
		("inb %%dx,%%al\n\t" : "=a" (___v) : "d" (___p)); \
\
	___v;\
})

#define inw(___p) ({\
	uint16_t ___v;\
	\
	__asm __volatile__\
		("inw %%dx,%%ax\n\t" : "=a" (___v) : "d" (___p)); \
\
	___v;\
})

#define inl(___p) \
({\
	uint32_t ___v;\
	\
	__asm __volatile__\
		("inl %%dx,%%eax\n\t" : "=a" (___v) : "d" (___p)); \
\
	___v;\
})

/*
 * Read x86 time stamp counter (64-bit value; only available to Pentium or
 * later)
 *
 * ___l		variable that should save the lowest 32-bit of the counter
 * ___h		variable that should save the highest 32-bit of the counter
 *
 */
#define rdtsc(___l, ___h) \
({\
	\
	__asm __volatile__\
		("rdtsc\n\t" : "=a" (___l),  "=d" (___h):: "memory"); \
\
})

	
#endif


