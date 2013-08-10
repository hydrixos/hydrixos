/*
 *
 * shell.c
 *
 * (C)2006 by Friedrich Gr�ter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying').   
 *
 * Debugger shell
 *
 */
#include <hydrixos/types.h>
#include <hydrixos/tls.h>
#include <hydrixos/errno.h>
#include <hydrixos/blthr.h>
#include <hydrixos/mem.h>
#include <hydrixos/stdfun.h>
#include <hydrixos/system.h>

#include <coredbg/cdebug.h>


#include "../hyinit.h"
#include "coredbg.h"

/* Command managment */
dbg_command_t *dbg_commands = NULL;
unsigned dbg_commands_n = 0;	

/* Shell managment */
dbg_shell_t *dbg_shells = NULL;
mtx_t dbg_shell_mutex = MTX_DEFINE();

/* Shell implementation */
dbg_shell_t **dbg_tls_shellptr = NULL;

/*
 * dbg_init_shell
 *
 * Initializes the shell
 *
 */
void dbg_init_shell(void)
{
	/* Initialize shell handling */
	dbg_tls_shellptr = (dbg_shell_t**)tls_global_alloc();
	
	/* Register shell commands */
	dbg_register_command("version", dbg_sh_version);
	dbg_register_command("help", dbg_sh_help);
	dbg_register_command("dbgtest", dbg_sh_dbgtest);

	dbg_register_command("term", dbg_sh_term);
	dbg_register_command("export", dbg_sh_export);
	dbg_register_command("echo", dbg_sh_echo);
	
	dbg_register_command("start", dbg_sh_start);
	dbg_register_command("hook", dbg_sh_hook);
	dbg_register_command("exit", dbg_sh_exit);
	
	dbg_register_command("getreg", dbg_sh_getreg);
	dbg_register_command("setreg", dbg_sh_setreg);
	
	dbg_register_command("readd", dbg_sh_readd);
	dbg_register_command("readb", dbg_sh_readb);
	dbg_register_command("writed", dbg_sh_writed);
	dbg_register_command("writeb", dbg_sh_writeb);
	dbg_register_command("writec", dbg_sh_writec);
	
	dbg_register_command("dump", dbg_sh_dump);
	dbg_register_command("dumpint", dbg_sh_dumpint);
	
	dbg_register_command("trace", dbg_sh_trace);
	dbg_register_command("halton", dbg_sh_halton);
	
	dbg_register_command("break", dbg_sh_break);
	
	dbg_register_command("sysinfo", dbg_sh_sysinfo);
	dbg_register_command("proc", dbg_sh_proc);
	dbg_register_command("thrd", dbg_sh_thrd);
}

/*
 * dbg_find_command(name)
 *
 * Finds a command within the command list by its name.
 *
 */
dbg_command_t* dbg_find_command(const utf8_t* name)
{
	int l__i = dbg_commands_n;
	
	while (l__i --)
	{
		if (!str_compare(dbg_commands[l__i].cmd_name, name, 32))
		{
			return &dbg_commands[l__i];
		}
	}
	
	return NULL;
}

/*
 * dbg_register_command(nr, name, handler)
 *
 * Installs a debuger command. The command has the
 * command id "cmd_nr" and the name "name". It will
 * be handled by the handler "handler". 
 *
 * 	handler(void)
 *
 *	Is a handler function for a debugger shell command.
 *	The command should be executed in the environment
 *	of the current shell thread.
 *
 * Return values:
 *	=0	Successfull
 *	<0	Error
 *
 */ 
int dbg_register_command(const utf8_t *name, int (*handler)(void))
{
	/* Is this command already registered? */
	if (dbg_find_command(name) != NULL)
	{
		dbg_isprintf("Debugger: Implementation error. Command %s is already registered.\n", name);
		return -1;
	}
	
	/* Allocate a buffer for the command name */
	utf8_t *l__name = mem_alloc(str_len(name, 32) + 1);
	if (l__name == NULL)
	{
		dbg_isprintf("Debugger: Can't allocate command name for %s, because of 0x%X.\n", name, *tls_errno);
		return -1;
	}
		
	/* Resize command table */
	dbg_command_t *l__ptr = mem_realloc(dbg_commands, sizeof(dbg_command_t) * (dbg_commands_n + 1));
	if (l__ptr == NULL) 
	{
		mem_free(l__name);
		dbg_isprintf("Debugger: Implementation error. Can't install command %s, because of 0x%X.\n", name, *tls_errno);
		return -1;
	}
		
	/* Reset command table pointer */
	dbg_commands = l__ptr;
	
	/* Add command */
	dbg_commands[dbg_commands_n].cmd_name = l__name;
	dbg_commands[dbg_commands_n].execute_cmd = handler;
	
	/* Copy name */
	str_copy(l__name, name, 32);
	
	/* Now we have more commands */
	dbg_commands_n ++;
	
	return 0;	
}

