/*
 *
 * coredbg.h
 *
 * (C)2006 by Friedrich Gr�ter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying'). 
 *
 * Header of the core debugger
 *
 */
#ifndef _COREDBG_H
#define _COREDBG_H

#include <hydrixos/list.h>
#include <hydrixos/mutex.h>
#include <hydrixos/blthr.h>
#include <hydrixos/errno.h>
#include <coredbg/cdebug.h>

/* EFLAG flags */
#define EFLAG_CF		1
#define EFLAG_PF		4
#define EFLAG_AF		16
#define EFLAG_ZF		64
#define EFLAG_SF		128
#define EFLAG_TF		256
#define EFLAG_IF		512
#define EFLAG_DF		1024
#define EFLAG_OF		2048
#define EFLAG_IOPL		(4096 | 8192)
#define EFLAG_NT		16384
#define EFLAG_RF		65536
#define EFLAG_VM		131072
#define EFLAG_AC		262144
#define EFLAG_VIF		524288
#define EFLAG_VIP		1048576
#define EFLAG_ID		2097152

#define EFLAG_CARRY_FLAG			1
#define EFLAG_PARITY_FLAG			4
#define EFLAG_AUXILARY_FLAG			16
#define EFLAG_ZERO_FLAG				64
#define EFLAG_SIGN_FLAG				128
#define EFLAG_TRAP_FLAG				256
#define EFLAG_INTERRUPT_ENABLE_FLAG		512
#define EFLAG_DIRECTION_FLAG			1024
#define EFLAG_OVERFLOW_FLAG			2048
#define EFLAG_IO_PRIVILEGE_LEVEL		(4096 | 8192)
#define EFLAG_NESTED_TASK			16384
#define EFLAG_RESUME_FLAG			65536
#define EFLAG_VM_8086				131072
#define EFLAG_ALIGNMENT_CHECK			262144
#define EFLAG_VIRTUAL_INTERRUPT_FLAG		524288
#define EFLAG_VIRTUAL_INTERRUPT_PENDING		1048576
#define EFLAG_ID_FLAG				2097152

/*
 * =====================================================================================
 * 
 *				    Simple terminal driver
 *
 * =====================================================================================
 *
 */
/* Terminal structure */
typedef struct 
{
	int		keynum;			/* F-Key for window selection (2 - 12) */
	
	char		*buffer;		/* Window buffer (25 lines) */
	uint32_t	keybuf;			/* Keyboard buffer */
	
	char		*headline;		/* Headline of the terminal window */
	
	int		is_current;		/* Is it the current terminal? */
	mtx_t		is_reading;		/* Reading-Mutex */	
	mtx_t		is_writing;		/* Writing-Mutex */
	sid_t		reader;			/* Reading thread subject */	
	
	int		column;			/* Current column */
	int		line;			/* Current line */
	
	unsigned	flags;			/* DBGTERM_ Flags */
	int		prompt_col;		/* Current Prompt position */
	int		prompt_line;
	
	char		output_color;		/* Output color DBGCOL_ MACROS */
	
	list_t	ls;				/* Linked list of terminals */
}dbg_terminals_t;

extern mtx_t dbg_display_mutex;
extern thread_t* dbg_keyboard_thread;
extern dbg_terminals_t *dbg_terminals;
extern dbg_terminals_t *dbg_current_term;

int dbg_init_driver(void);

int dbg_create_window(const char* head);		/* Creates a window */
void dbg_destroy_window(int termid);			/* Destroys a window */

void dbg_switch_window(int termid);

void dbg_putc(int termid, uint32_t c);			/* Put a char on the terminal */
void dbg_puts(int termid, const utf8_t* str);		/* Put sthg on the terminal */
uint32_t dbg_getc(int termid);				/* Read one char from the keyboard */
size_t dbg_gets(int termid, size_t num, utf8_t* buf);	/* Read a line from the keyboard */

void dbg_set_headline(int termid, const utf8_t* head);	/* Set headline */

int dbg_iprintf(int termid, const utf8_t *fm, ...) __attribute__ ((noinline));	/* Printf (internal use) */
int dbg_isprintf(const utf8_t *fm, ...) __attribute__ ((noinline));		/* Printf on system terminal (internal use) */

