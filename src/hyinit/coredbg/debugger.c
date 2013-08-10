/*
 *
 * debugger.c
 *
 * (C)2006 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying').   
 *
 * Debugger initialization
 *
 */
#include <hydrixos/types.h>
#include <hydrixos/tls.h>
#include <hydrixos/errno.h>
#include <hydrixos/mem.h>

#include "../hyinit.h"
#include "coredbg.h"

/*
 * debugger_main
 *
 * Startup code of the core debugger process.
 *
 */
int debugger_main(void)
{
	/* Initialize the display driver */
	dbg_init_driver();

	dbg_create_window("View 2");
	dbg_create_window("View 3");
	dbg_create_window("View 4");
	dbg_create_window("View 5");
	dbg_create_window("View 6");
	dbg_create_window("View 7");
	dbg_create_window("View 8");
	dbg_create_window("View 9");
	dbg_create_window("View 10");
	dbg_create_window("View 11");
	dbg_create_window("View 12");
	

	/* Initialize the shell */
	dbg_init_shell();
	
	/*** Test code **/
	dbg_set_termcolor(1, DBGCOL_WHITE);
	dbg_isprintf("HydrixOS Operating System 0.0.2\n");
	dbg_isprintf("-------------------------------\n\n");
	dbg_set_termcolor(1, DBGCOL_GREY);
	dbg_isprintf("HyCoreDbg - HydrixOS Core Debugger v 0.0.1\n");
	dbg_isprintf("This application will help you to debug the first\n");
	dbg_isprintf("initial processes of HydrixOS. It gives you a\n");
	dbg_isprintf("terminal with different windows (F1 - F12) and interactive\n");
	dbg_isprintf("shells for debugging the system. The application waits\n");
	dbg_isprintf("now for incomming debugging requests from the other\n");
	dbg_isprintf("threads.\n\n");
	
	while(1) dbg_logon_loop();
	
	return 0;
}