/*
 * dbg_find_shell(term, client)
 *
 * Returns the shell structure of the shell, that own terminal
 * "term" and is currently used by the Client "client".
 *
 * The function leaves the Mutex dbg_shell_mutex locked, if 
 * it was successful.
 *
 * Return value:
 *	Pointer to the shell structure
 *	NULL, if error
 *
 */
dbg_shell_t* dbg_find_shell(int terminal, sid_t client)
{
	dbg_shell_t *l__shell = NULL;
	
	mtx_lock(&dbg_shell_mutex, -1);
	
	l__shell = dbg_shells;
	
	/* Find the shell */
	while(l__shell != NULL)
	{
		/* Is it the right shell. If yes, return the structure and leave dbg_shell_mutex closed */
		if ((l__shell->terminal == terminal) && (l__shell->client == client))
			return l__shell;
			
		l__shell = l__shell->ls.n;
	}
	
	/* No shell found, return NULL and open dbg_shell_mutex */
	mtx_unlock(&dbg_shell_mutex);
	return NULL;
}

/*
 * dbg_create_shell
 *
 * Creates a new shell. This function creates:
 *		- a new shell structure
 *		- a new shell thread
 *
 */
dbg_shell_t* dbg_create_shell(int terminal, sid_t client)
{
	dbg_shell_t *l__shell = NULL;
	
	/* Found an existing shell for it? */
	if ((l__shell = dbg_find_shell(terminal, client)) != NULL)
	{
		mtx_unlock(&dbg_shell_mutex);
		return l__shell;
	}
	
	/* Create the structure */
	l__shell = mem_alloc(sizeof(dbg_shell_t));
	if (l__shell == NULL)
	{
		dbg_isprintf("Can't create a shell structure for %i %i, because of %i\n", terminal, client, *tls_errno);
	}
	
	l__shell->terminal = terminal;
	l__shell->client = client;
	l__shell->cmd_buffer = NULL;
	l__shell->cmd = NULL;
	l__shell->pars = NULL;
	l__shell->n_pars = 0;
	
	l__shell->owner = blthr_create(&dbg_shell_thread, 65536);
	if (l__shell->owner == NULL)
	{
		dbg_isprintf("Can't create a shell thread for %i %i, because of %i.\n", terminal, client, *tls_errno);
		mem_free(l__shell);
	}

	mtx_lock(&dbg_shell_mutex, -1);
	lst_add(dbg_shells, l__shell);
	
	/* Start the thread */
	blthr_awake(l__shell->owner);
	
	mtx_unlock(&dbg_shell_mutex);
	
	return l__shell;	
}

/*
 * dbg_destroy_shell_cur()
 *
 * Destroys the shell which is used by the current
 * thread. This function will also terminate the current
 * thread.
 *
 */
void dbg_destroy_shell_cur(void)
{
	dbg_shell_t *l__shell = (*dbg_tls_shellptr);
	
	/* Free the memory of the shell structures */
	if (l__shell->cmd_buffer != NULL) mem_free(l__shell->cmd_buffer);
	if (l__shell->pars != NULL)
	{
		int l__i = l__shell->n_pars;
		
		while(l__i --) 
			if (l__shell->pars[l__i] != NULL) mem_free(l__shell->pars[l__i]);
	}
	mem_free(l__shell->pars);
	lst_dellst(dbg_shells, l__shell);
	mem_free(l__shell);
	
	/* Finish us */
	blthr_finish();
}