unsigned dbg_get_termflags(int termid);			/* Read terminal flags (DBGTERM_*) */
void dbg_set_termflags(int termid, unsigned flags);	/* Set terminal flags (DBGTERM_*) */

void dbg_set_termcolor(int termid, unsigned color);	/* Set terminal color (DBGCOL_*) */

void dbg_pause(int term);				/* - Press any key to continue - */

int dbg_lock_terminal(int term, int stop);		/* Locks the terminal for exclusiv access */
void dbg_unlock_terminal(int term);			/* Unlocks the terminal for leaving the exclusiv access mode */

/*
 * =====================================================================================
 * 
 *				    Command Line Functions
 *
 * =====================================================================================
 *
 */
/* Command structure */
typedef struct 
{
	utf8_t		*cmd_name;		/* Command name */
	
	int (*execute_cmd)(void);		/* Command handler */
}dbg_command_t;

extern dbg_command_t *dbg_commands;	/* Array of commands */
extern unsigned dbg_commands_n;		/* Number of debug commands */

/* Register a debugger command */
dbg_command_t* dbg_find_command(const utf8_t *name);
int dbg_register_command(const utf8_t *name, int (*handler)(void));

/*** Shell table structure ***/
typedef struct 
{
	sid_t		client;			/* Client thread refering to this cmd-line */
	thread_t	*owner;			/* Debugger-Thread executing this shell */
	int		terminal;		/* Terminal number */
	
	utf8_t		*cmd_buffer;		/* Command buffer */
	utf8_t		*cmd;			/* Command name (after parsing the buffer) */
	utf8_t		**pars;			/* List of parameters (after parsing the buffer) */
	unsigned	n_pars;			/* Count of parameters (after parsing the buffer) */
	
	list_t	ls;				/* Shells are organized as a linked list */
}dbg_shell_t;
 
#define DBGSHELL_CMDBUFFER_SIZE		1024
 
/*** Shell implementation ***/
extern dbg_shell_t **dbg_tls_shellptr;	/* Pointer to the TLS entry of the shell datastructure */

void dbg_shell_thread(thread_t *thr);					/* Creates a shell thread */
int dbg_parse_cmd(void);						/* Parse a shell command */
int dbg_execute_cmd(void);						/* Executa a shell command */
void dbg_free_cmdbuf(void);						/* Free the command buffer */

int dbg_test_par(unsigned start, const utf8_t *val);			/* Searches a parameter value */
int dbg_par_to_uint(unsigned num, uint32_t *buf, int base);		/* Converts the parameter "num" to an unsigned int */
int dbg_par_to_int(unsigned num, int32_t *buf, int base);		/* Converts the parameter "num" to an unsigned int */
 
/*** Shell managment ***/
extern dbg_shell_t *dbg_shells;		/* Shells listing */
extern mtx_t dbg_shell_mutex;
 
dbg_shell_t* dbg_find_shell(int terminal, sid_t client);		/* Find a shell by its IDs */

dbg_shell_t* dbg_create_shell(int terminal, sid_t client);		/* Create a new shell */
void dbg_destroy_shell_cur(void);					/* Destroy current shell*/
void dbg_destroy_shell(int terminal, sid_t client);			/* Destroy a shell by its ID */

int dbg_find_free_terminal(void);					/* Finds a terminal that is not used by any shell */

/* Initialization */
void dbg_init_shell(void);						/* Initialize the shell subprogram */

/*
 * Shell variables 
 *
 */
typedef struct 
{
	utf8_t		name[32];		/* Name of the variable */
	utf8_t	 	value[32];		/* Value as string */
	
	list_t		ls;			/* It is organized as a linked list */
}dbg_variable_t;

extern dbg_variable_t *dbg_variables;
extern mtx_t dbg_variables_mtx;

int dbg_export(const utf8_t *name, const utf8_t *value);		/* Creates a variable or changes its value */
dbg_variable_t* dbg_get_variable(const utf8_t *name);			/* Returns a pointer to a variable buffer */
int dbg_get_value(const utf8_t *name, utf8_t *buf, size_t sz);		/* Returns the value of a variable to a buffer */

/*
 * =====================================================================================
 * 
 *				    Client interface
 *
 * =====================================================================================
 *
 */
/* Register informations */
typedef struct
{
	uint32_t	eax, ebx, ecx, edx;
	uint32_t	esp, ebp, esi, edi;
	uint32_t	eip, eflags;
}dbg_registers_t;
 
