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

#include "hybaselib.h"

thread_t **tls_my_thread = NULL; 			/* Pointer to the current thread descriptor */
uintptr_t lib_blthr_setup_stack_arch(thread_t *thr);	/* Declared in arch/blthr-arch.c */

/* Clean-up table */
thread_t **lib_blthr_cleanup_table = NULL;			/* The cleanup table */
unsigned lib_blthr_cleanup_table_num = 0;			/* Number of entries */
mtx_t lib_blthr_cleanup_mtx = MTX_DEFINE();			/* Mutex to lock the cleanup table */

/*
 * lib_init_blthreads(stack)
 *
 * Initializes the BasicLib Thread package.
 * This function will esp. initialize the thread
 * descriptor of the first thread of this process.
 * The new thread gets the stack at address
 * "stack" (this is a pointer to the upper end of
 * the stack and NOT the stack pointer used
 * by the CPU!)
 *
 * This function will also alloc the global TLS
 * value "tls_my_thread" and initializes it.
 *
 * Return value:
 *	0 	Operation successful
 *	1	Operation failed
 *
 */
int lib_init_blthreads(void* stack)
{
	/* Create and setup our thread descriptor */
	thread_t *l__thread = mem_alloc(sizeof(thread_t));
	if (l__thread == NULL) return 1;
	
	l__thread->thread_sid = hysys_info_read(MAININFO_CURRENT_THREAD);
	l__thread->stack = stack;
	l__thread->stack_sz = ARCH_STACK_SIZE;
	
	l__thread->start = NULL;	/* We have main() */
	
	l__thread->atexit_list = NULL;
	l__thread->atexit_cnt = 0;
	
	l__thread->mutex = MTX_NEW();
	
	/* Set up the global TLS entry of this thread */
	tls_my_thread = (thread_t**)tls_global_alloc();
	if (tls_my_thread == NULL) return 1;
	
	/* Set up our local TLS_MY_THREAD pointer */
	*tls_my_thread = l__thread;
	
	/* Okay. This was easy */
	return 0;
}


/*
 * blthr_cleanup(thr)
 *
 * Destroys all descriptors and existing data 
 * structures of the thread "thr". This function
 * will also kill the thread "thr".
 *
 */
void blthr_cleanup(void)
{
	mtx_lock(&lib_blthr_cleanup_mtx, MTX_UNLIMITED);
	
	int l__i = lib_blthr_cleanup_table_num;

	/* Nothing to clean... */
	if ((lib_blthr_cleanup_table == NULL) || (l__i == 0))
	{
		mtx_unlock(&lib_blthr_cleanup_mtx);
		return;
	}
	
	while (l__i --)
	{
		thread_t* l__thr = lib_blthr_cleanup_table[l__i];
		
		/* We can't commit suicide... */
		if (l__thr == *tls_my_thread) 
		{
			/* Just remove us from the table, don't do anything else */
			lib_blthr_cleanup_table[l__i] = NULL;
			
			/* This can't be, test your implementation, stupid! */
			*tls_errno = ERR_IMPLEMENTATION_ERROR;
			
			mtx_unlock(&lib_blthr_cleanup_mtx);
			return;
		}
		
		/* Ignore empty entries */
		if (l__thr == NULL)
		{
			continue;
		}
	
		/* Kill the thread "thr" */
		hymk_destroy_subject(l__thr->thread_sid);
		if (*tls_errno) 
		{
			mtx_unlock(&l__thr->mutex);
			mtx_unlock(&lib_blthr_cleanup_mtx);
			return;
		}
		
		/* Free its stack */
		mem_stack_free(l__thr->stack);
		if (*tls_errno)
			*tls_errno = 0;
	
		/* Free its atexit functions */
		int l__n = l__thr->atexit_cnt;
		atexit_t *l__atexit = l__thr->atexit_list;
	
		while (l__n --)
		{
			atexit_t *l__tmp_atexit	= l__atexit->ls.n;
			
			mem_free(l__atexit);
			l__atexit = l__tmp_atexit;
		
			if (l__atexit == NULL) break;
			
			*tls_errno = 0;
		}
	
		/* Free its descriptor */
		mem_free(l__thr);
		*tls_errno = 0;
		
	}
	
	/* Free the cleanup table */
	mem_free(lib_blthr_cleanup_table);
	lib_blthr_cleanup_table_num = 0;
	lib_blthr_cleanup_table = NULL;
	
	mtx_unlock(&lib_blthr_cleanup_mtx);
	return;
}

/*
 * lib_blthr_add_cleanup(thr)
 *
 * Adds a thread to the cleanup list
 *
 */