/*
 * dbg_find_free_terminal()
 *
 * Finds a free terminal which is not used by any shell.
 *
 * Return value:
 *	1	No free terminal found (defaulted to terminal 1)
 *	>1	Terminal number
 *
 * NOTE:	There is no warranty, that the terminal got used
 *		by another thread in between. So please be carefull
 *		using this function!
 */ 
int dbg_find_free_terminal(void)
{
	dbg_shell_t *l__shell = NULL;
	int l__i;

	mtx_lock(&dbg_shell_mutex, -1);

	for (l__i = 2; l__i < 12; l__i ++)
	{
		l__shell = dbg_shells;
			
		/* Find the shell */
		while(l__shell != NULL)
		{
			/* Is it the right shell. If yes, return the structure and leave dbg_shell_mutex closed */
			if (l__shell->terminal == l__i) 
				break;
				
			l__shell = l__shell->ls.n;
		}	
		
		if (l__shell == NULL) break;
	}
	
	mtx_unlock(&dbg_shell_mutex);

	if (l__i >= 12) return 1;
	
	return l__i;
}

/*
 * dbg_destroy_shell(term, sid)
 *
 * Destroys the shell which is running on terminal "term" for the
 * client "sid".
 *
 * This function may not be called by the shell thread itself. The
 * shell thread should call "dbg_destroy_shell_cur".
 *
 */
void dbg_destroy_shell(int terminal, sid_t client)
{
	dbg_shell_t *l__shell = dbg_find_shell(terminal, client);
	thread_t *l__thr;
		
	if (l__shell == (*dbg_tls_shellptr))
	{
		dbg_isprintf("Debugger: Implementation error. Shell thread 0x%X tries to kill itselfs with dbg_destroy_shell. Use dbg_destroy_shell_cur instead!\n", (*tls_my_thread)->thread_sid);
		
		return;
	}
	
	l__thr = l__shell->owner;
	
	/* Free the memory of the shell structures */
	if (l__shell->cmd_buffer != NULL) mem_free(l__shell->cmd_buffer);
	if (l__shell->pars != NULL)
	{
		int l__i = l__shell->n_pars;
		
		while(l__i --) 
			if (l__shell->pars[l__i] != NULL) mem_free(l__shell->pars[l__i]);
	}
	mem_free(l__shell->pars);
	lst_dellst(dbg_shells, l__shell);
	mem_free(l__shell);
		
	/* Kill its thread */
	blthr_kill(l__thr);
}


/*
 * dbg_shell_thread(thr)
 *
 * This is the main function of a new shell thread. It
 * waits for incoming keyboard events on its terminal
 * and executes the command passed by the user.
 *
 * The shell structure of the current thread will be
 * stored in the TLS-Pointer dbg_tls_shellptr during
 * the initialization part.
 *
 */
