/*
 *
 * iprintf.c
 *
 * (C)2001, 2002 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying'). 
 *
 * Functions used by kernel internal for simple 
 * console support during the initialization.
 *
 */
#include <hydrixos/types.h>
#include <hydrixos/stdfun.h>
#include <hydrixos/tls.h>
#include <hydrixos/errno.h>
#include <hydrixos/hymk.h>
#include <hymk/x86-io.h>
#include <hydrixos/pmap.h>

#include "hyinit.h"

typedef char *va_list;

#define va_start(ap,v)  ap = (va_list)&v + sizeof(v)
#define va_arg(ap,t)    ( (t *)(ap += sizeof(t)) )[-1]
#define va_end(ap)      ap = NULL

int vsnprintf(char *bf, size_t sz, const char *fm, va_list ap);

volatile uint8_t *i__screen = NULL;
int i__is_online = 0;

/* Get the address of a charracter in the EGA video buffer */
#define	l__KCON_GET_ADDR(l___line,l___column)		\
			(((l___line *160) + (l___column * 2)) + ((uintptr_t)i__screen))

/* Scroll one line */			
#define l__KCON_NEW_CONSOLE_LINE	\
		{\
			buf_copy((void*)((uintptr_t)i__screen), (void*)(((uintptr_t)i__screen) + (80*2)), 80*23*2);\
			buf_copy((void*)(\
				       ((uintptr_t)i__screen) + (80*23*2)), \
				       (void*)(((uintptr_t)i__screen) + (80*26*2)), 80*2\
				      );\
		}
		
/* Set the EGA cursor to the current charracter */		
#define l__KCON_GOTXY  \
({\
	unsigned long	l__loc_pos;\
	l__loc_pos = l__line * 80 + l__column;\
	outb(0x3D4, 0x0F);\
	outb(0x3D5, (unsigned char) l__loc_pos);\
	outb(0x3D4, 0x0E);\
	l__loc_pos >>= 8;\
	outb(0x3D5, (unsigned char) l__loc_pos);\
	l__loc_pos;\
})

/* Line feed & carrige return */
#define l__KCON_NEW_LINE \
({\
	l__line++;\
	l__column = 0;\
	if (l__line >= 24) {l__KCON_NEW_CONSOLE_LINE; l__line = 23;}\
	l__KCON_GOTXY;\
})

/* Set the cursor to the next char */
#define l__KCON_NEXT_CHAR \
({\
	l__column++;\
	if (l__column > 79) l__KCON_NEW_LINE;\
	l__KCON_GOTXY;\
})

/* Get the address of the current charracter */
#define l__KCON_GET_C	l__KCON_GET_ADDR(l__line, l__column)

/* Write a charracter to the screen and move the cursor to the next char */
#define l__KCON_CHAR_OUT(l__c)		({\
						if (l__column == 0) \
						({\
							*((char *)l__KCON_GET_C) = ':';\
							*((char *)l__KCON_GET_C + 1) = (char)0xB;\
							l__KCON_NEXT_CHAR;\
						});\
						*((char *)l__KCON_GET_C) = (char)l__c;\
						*((char	*)l__KCON_GET_C+1) = (char)14;\
					})

/* Clear screen */					
#define l__KCON_CLEAR_SCREEN	\
({\
	unsigned int adr;\
	for(adr = ((uintptr_t)i__screen); adr<((uintptr_t)i__screen) + 80*25*2; adr+=2) \
		*((unsigned short *)adr) = 0x0700;\
	l__column = 0;\
	l__line = 0;\
	l__KCON_GOTXY;\
})




volatile unsigned int l__line = 2;
volatile unsigned int l__column = 0;

/*
 * kgetcurpos()
 *
 * Loads the current cursor position to the line pointer
 * And resets the column pointer. This may be usefull, to
 * make user-mode Outputs still readable.
 *
 */
static void kgetcursorpos()
{
	uint8_t l__low = 0, l__hi = 0;
	long l__cp;
	
	outb(0x3D4, 0xE);
	l__hi = inb(0x3D5);
	
	outb(0x3D4, 0xF);
	l__low = inb(0x3D5);

	l__cp = l__low + (l__hi << 8);
	l__line = l__cp / 80;
	l__column = l__cp % 80;
		
	return;
}	


/*
 *
 * int kprintf(char *fm, ...);
 *
 * Output to console in kernel mode, without abstraction layer.
 * THIS FUNCTION SHOULD ONLY BE USED DURING INITIALIZATION OR
 * WHEN A PANIC APPEARS (or to debug some kernel code), because
 * it is not save to use this function !
 *
 */
int iprintf(const utf8_t* fm, ...) __attribute__ ((noinline));

int iprintf(const utf8_t* fm, ...)
{
	utf8_t		buf[1000];
	unsigned long	bufptr;
	int i;
	va_list args;

	va_start(args, fm);
	i = vsnprintf(buf, 1000, fm, args);
    	va_end(args);

	kgetcursorpos();

	for (bufptr=0; buf[bufptr]!=0; bufptr++)
	{
		switch (buf[bufptr])
		{
			case	'\n'	: l__KCON_NEW_LINE; break;
			default		: {
			 			l__KCON_CHAR_OUT(buf[bufptr]); 
						l__KCON_NEXT_CHAR;
						break;
	 				  }
	 	}
	}

	return i;
}


void kclrscr(void)
{
	l__KCON_CLEAR_SCREEN;
}

void init_iprintf(void)
{
	if (i__screen == NULL) i__screen = pmap_alloc(2 * 4096);
	
	hymk_io_alloc(0xB8000, (void*)i__screen, 2, IOMAP_READ | IOMAP_WRITE);
	i__is_online = 1;
}