static void lib_blthr_add_cleanup(thread_t *thr)
{
	mtx_lock(&lib_blthr_cleanup_mtx, MTX_UNLIMITED);
	
	/* Can't add it. Should be an implementation error */
	if ((lib_blthr_cleanup_table_num + 1) < lib_blthr_cleanup_table_num)
	{
		*tls_errno = ERR_IMPLEMENTATION_ERROR;
		mtx_unlock(&lib_blthr_cleanup_mtx);
		return;
	}
	
	lib_blthr_cleanup_table_num ++;
	
	thread_t** l__tmp = mem_realloc(lib_blthr_cleanup_table, lib_blthr_cleanup_table_num);
	if (l__tmp == NULL) 
	{
		mtx_unlock(&lib_blthr_cleanup_mtx);
		return;	
	}
	
	lib_blthr_cleanup_table = l__tmp;
	
	lib_blthr_cleanup_table[lib_blthr_cleanup_table_num - 1] = thr;

	mtx_unlock(&lib_blthr_cleanup_mtx);
	
	return;
}

/*
 * lib_blthr_do_atexit(thr)
 *
 * Executes the atexit functions of the thread "thr".
 * 
 * NOTE: This function will expect that the mutex
 *	 thr->mutex is unlocked. To prevent
 *	 asynchronities, this function will create
 *	 a temporary local copy of the atexit function
 *	 list.
 *
 */
static void lib_blthr_do_atexit(thread_t *thr)
{
	atexit_t *l__atexits = 0;
	atexit_t *l__cur_atexit;
	int l__atexit_cnt = 0;

	if (thr == NULL) return;
	if ((thr->atexit_list == NULL) || (thr->atexit_cnt == 0)) return;
	
	/* Copy the atexit-list */
	mtx_lock(&thr->mutex, -1);
	l__atexit_cnt = thr->atexit_cnt;
	l__atexits = mem_alloc(thr->atexit_cnt * sizeof(atexit_t));
	l__cur_atexit = thr->atexit_list;
	
	/* Copy the atexit list */
	int l__i = l__atexit_cnt;
	
	while(l__i --)
	{
		l__atexits[l__i] = *l__cur_atexit;
		l__cur_atexit = l__cur_atexit->ls.n;
		
		if (l__cur_atexit == NULL) break;
	}
	
	mtx_unlock(&thr->mutex);
	
	/* Execute the functions */
	while(l__atexit_cnt --)
	{
		l__atexits[l__atexit_cnt].func(thr);
	}
	
	mem_free(l__atexits);
	
	return;
}


/*
 * blthr_kill
 *
 * Kills the thread "thr" and cleans all of its
 * data structures after calling all of its
 * atexit functions.
 *
 * The function kills the thread using "blthr_cleanup".
 *
 */
void blthr_kill(thread_t *thr)
{
	if (thr == NULL)
	{
		*tls_errno = ERR_INVALID_ADDRESS;
		return;
	}
	
	/* We are not allowed to kill our self */
	if (thr == *tls_my_thread) 
	{
		*tls_errno = ERR_ACCESS_DENIED;
		return;
	}
	
	/* Lock its mutex */
	mtx_lock(&thr->mutex, -1);
	
	/* Stop it */
	hymk_freeze_subject(thr->thread_sid);
	if (*tls_errno) {mtx_unlock(&thr->mutex); return;}
	
	mtx_unlock(&thr->mutex);
	
	/* Execute its atexit functions */
	lib_blthr_do_atexit(thr);	
	
	/* Clean it up ... */
	lib_blthr_add_cleanup(thr);
	if (*tls_errno)
	{
		return;
	}
	
	blthr_cleanup();
}

/*
 * blthr_awake(thr)
 *
 * Awake the freezed thread "thr" using 
 * hymk_awake_subject.
 *
 */
void blthr_awake(thread_t *thr)
{
	sid_t l__sid = 0;
	
	if (thr == NULL)
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
	}
	
	/* Get its SID */
	mtx_lock(&thr->mutex, -1);
	l__sid = thr->thread_sid;
	mtx_unlock(&thr->mutex);
	
	/* Freeze it */
	hymk_awake_subject(l__sid);
	
	return;
}

/*
 * blthr_freeze(thr)
 *
 * Freezes the running thread "thr" using
 * hymk_freeze_subject.
 *
 */