void dbg_shell_thread(thread_t *thr)
{
	/*
	 * Part A - Initialization
	 *
	 */	
	dbg_shell_t *l__myshell = NULL;
	*tls_errno = 0;	
	/* Find our shell by our thread id */
	mtx_lock(&dbg_shell_mutex, -1);
	
	l__myshell = dbg_shells;
	while(l__myshell != NULL)
	{
		if (l__myshell->owner == thr)
			break;
			
		l__myshell = l__myshell->ls.n;
	}
	
	/* No shell found. Error. */
	if (l__myshell == NULL)
	{
		dbg_isprintf("Can't find the shell structure. Shell thread 0x%X got killed now.\n", thr->thread_sid);
		mtx_unlock(&dbg_shell_mutex);	
		
		/* Give up. */
		while(1) blthr_finish();
	}
	
	/* Save the structure */
	*dbg_tls_shellptr = l__myshell;
	
	mtx_unlock(&dbg_shell_mutex);	

	/* Create the input buffer */
	l__myshell->cmd_buffer = mem_alloc(DBGSHELL_CMDBUFFER_SIZE);
	if (l__myshell->cmd_buffer == NULL)
	{
		/* Give up. */
		dbg_isprintf("Can't allocate command line buffer for 0x%X, because of %i.\n", thr->thread_sid, *tls_errno);
		
		while(1) blthr_finish();		
	}
	
	/* Contact our client thread */
	if (l__myshell->client)
	{
		int l__hooked;
		
		/* Get the client structure */
		mtx_lock(&dbg_clients_mtx, -1);
	
		dbg_client_t *l__client = dbg_find_client(l__myshell->client);
		if (l__client == NULL)
		{
			dbg_isprintf("Can't find a valid client structure for 0x%X.\n", l__myshell->client);
			mtx_unlock(&dbg_clients_mtx);
			
			while(1) blthr_finish();
		}
		l__hooked = l__client->hooked;
		
		mtx_unlock(&dbg_clients_mtx);		
		
		/* Is it a voluntary or a "hooked" client? */
		if (!l__hooked)
		{
			/* Voluntary, so we have to answer it */
			hymk_sync(l__myshell->client, 0xFFFFFFFF, 0);
			if (*tls_errno != 0)
			{
				dbg_isprintf("Can't connect client thread 0x%X, because of %i.\n", l__myshell->client, *tls_errno);
			
				while(1) blthr_finish();
			}
		
			/* Wait until it freezes itselfs */
			while(!(hysys_thrtab_read(l__myshell->client, THRTAB_THRSTAT_FLAGS) & THRSTAT_FREEZED))
			{
				blthr_yield(l__myshell->client);
			}
		}
		 else
		{
			/* Hooked. Just stop it now. */
			hymk_freeze_subject(l__myshell->client);
			if (*tls_errno != 0)
			{
				dbg_isprintf("Can't hook client thread 0x%X, because of %i.\n", l__myshell->client, *tls_errno);
			
				while(1) blthr_finish();
			}
		}
	
		dbg_iprintf(l__myshell->terminal, "Ready for debugging 0x%X.\n- Enter \"start\" to activate the thread.\n- Enter \"help\" for further informations.\n", l__myshell->client);
	}
	
	/*
	 * Part C - Shell Mainloop
	 *
	 */
	while(1)
	{
		size_t l__nchars;
		
		dbg_lock_terminal(l__myshell->terminal, 0);
		
		/* Display prompt and get datas */
		dbg_set_termcolor(l__myshell->terminal, DBGCOL_YELLOW);
		dbg_set_termflags(l__myshell->terminal, DBGTERM_ECHO_OFF|DBGTERM_PROMPT_ON|DBGTERM_PROMPT_ECHO_ON);							
		dbg_iprintf(l__myshell->terminal, "0x%X:>", l__myshell->client);
				
		dbg_set_termcolor(l__myshell->terminal, DBGCOL_WHITE);
		l__nchars = dbg_gets(l__myshell->terminal, DBGSHELL_CMDBUFFER_SIZE, l__myshell->cmd_buffer);
		dbg_set_termcolor(l__myshell->terminal, DBGCOL_GREY);

		/* Parse the input string */
		dbg_parse_cmd();
		
		dbg_execute_cmd();
		*tls_errno = 0;		/* Reset error status */
		
		dbg_unlock_terminal(l__myshell->terminal);
		
		/* Free the command buffers */
		dbg_free_cmdbuf();
	}
}

/*
 * dbg_filter_copy
 *
 * Works like "str_copy" but converts all \\ to \
 * And removes all single occurences of \.
 *
 */
static size_t dbg_filter_copy(utf8_t* dest, const utf8_t* src, size_t max)
{
	size_t l__retval = 0;
	
	if ((dest == NULL) || (src == NULL))
	{
		*tls_errno = ERR_INVALID_ADDRESS;
		return (size_t)-1;
	}
	
	while (max --)
	{
		l__retval ++;
		
		if (max > 0)
		{
			if (*src == '\\')
			{
				src ++;
				max --;
			}
		}
		
		*dest ++ = *src;
		if (!(*src ++)) break;
	}
	
	if (((max + 1) == 0) && (*(dest - 1) != 0))
	{
		return l__retval;
	}
		
	return l__retval;
}


/*
 * dbg_parse_cmd
 *
 * Analyses the keyboard input of the current shell. It
 * will split the command line into a set of single
 * atoms. Each atom will be stored in it order to the
 * pars[] list of the shell structure. The pointer to the
 * buffer with the first atom will be also included in "cmd".
 * 
 * Return values:
 *	0<	Error
 *	0	Empty input
 *	1 + n	Command and n parameters
 *
 */
