/*
 *
 * console.c
 *
 * (C)2006 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library').   
 *
 * Access to the console driver of the debugger
 *
 */
#include <hydrixos/types.h>
#include <hydrixos/tls.h>
#include <hydrixos/errno.h>
#include "./hycoredbg.h"
#include <coredbg/cdebug.h>

/*
 * dc_printf(fmt, ...)
 *
 * Simple "printf" implementation. Prints on the
 * debugger terminal.
 *
 */
int dc_printf(const utf8_t *fm, ...)
{
	utf8_t l__buf[1000];
	int l__i;
	va_list l__args;

	va_start(l__args, fm);
	l__i = vsnprintf(l__buf, 1000, fm, l__args);
    	va_end(l__args);
	
	if (l__i > 0) dc_puts(l__buf, l__i);
	
	return l__i;
}
