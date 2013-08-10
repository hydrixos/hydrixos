/*
 *
 * cterm.c
 *
 * (C)2007 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying').   
 *
 * Client operation terminal
 *
 */
#include <hydrixos/types.h>
#include <hydrixos/tls.h>
#include <hydrixos/errno.h>
#include <hydrixos/blthr.h>
#include <hydrixos/mem.h>
#include <hydrixos/stdfun.h>
#include <hydrixos/system.h>

#include <hydrixos/pmap.h>

#include "../hyinit.h"
#include "coredbg.h"

/*
 * dbg_change_terminal
 *
 * Changes the terminal of the shell of client "client".
 *
 * This function expects dbg_client_mtx to be locked and leaves it locked!
 *
 */
int dbg_change_terminal(dbg_client_t *client, int term)
{
	client->shell->terminal = term;
	
	return 0;
}

/*
 * dbg_execute_client
 *
 * Executes a client, which was stopped before.
 *
 * This function expects dbg_client_mtx to be locked and leaves it locked!
 *
 */
void dbg_execute_client(dbg_client_t *client)
{
	dbg_shell_t *l__shell = *dbg_tls_shellptr;
	int l__softint = -1;
	int l__halt, l__trace;
		
	while (1)
	{
	       /*
	 	* Get its halt and trace flags
	 	*
	 	*/
		l__halt = client->halt_flags;
		l__trace = client->trace_flags;
		
		
		/* Set trace flags */
		if (    (((l__trace & DBG_TRACE_BREAKPOINT_COUNTER) || (l__halt & DBG_HALT_BREAKPOINTS)) && (client->breakpoints_n > 0))
                     || (l__trace & DBG_TRACE_MACHINE_EXECUTION) 
                     || (l__halt & DBG_HALT_SINGLE_STEP)
		   )
		{
			uint32_t l__eflags = dbg_get_eflags(client->client);
			l__eflags |= EFLAG_TRAP_FLAG;
			dbg_set_eflags(client->client, l__eflags);
		}

		/* Receive and handle software interrupts */
		l__softint = hymk_recv_softints(client->client, 0xFFFFFFFF, RECV_AWAKE_OTHER|RECV_TRACE_SYSCALL);

		int l__i = dbg_handle_event(client, l__softint);
		
		if (l__i == 0) continue;
		
		if (l__i > 0)
		{
			dbg_set_termcolor(l__shell->terminal, DBGCOL_YELLOW);
			dbg_iprintf(l__shell->terminal, "Control back from 0x%X.\n", client->client);
			dbg_set_termcolor(l__shell->terminal, DBGCOL_GREY);
			break;
		}
		
		if (l__i < 0)
		{
			dbg_set_termcolor(l__shell->terminal, DBGCOL_YELLOW);
			dbg_iprintf(l__shell->terminal, "Error during execution of 0x%X.\n", client->client);
			dbg_set_termcolor(l__shell->terminal, DBGCOL_GREY);
			break;
		}		
	}
		
	/* Delete trace flags */
	uint32_t l__eflags = dbg_get_eflags(client->client);
	l__eflags &= (~EFLAG_TRAP_FLAG);
	dbg_set_eflags(client->client, l__eflags);
		
		
	return;
}

/*
 * dbg_disconnect_client
 *
 * Disconnects a client
 *
 */
void dbg_disconnect_client(sid_t client)
{
	dbg_destroy_client(client);
	
	return;
}


/*
 * dbg_handle_event(client, nr)
 *
 * Handles an incomming software interrupt event "nr" 
 * from the client "client".
 *
 * This function expects dbg_client_mtx to be locked and leaves it locked!
 *
 *
 * Return value:
 *	 1	Interrupt execution and start shell
 *	 0	Recover execution
 *	-1	Invalid event
 */
