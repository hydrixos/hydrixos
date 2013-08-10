/*
 *
 * main.c
 *
 * (C)2005 by Friedrich Gr�ter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying'). 
 *
 * Entry code of hyInit
 *
 */
#include <hydrixos/types.h> 
#include <hydrixos/errno.h> 
#include <hydrixos/hymk.h> 
#include <hydrixos/tls.h>
#include <hydrixos/stdfun.h>
#include <hydrixos/blthr.h>

#include <hydrixos/system.h>
#include <hydrixos/mem.h>
#include <hydrixos/pmap.h>

#include "hyinit.h"

volatile int init_process_number = INITPROC_MAIN;
extern region_t *pmap_region;

extern void initfork_entry(void);

uintptr_t initfork_stack_buf;
thread_t* initfork_thread_buf;



/*
 * initfork_libinit(void)
 *
 * Reinitializes the library after a new fork
 * was created.
 *
 */
void initfork_libinit(void)
{
	*tls_my_thread = initfork_thread_buf;
	*tls_errno = 0;
	
	return;
}

/*
 * init_fork
 *
 * Creates a copy of the init process. The new process will
 * start up completly. After entering main() the program will
 * read the content of the init_process_number value to detect
 * what subprocess has to be started.
 *
 */
extern void crt_entry(void);

sid_t init_fork(int pnum)
{
	int l__oldpnum = init_process_number;
		
	/* Create a new process */
	sid_t l__new_proc = hymk_create_process(&initfork_entry, NULL);
	if (*tls_errno) {iprintf("CREATE ERROR: %i\n", *tls_errno); while(1);}
	sid_t l__new_thr = hysys_prctab_read(l__new_proc, PRCTAB_CONTROLLER_THREAD_SID);
		
	/* Change init process number temporarily */
	init_process_number = pnum;
	HYSYS_MSYNC();
	
	initfork_stack_buf = ((uintptr_t)(*tls_my_thread)->stack) + (*tls_my_thread)->stack_sz;
	initfork_thread_buf = (*tls_my_thread);
	
	/* Fill its address space */
	hysys_map(l__new_thr, 
		  code_region->start,
		  code_region->pages,
		  MAP_READ|MAP_WRITE|MAP_EXECUTABLE|MAP_COPYONWRITE,
		  (uintptr_t)code_region->start
		 );
	hysys_map(l__new_thr, 
		  heap_region->start,
		  heap_region->pages,
		  MAP_READ|MAP_WRITE|MAP_EXECUTABLE|MAP_COPYONWRITE,
		  (uintptr_t)heap_region->start
		 );	
	hysys_map(l__new_thr, 
		  pmap_region->start,
		  pmap_region->pages,
		  MAP_READ|MAP_WRITE|MAP_EXECUTABLE|MAP_COPYONWRITE,
		  (uintptr_t)pmap_region->start
		 );		 
	if (*tls_errno) {iprintf("MAP ERROR: %i\n", *tls_errno); while(1);}
	
	/* Synchronize memory and re-set init process number */
	HYSYS_MSYNC();	
	
	init_process_number = l__oldpnum;
	
	/* Start it */
	hymk_awake_subject(l__new_thr);
	if (*tls_errno) {iprintf("AWAKE ERROR: %i\n", *tls_errno); while(1);}
	
	return l__new_proc;
}

/*
 * init_kill
 *
 * Terminates the current init process.
 *
 */
void init_kill(void)
{
	hymk_destroy_subject(hysys_info_read(MAININFO_CURRENT_PROCESS));
}

/*
 * main
 *
 * Entry code of any init process. It will be used for initialization of any 
 * subprocess of init.
 *
 */
int main(void)
{
	/* Init the initial console output */
	init_iprintf();

	if (init_process_number != INITPROC_MAIN) {pmap_alloc(2); pmap_alloc(2);	}

	if (init_process_number == 0) {iprintf("NUMBER STOPPED."); while(1);};

	if (hysys_info_read(MAININFO_CURRENT_PROCESS) > 0x2000002) {iprintf("ERROR STOPPED."); while(1);}

	/* Detect the right init process */
	switch(init_process_number)
	{
		case (INITPROC_MAIN):		init_main(); break;	/* Init main process */
		
		case (INITPROC_DEBUGGER):	debugger_main(); break;	/* Debugger process */
	
		default:
		{
			iprintf("Invalid init process number %i. Killing 0x%X.\n", init_process_number, hysys_info_read(MAININFO_CURRENT_PROCESS));
			init_kill();
		}
	}
	
	iprintf("Init process %i (0x%X) terminated. Killing it.", init_process_number, hysys_info_read(MAININFO_CURRENT_PROCESS));
	
	init_kill();
	
	/* Never reached */
	return 0;
}
