/*
 *
 * comm.c
 *
 * (C)2006 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library').   
 *
 * Communication with the debugger
 *
 */
#include <hydrixos/types.h>
#include <hydrixos/tls.h>
#include <hydrixos/errno.h>
#include "./hycoredbg.h"
#include <hydrixos/hymk.h>
#include <hydrixos/system.h>
#include <hydrixos/blthr.h>
#include <coredbg/cdebug.h>

/*
 * For those who are interested how the debugger calls are working:
 *
 * Our debugger is listening on the software interrupts of the
 * current thread since we connected to him with cdbg_connect.
 * The dc_*-functions will call the (unused) software interrupt
 * 0xB0. The debugger detects it and executes a function according
 * to the function number in the EAX register of the client.
 *
 * It is not the regular way your applications should communicate
 * with their servers. It could be a solution for emulation foreign
 * operating systems using an external thread.
 *
 */

/*
 * dc_disconnect
 *
 * Disconnects the client from the debugger
 *
 */
void dc_disconnect(void)
{
	__asm__ __volatile__("int $0xB0\n"
		     	     "addl $0, %%esp\n"
		     	     : "=a" (*tls_errno)
		     	     : "a" (DBG_COM_DISCONNECT)
		     	     : "memory"
		     	    );
}

/*
 * dc_add_breakpoint(name, adr)
 *
 * Adds a breakpoint for the current client with the name "name"
 * at the address "adr".
 *
 * Return value:
 *	   0	Success
 *	!= 0	Error
 * 
 */
int dc_add_breakpoint(const utf8_t *name, uintptr_t adr)
{
	int l__retval = -1;
	
	__asm__ __volatile__("int $0xB0\n"
		     	     "addl $0, %%esp\n"
		     	     : "=a" (*tls_errno),
		     	       "=b" (l__retval)
		     	     : "a" (DBG_COM_ADD_BREAKPOINT),
		     	       "b" (name),
		     	       "c" (adr)
		     	     : "memory"
		     	    );	
		     	    
	return l__retval;
}

/*
 * dc_del_breakpoint(adr)
 *
 * Removes the breakpoint at address "adr".
 *
 * Return value:
 *	   0	Success
 *	!= 0	Error
 * 
 */
int dc_del_breakpoint(uintptr_t adr)
{
	int l__retval = -1;
	
	__asm__ __volatile__("int $0xB0\n"
		     	     "addl $0, %%esp\n"
		     	     : "=a" (*tls_errno),
		     	       "=b" (l__retval)
		     	     : "a" (DBG_COM_DEL_BREAKPOINT),
		     	       "b" (adr)
		     	     : "memory"
		     	    );	
		     	    
	return l__retval;
}

/* 
 * dc_inc_breakpoint(adr)
 *
 * Increments the breakpoint counter of the breakpoint
 * at address "adr".
 *
 * Return value:
 *	   0 	Success
 *	!= 0	Error
 *
 */
int dc_inc_breakpoint(uintptr_t adr)
{
	int l__retval = -1;
	
	__asm__ __volatile__("int $0xB0\n"
		     	     "addl $0, %%esp\n"
		     	     : "=a" (*tls_errno),
		     	       "=b" (l__retval)
		     	     : "a" (DBG_COM_INC_BREAKPOINT),
		     	       "b" (adr)
		     	     : "memory"
		     	    );	
	
	return l__retval;
}

/*
 * dc_reset_breakpoint(adr)
 *
 * Resets the breakpoint counter of the breakpoint at
 * address "adr".
 *
 * Return value:
 *	   0	Success
 *	!= 0	Error
 *
 */
int dc_reset_breakpoint(uintptr_t adr)
{
	int l__retval = -1;
	
	__asm__ __volatile__("int $0xB0\n"
		     	     "addl $0, %%esp\n"
		     	     : "=a" (*tls_errno),
		     	       "=b" (l__retval)
		     	     : "a" (DBG_COM_RESET_BREAKPOINT),
		     	       "b" (adr)
		     	     : "memory"
		     	    );	
	
	return l__retval;
}

/*
 * dc_get_breakpoint_adr(name)
 *
 * Returns the address of a breakpoint.
 *
 */
