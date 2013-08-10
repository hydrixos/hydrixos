/*
 *
 * kcon.h
 *
 * (C)2001, 2002 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying'). 
 *
 * Hardware dependend Macros for simple console support.
 *
 */
#ifndef _KCON_H
#define _KCON_H

#include <hydrixos/types.h>
#include <hymk/x86-io.h>
#include <string.h>

/* Debugger: Test boot mode */
extern int kdebug_no_boot_mode;

/* Get the address of a charracter in the EGA video buffer */
#define	l__KCON_GET_ADDR(l___line,l___column)		\
			(((l___line *160) + (l___column * 2)) + 0xB8000)

/* Scroll one line */			
#define l__KCON_NEW_CONSOLE_LINE	\
		{\
			memcpy((void*)0xB8000, (void*)(0xB8000 + (80*2)), 80*23*2);\
			memcpy((void*)(0xB8000 + (80*23*2)), (void*)(0xB8000 + (80*26*2)), 80*2);\
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
							*((char *)l__KCON_GET_C) = 0x3E;\
							*((char *)l__KCON_GET_C + 1) = 0xC;\
							l__KCON_NEXT_CHAR;\
						});\
						*((char *)l__KCON_GET_C) = l__c;\
						*((char	*)l__KCON_GET_C+1) = 15;\
					})

/* Clear screen */					
#define l__KCON_CLEAR_SCREEN	\
({\
	unsigned int adr;\
	for(adr = 0xB8000; adr<0xB8000 + 80*25*2; adr+=2) \
		*((unsigned short *)adr) = 0x0700;\
	l__column = 0;\
	l__line = 0;\
	l__KCON_GOTXY;\
})


#endif
