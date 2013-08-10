/*
 *
 * libinit.c
 *
 * (C)2005 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library').   
 *
 * Initialization of the hyBaseLib
 *
 */
#include <hydrixos/types.h>
#include <hydrixos/tls.h>
#include <hydrixos/blthr.h>
#include <hydrixos/mem.h>
#include "hybaselib.h"

/*
 * lib_init_hybaselib()
 *
 * Initializes the hyBaseLib. This function should be
 * only called from the C-Runtime start up code (crt0.s).
 * In detail it initializes:
 *
 *		- The TLS-managment
 *		- The memory regions
 *		- The heap managment
 *		- The mapping managment
 *		- The primary stack
 *		- The BlThread package
 *
 */
void* lib_init_hybaselib(void)
{
	void *l__stack = NULL;
	uintptr_t l__stackptr = 0;
	
	/* Setup the TLS managment */
	if (lib_init_global_tls() == 1) return NULL;
	
	/* Setup the memory regions */
	if (lib_init_regions() == 1) return NULL;
	
	/* Initializes the heap */
	if (lib_init_heap() == 1) return NULL;
	
	/* Initialize our new stack */
	l__stack = mem_stack_alloc(ARCH_STACK_SIZE);
	if (l__stack == NULL) return NULL;
		
	/* Calculate the new stack pointer (stack_begin + stack_size) */
	l__stackptr = ((uintptr_t)l__stack) + ARCH_STACK_SIZE;
	
	/* Setup our initial thread */
	if (lib_init_blthreads(l__stack) == 1) return NULL;

	/* Initializes the mapping managment */
	if (lib_init_pmap() == 1) return NULL;

	/* TODO: This is temporary */
	return (void*)(uintptr_t)l__stackptr;
}