/* Softint informations */
typedef struct
{
	uint32_t	softint;		/* Number of the last called software interrupt */
	utf8_t		*name;			/* Name of that softint (hymk_*, exc_*, int_* etc.) */
	int		type;			/* Type of that softint (DBG_SOFTINT_*) */
	dbg_registers_t	regs;			/* Register state of the last softint */
}dbg_softint_t;

#define DBG_SOFTINT_EXCEPTION		0
#define DBG_SOFTINT_TRAP		1
#define DBG_SOFTINT_SYSCALL		2
#define DBG_SOFTINT_EMPTY		3

/* Breakpoint informations */
typedef struct 
{
	utf8_t		name[32];		/* Name of the breaking point */
	uintptr_t	address;		/* Its address */
	unsigned	count;			/* Count of executions on this break point */
	
	list_t		ls;			/* Breakpoints  */
}dbg_breaks_t;
 
/* Client information structure */
typedef struct 
{
	/* General informations */
	sid_t		client;			/* Client-thread connected to the debugger */
	dbg_shell_t	*shell;			/* Shell environment of the controlling the client */
	
	/* Debugging parameters */
	uint32_t	trace_flags;		/* Flags about the trace operations (DBG_TRACE_*) */
	uint32_t	halt_flags;		/* Flags about the halt operations (DBG_HALT_*) */
	
	int		hooked;			/* Client was hooked by the debugger */
	
	dbg_breaks_t	*breakpoints;		/* List of break points */
	unsigned	breakpoints_n;		/* Count of break points */

	list_t		ls;			/* It is organized as a linked list */
}dbg_client_t;
	
#define DBG_TRACE_IGNORE		0
#define DBG_TRACE_BREAKPOINT_COUNTER	1
#define DBG_TRACE_OTHER_SOFTINTS	2
#define DBG_TRACE_DEBUGGER_CALLS	4
#define DBG_TRACE_SYSCALLS		8
#define DBG_TRACE_MACHINE_EXECUTION	16

#define DBG_HALT_IGNORE			0
#define DBG_HALT_BREAKPOINTS		1
#define DBG_HALT_OTHER_SOFTINTS		2
#define DBG_HALT_SYSCALLS		4
#define DBG_HALT_DEBUGGER_CALLS		8
#define DBG_HALT_SINGLE_STEP		16

extern dbg_client_t *dbg_clients;
extern mtx_t dbg_clients_mtx;

/* Client managment operations */
dbg_client_t* dbg_find_client(sid_t client);		/* Find a client by its SID */

int dbg_create_client(sid_t client, int term);	/* Creates a client-structure and a worker thread */
void dbg_destroy_client(sid_t client);		/* Destroys a client-structure and a worker thread */

/* Debugging operations */
dbg_registers_t dbg_get_registers(sid_t client);			/* Get the registers of the current client */
void dbg_set_registers(sid_t client, dbg_registers_t reg);		/* Change the registers of the current client */

utf8_t* dbg_analyze_syscall(int sysc, sid_t client);			/* Displays a system call information of a client */
utf8_t* dbg_analyze_exception(int intr, sid_t client);			/* Displays an exception information  of a client */
utf8_t* dbg_analyze_softint(int intr, sid_t client);			/* Displays any software interrupt of a client */

uintptr_t dbg_get_eip(sid_t client);					/* Loads the current instruction pointer */
void dbg_set_eip(sid_t client, uintptr_t adr);				/* Sets the current instruction pointer */
uintptr_t dbg_get_esp(sid_t client);					/* Loads the current stack pointer */
void dbg_set_esp(sid_t client, uintptr_t adr);				/* Sets the current stack pointer */
uint32_t dbg_get_eflags(sid_t client);					/* Loads the current EFLAGS */
void dbg_set_eflags(sid_t client, uint32_t flags);			/* Sets the current EFLAGS */

int dbg_read(sid_t client, uintptr_t adr, void* buf, size_t len);		/* Read datas from the client's memory */
int dbg_write(sid_t client, uintptr_t adr, const void* buf, size_t len);	/* Write datas to the client's memory */

uint32_t dbg_read_stack(sid_t client, int level);			/* Reads a dword from the client's stack */