uintptr_t dc_get_breakpoint_adr(const utf8_t *name)
{
	uintptr_t l__retval = 0;
	
	__asm__ __volatile__("int $0xB0\n"
		     	     "addl $0, %%esp\n"
		     	     : "=a" (*tls_errno),
		     	       "=b" (l__retval)
		     	     : "a" (DBG_COM_GET_BREAKPOINT_ADDRESS),
		     	       "b" (name)
		     	     : "memory"
		     	    );	
	
	return l__retval;
}

/*
 * dc_get_breakpoint_counter(adr)
 *
 * Returns the execution counter of a breakpoint
 *
 */
int dc_get_breakpoint_counter(uintptr_t adr)
{
	int l__retval = -1;
	
	__asm__ __volatile__("int $0xB0\n"
		     	     "addl $0, %%esp\n"
		     	     : "=a" (*tls_errno),
		     	       "=b" (l__retval)
		     	     : "a" (DBG_COM_GET_BREAKPOINT_COUNTER),
		     	       "b" (adr)
		     	     : "memory"
		     	    );	
	
	return l__retval;
}

/*
 * dc_set_traceflags(tflags)
 *
 * Change the trace flags to "tflags".
 *
 */
void dc_set_traceflags(uint32_t tflags)
{
	__asm__ __volatile__("int $0xB0\n"
		     	     "addl $0, %%esp\n"
		     	     : "=a" (*tls_errno)
		     	     : "a" (DBG_COM_SET_TRACEFLAGS),
		     	       "b" (tflags)
		     	     : "memory"
		     	    );	
	
	return;
}

/*
 * dc_get_traceflags()
 *
 * Returns the trace flags of this thread.
 *
 */
uint32_t dc_get_traceflags(void)
{
	uint32_t l__retval = 0;
	
	__asm__ __volatile__("int $0xB0\n"
		     	     "addl $0, %%esp\n"
		     	     : "=a" (*tls_errno),
		     	       "=b" (l__retval)
		     	     : "a" (DBG_COM_GET_TRACEFLAGS)
		     	     : "memory"
		     	    );	
	
	return l__retval;
}

/*
 * dc_set_haltflags(tflags)
 *
 * Changes the halt flags of this thread to "tflags".
 *
 */
void dc_set_haltflags(uint32_t tflags)
{
	__asm__ __volatile__("int $0xB0\n"
		     	     "addl $0, %%esp\n"
		     	     : "=a" (*tls_errno)
		     	     : "a" (DBG_COM_SET_HALTFLAGS),
		     	       "b" (tflags)
		     	     : "memory"
		     	    );	
	
	return;
}

/*
 * dc_get_haltflags()
 *
 * Returns the halt flags of this thread.
 *
 */
uint32_t dc_get_haltflags(void)
{
	uint32_t l__retval = 0;
	
	__asm__ __volatile__("int $0xB0\n"
		     	     "addl $0, %%esp\n"
		     	     : "=a" (*tls_errno),
		     	       "=b" (l__retval)
		     	     : "a" (DBG_COM_GET_HALTFLAGS)
		     	     : "memory"
		     	    );	
	
	return l__retval;
}

/*
 * dc_hook_client(sid, term)
 *
 * The debugger should get the controll over the thread
 * "sid" and display an interactive shell on the 
 * terminal with number "term".
 *
 * Return value:
 *	0	Success
 *	=! 0	Error
 */
int dc_hook_client(sid_t sid, int term)
{
	uint32_t l__retval = 0xFFFFFFFF;
	
	__asm__ __volatile__("int $0xB0\n"
		     	     "addl $0, %%esp\n"
		     	     : "=a" (*tls_errno),
		     	       "=b" (l__retval)
		     	     : "a" (DBG_COM_HOOK_CLIENT),
		     	       "b" (sid),
		     	       "c" (term)
		     	     : "memory"
		     	    );	
	
	return l__retval;
}

/*
 * dc_interactive_mode
 *
 * Interrupts the execution and displays the debugger
 * shell.
 *
 */
void dc_interactive_mode(void)
{
	__asm__ __volatile__("int $0xB0\n"
		     	     "addl $0, %%esp\n"
		     	     : "=a" (*tls_errno)
		     	     : "a" (DBG_COM_INTERACTIVE_MODE)
		     	     : "memory"
		     	    );	
	
	return;
}