int dbg_handle_event(dbg_client_t *client, int nr)
{
	dbg_shell_t *l__shell = *dbg_tls_shellptr;
	utf8_t *l__anl = NULL;
	int l__retval = 0;	
	int l__halt, l__trace;
	dbg_registers_t l__regs = dbg_get_registers(client->client);
	if (*tls_errno) 
	{
		dbg_isprintf("\nCan't read the register content of 0x%X in dbg_handle_event, because of %i!\n", client->client, *tls_errno);
		return -1;
	}
	
	/*
	 * Get its halt and trace flags
	 *
	 */
	l__halt = client->halt_flags;
	l__trace = client->trace_flags;

	/*
	 * Find the right handler
	 *
	 */
	/* Exceptions */
	if (nr <= 0x1F) 
	{
		/* Test for Breakpoints or single-step */
		if (nr == EXC_X86_DEBUG_EXCEPTION)
		{
			dbg_breaks_t l__brbuf;
			int l__isbr;

			/* If breakpoint tracing, increment the breakpoint counter */
			if ((l__trace & DBG_TRACE_BREAKPOINT_COUNTER) || (l__halt & DBG_HALT_BREAKPOINTS))
			{
				l__isbr = dbg_inc_breakpoint_ctr(client, l__regs.eip, &l__brbuf);
			
				if ((l__halt & DBG_HALT_BREAKPOINTS) && (l__isbr == 0))
				{
					dbg_set_termcolor(l__shell->terminal, DBGCOL_WHITE);
					dbg_iprintf(l__shell->terminal, "\nBREAKPOINT 0x%X (%s) VISITED %i TIMES. HALT.\n", l__regs.eip, l__brbuf.name, l__brbuf.count);
					dbg_set_termcolor(l__shell->terminal, DBGCOL_GREY);
					l__retval = 1;
				}
			}			
			
			/* If trace of machine execution */
			if ((l__trace & DBG_TRACE_MACHINE_EXECUTION) || (l__halt & DBG_HALT_SINGLE_STEP))
			{
				dbg_set_termcolor(l__shell->terminal, DBGCOL_WHITE);
				dbg_iprintf(l__shell->terminal, "\n[EIP 0x%X | ESP 0x%X]\n", l__regs.eip, l__regs.esp);
				dbg_set_termcolor(l__shell->terminal, DBGCOL_GREY);
				
				/* Stop if single step execution */
				if (l__halt & DBG_HALT_SINGLE_STEP)
				{
					l__retval = 1;
				}
			}
		}		
		 else
		{
			l__anl = dbg_analyze_softint(nr, client->client);
			
			dbg_set_termcolor(l__shell->terminal, DBGCOL_WHITE);
			dbg_iprintf(l__shell->terminal, "\n\n-- EXCEPTION CATCHED BY DEBUGGER FOR 0x%X --\n%s\n\n", client->client, l__anl);
			dbg_set_termcolor(l__shell->terminal, DBGCOL_GREY);
			
			l__retval = 1;
		}
	}
	/* Debugger calls */
	 else if (nr == 0xB0)
	{
		/* Halt after calling the command */
		if (l__halt & DBG_HALT_DEBUGGER_CALLS)
			l__retval = 1;		
		
		/* Trace the command */
		if ((l__trace & DBG_TRACE_DEBUGGER_CALLS) || (l__halt & DBG_HALT_DEBUGGER_CALLS))
		{
			l__anl = dbg_analyze_softint(nr, client->client);
			
			dbg_set_termcolor(l__shell->terminal, DBGCOL_WHITE);
			dbg_iprintf(l__shell->terminal, "\n[DBG 0x%X] %s\n\n", client->client, l__anl);
			dbg_set_termcolor(l__shell->terminal, DBGCOL_GREY);
		}
		
		int l__cwas = l__regs.eax;
		
		/* Execute the command */
		if (dbg_execute_client_command(client, &l__regs) == -1)
		{
			/* Error during client command: interrupt execution */
			dbg_set_termcolor(l__shell->terminal, DBGCOL_WHITE);
			dbg_iprintf(l__shell->terminal, "\nDebugger call 0x%X of 0x%X sent an error message (0x%X).\n", l__cwas, client->client, *tls_errno);
			dbg_set_termcolor(l__shell->terminal, DBGCOL_GREY);
			
			*tls_errno = 0;
			l__retval = 1;
		}
		 else
		{
			dbg_set_registers(client->client, l__regs);
		}
	}
	/* IRQs + Emptyints */
	 else if (((nr >= 0xA0) && (nr <= 0xAF)) || ((nr < 0xBF) || (nr > 0xD6)))
	{
		if ((l__trace & DBG_TRACE_OTHER_SOFTINTS) || (l__halt & DBG_HALT_OTHER_SOFTINTS))
		{
			l__anl = dbg_analyze_softint(nr, client->client);
			
			dbg_set_termcolor(l__shell->terminal, DBGCOL_WHITE);
			dbg_iprintf(l__shell->terminal, "\n[DBG 0x%X] %s\n\n", client->client, l__anl);
			dbg_set_termcolor(l__shell->terminal, DBGCOL_GREY);
			
			if (l__halt & DBG_HALT_OTHER_SOFTINTS)
			{
				l__retval = 1;
			}
		}
	}
	/* Syscalls */
	 else if ((nr >= 0xC0) && (nr <= 0xD6)) 
	{
		if ((l__trace & DBG_TRACE_SYSCALLS) || (l__halt & DBG_HALT_SYSCALLS))
		{
			l__anl = dbg_analyze_softint(nr, client->client);
						
			dbg_set_termcolor(l__shell->terminal, DBGCOL_WHITE);
			dbg_iprintf(l__shell->terminal, "\n[DBG 0x%X] %s\n\n", client->client, l__anl);
			dbg_set_termcolor(l__shell->terminal, DBGCOL_GREY);
							
			if (l__halt & DBG_HALT_SYSCALLS)
			{
				l__retval = 1;
			}
		}
	}
	 else
	{
		dbg_iprintf(l__shell->terminal, "\nUnkown debugger operation.\n\n", client->client, l__anl);
		l__retval = 1;
	}
	
	/*
	 * Exit
	 *
	 */
	if (l__anl != NULL) mem_free(l__anl);
	
	return l__retval;
}