void blthr_freeze(thread_t *thr)
{
	sid_t l__sid = 0;
	
	if (thr == NULL)
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
	}
		
	/* Get its SID */
	mtx_lock(&thr->mutex, -1);
	l__sid = thr->thread_sid;
	mtx_unlock(&thr->mutex);
	
	/* Freeze it */
	hymk_freeze_subject(l__sid);
	
	return;
}

/*
 * blthr_atexit(thr, func)
 *
 * Adds the function "func" to the so-called
 * atexit-list of the Thread "thr". If the thread
 * terminates or get killed by blthr_kill (not by
 * blthr_cleanup!) this function will be calld. It
 * get the parameter "thr" as a pointer to the
 * affected thread.
 *
 */
void blthr_atexit(struct thread_st *thr, void (*func)(struct thread_st *thr))
{
	atexit_t *l__atexit = NULL;
	
	if ((thr == NULL) || (func == NULL))
	{
		*tls_errno = ERR_INVALID_ADDRESS;
		return;
	}

	/* Create a new atexit structure */
	l__atexit = mem_alloc(sizeof(*l__atexit));
	if ((l__atexit == NULL) || (*tls_errno)) return;
	
	l__atexit->func = func;
		
	/* Add it to the atexit-List of the Thread descriptor */
	mtx_lock(&(thr->mutex), -1);
	lst_add(thr->atexit_list, l__atexit);
	thr->atexit_cnt ++;
	mtx_unlock(&(thr->mutex));
	
	return;	
}

/*
 * blthr_finish()
 *
 * Terminates the current thread. This
 * function will set the thread to a
 * permanent loop of hymk_freeze_subject.
 * The thread can be destroyed by blthr_cleanup
 * or blthr_kill. Before locking the thread
 * it will call all atexit-functions that
 * were defined for this thread.
 *
 */
void blthr_finish(void)
{
	/* Execute our atexit functions */
	lib_blthr_do_atexit(*tls_my_thread);
	
	/* Add us to the cleanup table */
	lib_blthr_add_cleanup(*tls_my_thread);
	
	/* Freeze us for ever */
	while(1) hymk_freeze_subject(0);
}

/*
 * blthr_create(start, stack)
 *
 * Creates a new BasicLib thread. The thread starts its work
 * in the function "start". Its initial stack has a size of
 * "stack" bytes.
 *
 * Return value:
 *	Pointer to the thread data structure.
 *
 * NOTE: This function passes the thraed descriptor to the
 *       startup code "blthr_init" using the stack of the
 *	 new thread. The function blthr_init is written in
 *	 assembler language and implemented in crt0.s
 *
 */
thread_t* blthr_create(void (*tstart)(thread_t *thr), size_t stack)
{
	uintptr_t l__stptr = 0;
	thread_t *l__thread = NULL;
	
	/* Clean up */
	blthr_cleanup();
	*tls_errno = 0;
	
	/* Valid start address? */
	if (tstart == NULL)
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		return NULL;
	}
	
	/* Architecture dependend stack size */
	if (stack == 0) stack = ARCH_STACK_SIZE;
	
	/* Create the new descriptor */
	l__thread = mem_alloc(sizeof(thread_t));
	if ((l__thread == NULL) || (*tls_errno))
	{
		return NULL;
	}
	
	/* Create the new stack */
	l__thread->stack = mem_stack_alloc(stack);
	if ((l__thread->stack == NULL) || (*tls_errno))
	{
		errno_t l__tmperrno = *tls_errno;
		*tls_errno = 0;
		
		mem_free(l__thread);
		
		*tls_errno = l__tmperrno;
		return NULL;
	}
	
	l__thread->stack_sz = stack;
	
	/* Initialize the new stack */
	l__stptr = lib_blthr_setup_stack_arch(l__thread);
	if (l__stptr == 0)
	{
		errno_t l__tmperrno = *tls_errno;
		*tls_errno = 0;
		
		mem_stack_free(l__thread->stack);
		mem_free(l__thread);
		
		*tls_errno = l__tmperrno;
		return NULL;
	}
	
	/* Write the new descriptor */
	l__thread->start = tstart;	/* We have main() */
	
	l__thread->atexit_list = NULL;
	l__thread->atexit_cnt = 0;
	
	l__thread->mutex = MTX_NEW();
	
	/* Create the new thread */
	l__thread->thread_sid = hymk_create_thread(&blthr_init_arch, (void*)l__stptr);
	if (*tls_errno)
	{
		errno_t l__tmperrno = *tls_errno;
		*tls_errno = 0;
		
		mem_stack_free(l__thread->stack);
		mem_free(l__thread);
		
		*tls_errno = l__tmperrno;
		return NULL;
	}
	
	return l__thread;
}