int dbg_parse_cmd(void)
{
	dbg_shell_t *l__shell = (*dbg_tls_shellptr);
	utf8_t *l__bufptr = l__shell->cmd_buffer;
	int l__retval = 0;
	size_t l__len = str_len(l__bufptr, DBGSHELL_CMDBUFFER_SIZE) + 1;
		
	while(*l__bufptr != '\0')
	{
		/* Search until we find sthg. else than a space */
		if (*l__bufptr != ' ')
		{
			utf8_t l__term = 0;
			utf8_t *l__endptr = NULL;
			utf8_t *l__outbuf = NULL;
			
			/* Is it a trailing " or the begin of a atomic command? */
			if (*l__bufptr == '\"')
			{
				l__term = '\"';
				l__bufptr ++;
				if (*l__bufptr == '\0') break;
			}
			 else
			{
				l__term = ' ';
			}
			
			utf8_t *l__seekptr = l__bufptr;
			size_t l__atmplen = l__len;
			
			while (1)
			{
				/* Search the end of our command */
				l__endptr = str_char(l__seekptr, l__term, l__atmplen);
			
				/* Not the terminating charracter, but an escaped \"? */
				if ((l__endptr != NULL) && (l__term == '\"'))
				{
					if (*(l__endptr - 1) == '\\') 
					{
						l__atmplen -= ((uintptr_t)l__endptr - (uintptr_t)l__seekptr) + 1;
						
						l__seekptr = l__endptr + 1;
						
						continue;
					}
				}
				
				break;
			}
			
			/* End of the string */
			if (l__endptr == NULL)
			{
				l__outbuf = mem_alloc(sizeof(utf8_t) * (l__len + 1));
				if (l__outbuf == NULL)
				{
					dbg_isprintf("(1) Can't allocate memory for the parameter buffer of 0x%X, because of %i\n", (*tls_my_thread)->thread_sid, *tls_errno);
					return l__retval;
				}
				
				size_t l__newlen = dbg_filter_copy(l__outbuf, l__bufptr, l__len);
				l__outbuf[l__newlen] = '\0';
				
				l__outbuf = mem_realloc(l__outbuf, l__newlen + 1);
				if (l__outbuf == NULL)
				{
					dbg_isprintf("(3) Can't allocate memory for the parameter buffer of 0x%X, because of %i\n", (*tls_my_thread)->thread_sid, *tls_errno);
					return l__retval;
				}
				
			}
			 else /* Somewhere within the string */
			{
				size_t l__tmplen = ((uintptr_t)l__endptr - (uintptr_t)l__bufptr);
				
				l__outbuf = mem_alloc(sizeof(utf8_t) * (l__tmplen + 1));
				
				if (l__outbuf == NULL)
				{
					dbg_isprintf("(2) Can't allocate memory for the parameter buffer of 0x%X, because of %i\n", (*tls_my_thread)->thread_sid, *tls_errno);
					return l__retval;
				}
				
				size_t l__newlen = dbg_filter_copy(l__outbuf, l__bufptr, l__tmplen);			
				
				l__outbuf[l__newlen] = '\0';
				l__len -= l__tmplen;
				l__outbuf = mem_realloc(l__outbuf, l__newlen + 1);
				if (l__outbuf == NULL)
				{
					dbg_isprintf("(4) Can't allocate memory for the parameter buffer of 0x%X, because of %i\n", (*tls_my_thread)->thread_sid, *tls_errno);
					return l__retval;
				}				
			}
			
			/* Put it into the parameter list */
			utf8_t **l__pars = mem_realloc(l__shell->pars, (l__shell->n_pars + 1) * sizeof(utf8_t*));
			if (l__pars == NULL)
			{
				dbg_isprintf("Can't allocate memory for the parameter list of 0x%X, because of %i\n", (*tls_my_thread)->thread_sid, *tls_errno);
				return l__retval;
			}
			
			l__shell->pars = l__pars;
			
			/* Put it into the parameter buffer */
			l__shell->pars[l__shell->n_pars] = l__outbuf;
			l__shell->n_pars ++;
			
			l__retval ++;	
			
			/* Search for the next parameter */			
			if (l__endptr != NULL)
			{
				l__bufptr = l__endptr + 1;
			}
			 else
			{
				break;
			}
		} 
		 else
		{
			/* Next char, continue */
			l__bufptr ++;
			l__len --;
		}
	}

	/* Link the first parameter to "cmd" */
	if (l__shell->pars != NULL) 
	{
		l__shell->cmd = l__shell->pars[0];
	}
	
	/* Convert all variables to strings */
	int l__n = l__shell->n_pars;

	while (l__n --)
	{
		if (l__shell->pars[l__n] != NULL)
		{
			/* Is it a value? */
			if (l__shell->pars[l__n][0] == '$')
			{
				const utf8_t *l__name = &l__shell->pars[l__n][1];
				utf8_t l__buf[32];
				
				/* Get the value */
				if (dbg_get_value(l__name, l__buf, 32))
					continue;
				
				/* Set the parameter to the value of the variable */
				mem_free(l__shell->pars[l__n]);
				l__shell->pars[l__n] = mem_alloc(str_len(l__buf, 32) + 1);
				str_copy(l__shell->pars[l__n], l__buf, 32);
			}
		}
	}

	return l__retval;
}

