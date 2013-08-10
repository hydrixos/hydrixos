/*
 *
 * blthr.h
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
#ifndef _BLTHR_H
#define _BLTHR_H

#include <hydrixos/types.h>
#include <hydrixos/list.h>
#include <hydrixos/mutex.h>
#include <hydrixos/hymk.h>

/*
 * Thread datastructurs
 *
 */
/* User-level thread descriptor */
typedef struct thread_st {
	sid_t	thread_sid;   /* SID of the thread */
	void*	stack;        /* Stack-Area of the thread */
	size_t  stack_sz;     /* Size of the thread's stack in byte */
    
	/* User-defined start-up function of the thread*/
	void (*start)(struct thread_st *thr);

	struct atexit_st *atexit_list;     /* List of the at-exit functions */
    	int    atexit_cnt;	/* Count of the at-exit functions */
    
    	mtx_t mutex;		   /* The entry mutex (it will not protect the function calls!) */
}thread_t;

/* At-Exit descriptor */
typedef struct atexit_st
{
	void (*func)(struct thread_st *thr);	/* At-exit function */
	list_t ls;			/* (Linked list) */
}atexit_t;


/*
 * Functions of the BasicLib thread package
 *
 */
/* Create a BlThread */
thread_t* blthr_create(void (*start)(thread_t *thr), size_t stack);

/* Handler functions of the thread package */
void blthr_cleanup(void);
void blthr_kill(thread_t *thr);
void blthr_init(thread_t *thr);
void blthr_init_arch(void);
void blthr_awake(thread_t *thr);
void blthr_freeze(thread_t *thr);
void blthr_finish(void);
void blthr_atexit(struct thread_st *thr, void (*func)(struct thread_st *thr));

/*
 * blthr_yield(sid)
 *
 * Yields the current thread. If a rest of CPU time
 * remains, the function will pass it to the thread
 * "sid". (More informations, see hymk_yield_thread).
 *
 */
static inline void blthr_yield(sid_t sid)
{
	hymk_yield_thread(sid);
}

/*
 * Global informations of the BlThread package
 *
 */
extern thread_t **tls_my_thread; 	/* Pointer to the current thread descriptor */

#endif
