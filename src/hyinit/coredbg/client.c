/*
 *
 * client.c
 *
 * (C)2006 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying').   
 *
 * Client managment functions
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


dbg_client_t *dbg_clients = NULL;
mtx_t dbg_clients_mtx = MTX_DEFINE();

/*
 * dbg_find_client(client)
 *
 * Returns the client structure of a client thread which
 * has the SID "client".
 *
 * The function expects dbg_client_mtx to be locked
 * before entering it!
 *
 */
dbg_client_t* dbg_find_client(sid_t client)
{
	dbg_client_t *l__client = NULL;
	
	l__client = dbg_clients;
	
	/* Search it */
	while(l__client != NULL)
	{
		if (l__client->client == client)
		{
			/* Found it, keep mutex locked */
			return l__client;
		}
		
		l__client = l__client->ls.n;
	}
	
	return NULL;
}

/*
 * dbg_create_client_general
 *
 * Creates a client structure and adds it to the
 * client list. This function is for internal usage
 * only. It expects dbg_clients_mtx locked and return
 * it also locked.
 *
 * Return value:
 *	 1	there is a client registered already
 *	 0	if successful
 *	-1	if failed
 *
 */
static int dbg_create_client_general(sid_t client, int term, int hook)
{
	/* Cleanup thread, to get sure that we are not debugging our own ghosts*/
	blthr_cleanup();
	
	dbg_client_t *l__client = dbg_find_client(client);
	
	/* Is there a client registered already? */
	if (l__client != NULL)
	{
		return 1;	
	}
		
	/* Is it a valid thread */
	if (hysys_thrtab_read(client, THRTAB_IS_USED) == 0)
	{
		dbg_isprintf("Invalid thread 0x%X.\n", client);
		return -1;
	}
			
	/* Create some resources before */
	dbg_client_t *l__new = mem_alloc(sizeof(dbg_client_t));
	if (l__new == NULL) 
	{
		return -1;
	}
	
	dbg_shell_t *l__shell = dbg_create_shell(term, client);
	if (l__shell == NULL)
	{
		mem_free(l__new);
		return -1;
	}				
				
	/* Setup the client structure */
	l__new->client = client;
	l__new->shell = l__shell;
	l__new->trace_flags = DBG_TRACE_BREAKPOINT_COUNTER;
	l__new->halt_flags = DBG_HALT_BREAKPOINTS;
	
	l__new->breakpoints = NULL;
	l__new->breakpoints_n = 0;
	
	l__new->hooked = hook;
	
	/* Add it to the clients list */
	lst_add(dbg_clients, l__new);
	
	return 0;
}

/*
 * dbg_create_client
 *
 * Creates a client structure and adds it to the
 * client list. Use this function only, if a thread
 * gets a client voluntary.
 *
 * Return value:
 *	 1	there is a client registered already
 *	 0	if successful
 *	-1	if failed
 *
 */
int dbg_create_client(sid_t client, int term)
{
	/* Lock the mutex for access */
	mtx_lock(&dbg_clients_mtx, -1);
	
	/* Create it. It is a voluntary client, hook = 0! */
	int l__i = dbg_create_client_general(client, term, 0);
	
	/* Done. */
	mtx_unlock(&dbg_clients_mtx);
	return l__i;
}

/*
 * dbg_hook_client
 *
 * Creates a debugger-session for the thread "other".
 * This will stop the execution of "other" during the
 * initialization of the threads' debugger shell. Use
 * this function, if a client doesn't get a debugger
 * client by itself.
 *
 * Return value:
 *	 1	there is a client registered already
 *	 0	if successful
 *	-1	if failed
 *
 */
int dbg_hook_client(sid_t other, int term)
{
	/* Lock the mutex for access */
	mtx_lock(&dbg_clients_mtx, -1);
	
	/* Create it. We force it to be a client, hook = 1! */
	int l__i = dbg_create_client_general(other, term, 1);
	
	/* Done. */
	mtx_unlock(&dbg_clients_mtx);
	return l__i;
}

/*
 * dbg_destroy_client
 *
 * Destroys a client and its shell.
 *
 */
void dbg_destroy_client(sid_t client)
{
	dbg_shell_t *l__shell = NULL;
	
	mtx_lock(&dbg_clients_mtx, -1);
	
	/* Get the client structure */
	dbg_client_t* l__client = dbg_find_client(client);
	if (l__client == NULL) 
	{
		mtx_unlock(&dbg_clients_mtx);
		return;
	}
		
	l__shell = l__client->shell;
	
	lst_dellst(dbg_clients, l__client);
	
	/* Destroy the client structure */
	dbg_free_breakpoint_list(l__client);
	
	mem_free(l__client);
		
	/* Destroy the shell */
	if (l__shell == (*dbg_tls_shellptr))
	{
		mtx_unlock(&dbg_clients_mtx);

		/* It is the current shell (we will be finished after that) */
		dbg_destroy_shell_cur();
		
		/* Still reached? Error */
		*tls_errno = ERR_IMPLEMENTATION_ERROR;
	}
	 else if (l__shell != NULL)
	{
		mtx_unlock(&dbg_clients_mtx);

		/* It is the shell of another worker thread */
		dbg_destroy_shell(l__shell->terminal, l__shell->client);
	}
		
	return;
}

/*
 * dbg_logon_client(client)
 *
 * Connects a client to a worker thread. This
 * function may be called by a future client thread
 * using a sync()-call to the controller thread
 * of the debugger. (look at dbg_logon_loop).
 *
 */
void dbg_logon_client(sid_t client)
{
	dbg_create_client(client, 11);	
}

/*
 * dbg_logon_loop
 *
 * Waits for incoming sync()-requests. If a thread
 * successfuly syncs() to the current thread (normally
 * the controller thread) the thread will be logged on
 * to the debuuger using dbg_logon_client.
 *
 * The syncing client thread will be awaked by another
 * sync() call, if its worker thread has been started
 * and got the control over the client side.
 *
 * Each new worker gots its first terminal on Terminal 2.
 *
 */
void dbg_logon_loop(void)
{
	dbg_isprintf("Debugger ready.\n\n");
	
	while (1)
	{
		sid_t l__other = hymk_sync(SID_USER_EVERYBODY, 0xFFFFFFFF, 0);
		
		if (*tls_errno)
		{
			/* Error? Retry. */
			dbg_isprintf("Can't sync to SID_USER_EVERYBODY in logon loop, because of 0x%X\n", *tls_errno);
			*tls_errno = 0;
		}
		
		dbg_isprintf("Logon to 0x%X...\n", l__other);
		/* Create a new session */
		int l__term = dbg_find_free_terminal();

		if (dbg_create_client(l__other, l__term) == 0) 
		{
			dbg_isprintf("\tClient created for 0x%X. Shell at terminal %i.\n", l__other, l__term);
			
			/* The new worker thread will awake the client later */
		}
	}
}