int dbg_set_traceflags(dbg_client_t *client, uint32_t flags);		/* Changes the trace flags (DBG_TRACE_*) */
uint32_t dbg_get_traceflags(dbg_client_t *client);			/* Sets the trace flags (DBG_TRACE_*) */

int dbg_set_haltflags(dbg_client_t *client, uint32_t flags);			/* Changes the trace flags (DBG_HALT_*) */
uint32_t dbg_get_haltflags(dbg_client_t *client);				/* Sets the trace flags (DBG_HALT_*) */

int dbg_add_breakpoint(dbg_client_t *client, const utf8_t *name, uintptr_t adr);	/* Adds a breakpoint */
int dbg_del_breakpoint(dbg_client_t *client, uintptr_t adr);				/* Deletes a breakpoint */
void dbg_free_breakpoint_list(dbg_client_t* client);					/* Remove all breakpoints */

int dbg_inc_breakpoint_ctr(dbg_client_t *client, uintptr_t adr, dbg_breaks_t *buf);	/* Increments the counter of a breakpoint */
int dbg_reset_breakpoint_ctr(dbg_client_t *client, uintptr_t adr, dbg_breaks_t *buf);	/* Resets the counter of a breakpoint */
int dbg_get_breakpoint_data(dbg_client_t *client, uintptr_t adr, dbg_breaks_t *buf);	/* Returns the information block of a breakpoint */
uintptr_t dbg_get_breakpoint_adr(dbg_client_t *client, const utf8_t *name);		/* Get the address of a break point by its name */

int dbg_hook_client(sid_t other, int term);				/* Create a new debugger for another thread */

int dbg_change_terminal(dbg_client_t *client, int term);			/* Change our output terminal */

/*
 * An Client operation will be pased through the interrupt 0xB0. The operation code will
 * be stored in the EAX register of the client.
 *
 */
/* Client execution control */
void dbg_execute_client(dbg_client_t *client);				/* Preforms the clients' execution */
void dbg_disconnect_client(sid_t client);				/* Disconnects a client */

int dbg_execute_client_command(dbg_client_t *client, dbg_registers_t *regs);	/* Executes a client command which was detected by dbg_handle_event */
int dbg_handle_event(dbg_client_t *client, int nr);			/* Handle an interrupt event */

/* Logon operations */
void dbg_logon_client(sid_t client);					/* Logs a client on to the debugger */
void dbg_logon_loop(void);						/* Waits for incoming logon requests (controller thread) */
	
/*
 * Shell commands
 *
 */
sid_t dbg_get_sidpar(const utf8_t *name, sid_t type);

/* Shell commands */
int dbg_sh_version(void);			/* Prints the current shell version */
int dbg_sh_help(void);				/* Prints a help screen */
int dbg_sh_dbgtest(void);			/* Simple testing program */
int dbg_sh_term(void);				/* Changes the current terminal */
int dbg_sh_export(void);			/* Sets a variable */
int dbg_sh_echo(void);				/* Echos a text */

/* Debugging commands */
int dbg_sh_start(void);				/* Starts the client thread */
int dbg_sh_hook(void);				/* Get the control over an uncontrolled thread */
int dbg_sh_exit(void);				/* Leave this debugger session */

/* Tracing commands */
int dbg_sh_getreg(void);			/* Get the content of one or more registers */
int dbg_sh_setreg(void);			/* Change the content of a register */

int dbg_sh_readd(void);				/* Read one or more dwords from memory */
int dbg_sh_writed(void);			/* Write one or more dwords to memory */
int dbg_sh_readb(void);				/* Read one or more bytes from memory */
int dbg_sh_writeb(void);			/* Write one or more bytes to memory */
int dbg_sh_writec(void);			/* Write one or more charracters to memory */

int dbg_sh_dump(void);				/* Dump the content of a client's stack */
int dbg_sh_dumpint(void);			/* Dumps the meaning of an interrupt */

int dbg_sh_trace(void);				/* Set/Get trace Flags */
int dbg_sh_halton(void);			/* Set/Get Halt-On Flags */

int dbg_sh_break(void);				/* Installs, Reads or deletes a breakpoint */

int dbg_sh_sysinfo(void);			/* Outputs system informations */
int dbg_sh_proc(void);				/* Outputs process informations */
int dbg_sh_thrd(void);				/* Outputs process informations */

#endif