/*
 * dbg_free_cmdbuf
 *
 * Frees the command buffer and the parameter buffers.
 *
 */
void dbg_free_cmdbuf(void)
{
	dbg_shell_t *l__shell = (*dbg_tls_shellptr);

	/* Reset the cmd pointer */
	l__shell->cmd = NULL;
	
	/* Free the parameter buffers */
	if (l__shell->pars != NULL)
	{
		int l__i = l__shell->n_pars;
		
		while(l__i --) 
			if (l__shell->pars[l__i] != NULL) mem_free(l__shell->pars[l__i]);
			
		mem_free(l__shell->pars);	
		l__shell->pars = NULL;			
	}

	/* Set the parameter count to zero */
	l__shell->n_pars = 0;
	
	return;
}


/*
 * dbg_execute_cmd
 *
 * Executes a command entered to the current shell. The
 * Command input has to be parsed by dbg_parse_cmd.
 *
 * Return value:
 *	>0	Status report of the executed command
 *	0	Command executed normally
 *	<0	Error value returned by the command
 *
 */
int dbg_execute_cmd(void)
{
	int l__i = dbg_commands_n;
	dbg_shell_t *l__shell = (*dbg_tls_shellptr);
	
	/* No command. Just return */
	if (l__shell->cmd == NULL) return 0;
	
	/* Find the right command */
	while (l__i --)
	{
		if (!str_compare(dbg_commands[l__i].cmd_name, l__shell->cmd, DBGSHELL_CMDBUFFER_SIZE))
		{
			/* Execute the command */
			return dbg_commands[l__i].execute_cmd();
		}
	}
	
	dbg_iprintf(l__shell->terminal, "Unkown command - \'%s\'\n", l__shell->cmd);
	
	return -1;
}

/*
 * dbg_test_par(start, val)
 *
 * Searches the parameter value "val" in the parameter list
 * starting from the parameter entry "start".
 *
 * Return value:
 *	>-1	Number of the parameter entry
 *	-1	Parameter value not found
 *
 */
int dbg_test_par(unsigned start, const utf8_t *val)
{
	unsigned l__i = 0;
	dbg_shell_t *l__shell = (*dbg_tls_shellptr);
	
	/* Is the list smaller? */
	if (start >= l__shell->n_pars) return -1;
	
	/* Search it */
	for (l__i = start; l__i < l__shell->n_pars; l__i ++)
	{
		if (!str_compare(l__shell->pars[l__i], val, DBGSHELL_CMDBUFFER_SIZE))
			return (signed)l__i;
	}
	
	return -1;	
}

/*
 * dbg_par_to_uint
 *
 * Converts the parameter "num" to an unsigned integer.
 * The parameter is stored to the buffer "buf". The parameter
 * is represented by an ASCII string. The string represents
 * a number to the given base "base".
 *
 * Valid bases are: 2, 8, 10, 16
 *
 * Return value:
 *	number -> *buf
 *	0	If successful
 *	1	If not a valid number
 *
 */
int dbg_par_to_uint(unsigned num, uint32_t *buf, int base)
{
	utf8_t *l__par;
	dbg_shell_t *l__shell = (*dbg_tls_shellptr);
	
	/* Valid parameter */
	if (num >= l__shell->n_pars)
		return 1;
		
	l__par = l__shell->pars[num];
	
	/* Calculate */
	return dbglib_atoul(l__par, buf, base);
}

