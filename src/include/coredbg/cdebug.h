/*
 *
 * cdebug.h
 *
 * (C)2005 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library'). 
 *
 * Simple debugging libary
 *
 */ 
#ifndef _CDEBUG_H
#define _CDEBUG_H

#include <hydrixos/types.h>
#include <hydrixos/list.h>
#include <hydrixos/mutex.h>
#include <hydrixos/hymk.h>

/*
 * Basical debugging functions
 *
 */
typedef char *va_list;

#define va_start(ap,v)  ap = (va_list)&v + sizeof(v)
#define va_arg(ap,t)    ( (t *)(ap += sizeof(t)) )[-1]
#define va_end(ap)      ap = NULL


int vsnprintf(char *bf, size_t sz, const char *fm, va_list ap);
int snprintf(char* str, size_t sz, const char* fm, ...) __attribute__ ((noinline));

int dbglib_atoul(utf8_t *str, uint32_t *buf, int base);
int dbglib_atosl(utf8_t *str, int32_t *buf, int base);

/*
 * Client/Server communication
 *
 */
/* Establishes a connection to the debugger */
sid_t cdbg_connect(void);


/*
 * Debugger I/O
 *
 */
/* Client command list */
enum dbg_client_commands 
{
	DBG_COM_DISCONNECT = 0x1000,
	
	DBG_COM_ADD_BREAKPOINT = 0x1010,
	DBG_COM_DEL_BREAKPOINT = 0x1011,
	DBG_COM_INC_BREAKPOINT = 0x1012,
	DBG_COM_RESET_BREAKPOINT = 0x1013,
	DBG_COM_GET_BREAKPOINT_ADDRESS = 0x1014,
	DBG_COM_GET_BREAKPOINT_COUNTER = 0x1015,
	
	DBG_COM_SET_TRACEFLAGS = 0x1020,
	DBG_COM_GET_TRACEFLAGS = 0x1021,
	DBG_COM_SET_HALTFLAGS = 0x1022,
	DBG_COM_GET_HALTFLAGS = 0x1023,
	DBG_COM_HOOK_CLIENT = 0x1024,

	DBG_COM_INTERACTIVE_MODE = 0x1030,
	
	DBG_COM_PUTC = 0x1040,
	DBG_COM_PUTS = 0x1041,
	DBG_COM_GETC = 0x1042,
	DBG_COM_GETS = 0x1043,

	DBG_COM_SET_TERMINAL = 0x1050,
	DBG_COM_GET_TERMFLAGS = 0x1051,
	DBG_COM_SET_TERMFLAGS = 0x1052,
	DBG_COM_SET_TERMCOLOR = 0x1053
};

void dc_disconnect(void);

int dc_add_breakpoint(const utf8_t *name, uintptr_t adr);
int dc_del_breakpoint(uintptr_t adr);
int dc_inc_breakpoint(uintptr_t adr);
int dc_reset_breakpoint(uintptr_t adr);
uintptr_t dc_get_breakpoint_adr(const utf8_t *name);
int dc_get_breakpoint_counter(uintptr_t adr);

void dc_set_traceflags(uint32_t tflags);
uint32_t dc_get_traceflags(void);
void dc_set_traceflags(uint32_t tflags);
uint32_t dc_get_haltflags(void);
void dc_set_haltflags(uint32_t tflags);

int dc_hook_client(sid_t sid, int term);

void dc_interactive_mode(void);

void dc_putc(uint32_t c);
size_t dc_puts(const utf8_t *str, size_t len);
uint32_t dc_getc(void);
size_t dc_gets(size_t num, utf8_t *buf);

int dc_set_terminal(int term);
unsigned dc_get_termflags(void);		
void dc_set_termflags(unsigned flags);
void dc_set_termcolor(unsigned color);		



/*
 * Console functions
 *
 */
int dc_printf(const utf8_t *fm, ...) __attribute__ ((noinline));

#define DBGTERM_ECHO_OFF		1	/* No keyboard echo */
#define DBGTERM_PROMPT_ON		2	/* Protect the prompt before the keyboard input from removing by backspace */
#define DBGTERM_PROMPT_ECHO_ON		4	/* Echo prompt input even if ECHO_OFF is set */

enum dbg_term_colors {
	DBGCOL_BLACK = 0,
	DBGCOL_BLUE,
	DBGCOL_GREEN,
	DBGCOL_CYAN,
	DBGCOL_RED,
	DBGCOL_MAGENTA,
	DBGCOL_BROWN,
	DBGCOL_GREY,
	DBGCOL_LIGHTEN_GREY,
	DBGCOL_LIGHTEN_BLUE,
	DBGCOL_LIGHTEN_GREEN,
	DBGCOL_LIGHTEN_CYAN,
	DBGCOL_LIGHTEN_RED,
	DBGCOL_PURPLE,
	DBGCOL_YELLOW,
	DBGCOL_WHITE
};

#endif