/*
 * dbg_execute_client_command(client)
 *
 * Executes a debugger command which was called by "client".
 *
 * This function expects dbg_client_mtx to be locked and leaves it locked!
 *
 * Return value:
 *	0	Successfull
 *	-1	Error
 *
 */
int dbg_execute_client_command(dbg_client_t *client, dbg_registers_t *regs)
{
	dbg_shell_t *l__shell = *dbg_tls_shellptr;
	int l__retval = 0;
	
	/*
	 * dbg_execute_command executes a client command. The client
	 * can start a command by calling the empty software interrupt
	 * 0xB0. The command number (DBG_COM_*) has to be stored in
	 * EAX. The other parameters in the following registers.
	 *
	 * The tls_errno value will be written to EAX after executing
	 * the command. The return value will be stored to EBX.
	 *
	 */
	
	/* Witch command? */
	switch (regs->eax)
	{
		case (DBG_COM_DISCONNECT):
		{
			dbg_disconnect_client(client->client);
			break;
		}
		
		case (DBG_COM_ADD_BREAKPOINT):
		{
			utf8_t *l__buf = mem_alloc(32);
			if (l__buf == NULL)
			{
				l__retval = -1;
				break;
			}
			
			if (dbg_read(client->client, regs->ebx, l__buf, 32))
			{
				l__retval = -1;
				break;
			}
			
			l__buf[31] = 0;
			
			regs->ebx = dbg_add_breakpoint(client, l__buf, regs->ecx);

			mem_free(l__buf);

			break;
		}		
		
		case (DBG_COM_DEL_BREAKPOINT):
		{
			regs->ebx = dbg_del_breakpoint(client, regs->ebx);

			break;
		}		
		
		case (DBG_COM_INC_BREAKPOINT):
		{
			regs->ebx = dbg_inc_breakpoint_ctr(client, regs->ebx, NULL);

			break;
		}	
			
		case (DBG_COM_RESET_BREAKPOINT):
		{
			regs->ebx = dbg_reset_breakpoint_ctr(client, regs->ebx, NULL);

			break;
		}			
		
		case (DBG_COM_GET_BREAKPOINT_ADDRESS):
		{
			utf8_t *l__buf = mem_alloc(32);
			if (l__buf == NULL)
			{
				l__retval = -1;
				break;
			}
			
			if (dbg_read(client->client, regs->ebx, l__buf, 32))
			{
				l__retval = -1;
				break;
			}
			
			l__buf[31] = 0;
			
			regs->ebx = dbg_get_breakpoint_adr(client, l__buf);

			mem_free(l__buf);

			break;
		}		
				
		case (DBG_COM_GET_BREAKPOINT_COUNTER):
		{
			dbg_breaks_t l__buf;
			
			if (dbg_get_breakpoint_data(client, regs->ebx, &l__buf))
			{
				l__retval = -1;
				break;
			}

			regs->ebx = l__buf.count;

			break;
		}
							
		case (DBG_COM_SET_TRACEFLAGS):
		{
			dbg_set_traceflags(client, regs->ebx);

			break;
		}		
		
		case (DBG_COM_GET_TRACEFLAGS):
		{
			regs->ebx = dbg_get_traceflags(client);

			break;
		}		
		
		case (DBG_COM_SET_HALTFLAGS):
		{
			dbg_set_haltflags(client, regs->ebx);

			break;
		}		
		
		case (DBG_COM_GET_HALTFLAGS):
		{
			regs->ebx = dbg_get_haltflags(client);

			break;
		}
				
		case (DBG_COM_HOOK_CLIENT):
		{
			regs->ebx = dbg_hook_client(regs->ebx, regs->ecx);
			
			break;
		}
		
		case (DBG_COM_INTERACTIVE_MODE):
		{
			l__retval = 1;
			break;
		}
		
		case (DBG_COM_PUTC):
		{
			dbg_putc(l__shell->terminal, regs->ebx);
			break;
		}
		
		case (DBG_COM_PUTS):
		{
			utf8_t *l__buf = mem_alloc(regs->ecx);
			if (l__buf == NULL)
			{
				l__retval = -1;
				break;
			}

			if (dbg_read(client->client, regs->ebx, l__buf, regs->ecx))
			{
				l__retval = -1;
				mem_free(l__buf);
				break;
			}
			l__buf[regs->ecx - 1] = 0;
			
			dbg_puts(l__shell->terminal, l__buf);

			mem_free(l__buf);
			break;
		}
		
		case (DBG_COM_GETC):
		{
			regs->ebx = dbg_getc(l__shell->terminal);
			break;
		}
		
		case (DBG_COM_GETS):
		{
			utf8_t *l__buf = mem_alloc(regs->ebx);
			if (l__buf == NULL)
			{
				l__retval = -1;
				break;
			}
			
			dbg_gets(l__shell->terminal, regs->ebx, l__buf);
						
			if (dbg_write(client->client, regs->ecx, l__buf, regs->ebx))
			{
				l__retval = -1;
			}			

			mem_free(l__buf);
			
			break;
		}	
			
		case (DBG_COM_SET_TERMINAL):
		{
			regs->ebx = dbg_change_terminal(client, regs->ebx);
			break;
		}
		
		case (DBG_COM_GET_TERMFLAGS):
		{
			regs->ebx = dbg_get_termflags(l__shell->terminal);
			break;
		}
		
		case (DBG_COM_SET_TERMFLAGS):
		{
			dbg_set_termflags(l__shell->terminal, regs->ebx);
			break;
		}
				
		case (DBG_COM_SET_TERMCOLOR):
		{
			dbg_set_termcolor(l__shell->terminal, regs->ebx);
			break;
		}				
		
		default:
		{
			l__retval = -1;
			break;
		}
	}
	
	regs->eax = *tls_errno;
	*tls_errno = 0;
	
	return l__retval;
}