/*
 * dbg_par_to_int
 *
 * Converts the parameter "num" to an signed integer.
 * The parameter is stored to the buffer "buf". The parameter
 * is represented by an ASCII string. The string represents
 * a number to the given base "base".
 *
 * Valid bases are: 2, 8, 10, 16
 *
 * Return value:
 *	number -> *buf
 *	0	If successful
 *	1	If not a valid number
 *
 */
int dbg_par_to_int(unsigned num, int32_t *buf, int base)
{
	utf8_t *l__par;
	dbg_shell_t *l__shell = (*dbg_tls_shellptr);
	
	/* Valid parameter */
	if (num >= l__shell->n_pars)
		return 1;
	
	l__par = l__shell->pars[num];
	
	/* Sign at the beginning? */
	return dbglib_atosl(l__par, buf, base);
}

/*
 * dbg_sh_term
 *
 * Changes the current terminal
 *
 * Usage
 *	term <number>
 *
 */
int dbg_sh_term(void)
{
	dbg_shell_t *l__shell = *dbg_tls_shellptr;
	
	/* Missing the terminal number? */
	if (l__shell->n_pars == 1)
	{
		dbg_iprintf(l__shell->terminal, "Missing parameter. Try \"help term\" for more informations.\n");	
		return -1;
	}
	
	/* Convert the parameter */
	uint32_t l__buf = 0;
	
	if (dbg_par_to_uint(1, &l__buf, 10) == 1)
	{
		dbg_iprintf(l__shell->terminal, "Invalid parameter. Try \"help term\" for more informations.\n");
		return -1;
	}
	else
	{
		if ((l__buf < 2) || (l__buf > 12))
		{
			dbg_iprintf(l__shell->terminal, "Invalid parameter. Try \"help term\" for more informations.\n");
			return -1;
		}
		
		dbg_iprintf(l__shell->terminal, "Changing to terminal %i.\n", l__buf);
		
		/* Get the client structure */
		mtx_lock(&dbg_clients_mtx, -1);
		dbg_client_t* l__client = dbg_find_client(l__shell->client);
		if (l__client == NULL) 
		{
			mtx_unlock(&dbg_clients_mtx);
			dbg_iprintf(l__shell->terminal, "Can't find client 0x%X for \"term\".\n", l__shell->client);
			return 0xFFFFFFFF;
		}
		mtx_unlock(&dbg_clients_mtx);
		
		/* Reset terminal flags to old state and changing terminal */
		dbg_set_termflags(l__shell->terminal, 0);
		dbg_change_terminal(l__client, l__buf);
		dbg_iprintf(l__shell->terminal, "Shell 0x%X switch to terminal %i.\n", l__shell->client, l__buf);
		
		return 0;
	}
}

/*
 * dbg_sh_export
 *
 * Creates or sets a variable
 *
 * Usage:
 *	export <variable> <value>
 *
 */
int dbg_sh_export(void)
{
	dbg_shell_t *l__shell = *dbg_tls_shellptr;
	const utf8_t *l__name = l__shell->pars[1];
	
	/* Missing parameters? */
	if (l__shell->n_pars < 3)
	{
		dbg_iprintf(l__shell->terminal, "Missing parameter. Try \"help export\" for more informations.\n");	
		return -1;
	}
	
	/* Trailing $? Remove it. */
	if (l__name[0] == '$') l__name ++;
	
	/* Export it */
	if (dbg_export(l__name, l__shell->pars[2]))
	{
		dbg_iprintf(l__shell->terminal, "Unable to export %s to %s.\n", l__shell->pars[1], l__shell->pars[2]);
		return -1;
	}
	
	return 0;
}

/* 
 * dbg_sh_echo
 *
 * Echos a text on the terminal
 *
 * Usage:
 *	echo <text>
 *
 */
int dbg_sh_echo(void)
{
	dbg_shell_t *l__shell = *dbg_tls_shellptr;
	
	/* Missing parameters? */
	if (l__shell->n_pars < 2)
	{
		/* Just print nothing */
		dbg_iprintf(l__shell->terminal, "\n");	
		return 0;
	}
	
	/* Print the first parameter */
	dbg_iprintf(l__shell->terminal, "%s\n", l__shell->pars[1]);
	
	return 0;
}	

