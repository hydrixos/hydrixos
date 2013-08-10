/*
 *
 * blthrd.c
 *
 * (C)2005 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library').   
 *
 * Basic library thread package
 *
 */
#include <hydrixos/types.h>
#include <hydrixos/tls.h>
#include <hydrixos/hymk.h>
#include <hydrixos/errno.h>
#include <hydrixos/mem.h>
#include <hydrixos/mutex.h>
#include <hydrixos/stdfun.h>
#include <hydrixos/system.h>
#include <hydrixos/blthr.h>

#include "../../hybaselib.h"

/*
 * blthr_init(thr)
 *
 * Initializes the Thread 'thr' and executes
 * its user-defined start routine.
 *
 * NOTE: We come from blthr_init_arch in crt0.s
 *
 */
void blthr_init(thread_t *thr)
{
	/* Initialize the TLS */
	*tls_my_thread = thr;
	*tls_errno = 0;
	
	/* Start our execution */
	thr->start(thr);
}

/*
 * lib_blthr_setup_stack_arch(thr, sz)
 *
 * This function will place the pointer to the
 * thread data structure of the new thread on its stack.
 * The stack has to be allready created and initialized
 * in the "thr" data structure.
 *
 * Return value:
 *	!= 0, The new stack pointer
 *	== 0, Error
 *
 */
uintptr_t lib_blthr_setup_stack_arch(thread_t *thr)
{
	uintptr_t l__retval = 0;
	thread_t **l__stackbuf = NULL;
	
	if (thr == NULL) return 0;
	if (thr->stack == NULL) return 0;
	
	/* Calculate the new stack pointer */
	l__retval = (uintptr_t)thr->stack;
	l__retval += thr->stack_sz;
		
	/* 
	 * Write the pointer to the thread descriptor 
	 * to the new stack 
	 *
	 */
	l__retval -= sizeof(thread_t*);
	l__stackbuf = (void*)l__retval;
	*l__stackbuf = thr;
	
	return l__retval;	
}