/*
 * dc_putc(c)
 *
 * Writes an charracter "c" to the terminal of
 * this thread.
 *
 */
void dc_putc(uint32_t c)
{
	__asm__ __volatile__("int $0xB0\n"
		     	     "addl $0, %%esp\n"
		     	     : "=a" (*tls_errno)
		     	     : "a" (DBG_COM_PUTC),
		     	       "b" (c)
		     	     : "memory"
		     	    );	
	
	return;
}

/*
 * dc_puts
 *
 * Writes an string "str" to the terminal of
 * this thread. The string has the length of
 * "len" bytes.
 *
 */
size_t dc_puts(const utf8_t *str, size_t len)
{
	size_t l__retval = 0;
	
	__asm__ __volatile__("int $0xB0\n"
		     	     "addl $0, %%esp\n"
		     	     : "=a" (*tls_errno),
		     	       "=b" (l__retval)
		     	     : "a" (DBG_COM_PUTS),
		     	       "b" ((uintptr_t)(const void*)str),
		     	       "c" (len)
		     	     : "memory"
		     	    );	
	
	return l__retval;
}

/*
 * dc_getc
 *
 * Reads an charrecter from the current terminal.
 *
 */
uint32_t dc_getc(void)
{
	uint32_t l__retval;
	
	__asm__ __volatile__("int $0xB0\n"
		     	     "addl $0, %%esp\n"
		     	     : "=a" (*tls_errno),
		     	       "=b" (l__retval)
		     	     : "a" (DBG_COM_GETC)
		     	     : "memory"
		     	    );	
	
	return l__retval;
}

/*
 * dc_gets
 *
 * Reads an entire string from the current terminal.
 *
 * Return value:
 *	The count of bytes read.
 *
 */
size_t dc_gets(size_t num, utf8_t *buf)
{
	size_t l__retval;
	
	__asm__ __volatile__("int $0xB0\n"
		     	     "addl $0, %%esp\n"
		     	     : "=a" (*tls_errno),
		     	       "=b" (l__retval)
		     	     : "a" (DBG_COM_GETS),
		     	       "b" (num),
		     	       "c" (buf)
		     	     : "memory"
		     	    );	
	
	return l__retval;
}

/*
 * dc_set_terminal(term)
 *
 * Changes the current terminal to "term".
 *
 * Return value:
 *	0	Success
 *	!= 0	Error
 *
 */
int dc_set_terminal(int term)
{
	int l__retval;
	
	__asm__ __volatile__("int $0xB0\n"
		     	     "addl $0, %%esp\n"
		     	     : "=a" (*tls_errno),
		     	       "=b" (l__retval)
		     	     : "a" (DBG_COM_SET_TERMINAL),
		     	       "b" (term)
		     	     : "memory"
		     	    );	
	
	return l__retval;
}

/*
 * dc_get_termflags()
 *
 * Returns the terminal flags of the current terminal
 *
 */
unsigned dc_get_termflags(void)
{
	int l__retval;
	
	__asm__ __volatile__("int $0xB0\n"
		     	     "addl $0, %%esp\n"
		     	     : "=a" (*tls_errno),
		     	       "=b" (l__retval)
		     	     : "a" (DBG_COM_GET_TERMFLAGS)
		     	     : "memory"
		     	    );	
	
	return l__retval;
}	
	
/*
 * dc_set_termflags(flags)
 *
 * Changes the terminal flags of the current terminal
 * to "flags"
 *
 */
void dc_set_termflags(unsigned flags)
{
	__asm__ __volatile__("int $0xB0\n"
		     	     "addl $0, %%esp\n"
		     	     : "=a" (*tls_errno)
		     	     : "a" (DBG_COM_SET_TERMFLAGS),
		     	       "b" (flags)
		     	     : "memory"
		     	    );	
}


/*
 * dc_set_termcolor(color)
 *
 * Changes the terminal color of the current terminal
 * to "color" (DBGCOL_* constants)
 *
 */
void dc_set_termcolor(unsigned color)
{
	__asm__ __volatile__("int $0xB0\n"
		     	     "addl $0, %%esp\n"
		     	     : "=a" (*tls_errno)
		     	     : "a" (DBG_COM_SET_TERMCOLOR),
		     	       "b" (color)
		     	     : "memory"
		     	    );	
}		

	
