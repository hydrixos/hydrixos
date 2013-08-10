/*
 *
 * stdio.h
 *
 * (C)2001, 2002 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g.
 * in the file 'copying'). 
 *
 */
#ifndef _STDIO_H
#define _STDIO_H

#include <hydrixos/types.h>
#include <stdarg.h>

	/*
	 * The type printf loads from arglist
	 *
	 */
	#define PRINTF_CT	unsigned long

	/* This is a restricted implementation of the ISO-C 'vsnprintf' */
	int vsnprintf
	(
		char *buffer, 
		size_t sz,
		const char *format, 
		va_list arglist
	);
	
	/* kprintf is a restricted implementation of the ISO-C 'printf' */
	extern	int kprintf(const char *fm, ...);
	extern	void kclrscr(void);
	
	#define dprintf		kprintf
#endif


