/*
 *
 * server.c
 *
 * (C)2006 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library').   
 *
 * Communication to the debugging server
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

/* This variable is only available to init! */
extern sid_t initproc_debugger_sid;

/*
 * cdbg_connect(void)
 *
 * Connects to the debugger thread and retrieves a worker thread
 * from it.
 *
 * Return value: The SID of the worker thread which is responsible
 *		 for the current thread.
 *
 */
sid_t cdbg_connect(void)
{
	/* Synchronize with its controler thread */
	sid_t l__server = hysys_prctab_read(initproc_debugger_sid, PRCTAB_CONTROLLER_THREAD_SID);
	
	hymk_sync(l__server, 0xFFFFFFFF, 0);
	if (*tls_errno != 0)
		return 0;	
	
	/* Now we wait for any thread of the other side */
	sid_t l__worker = hymk_sync(initproc_debugger_sid, 0xFFFFFFFF, 0);
		
	if (*tls_errno != 0)
		return 0;
		
	/* And freezing ourselfs */
	blthr_freeze(*tls_my_thread);
			
	if (*tls_errno != 0)
		return 0;
	else
		return l__worker;
}
