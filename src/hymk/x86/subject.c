/*
 *
 * subject.c
 *
 * (C)2004 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying'). 
 *
 * Process and thread managment
 *
 */
#include <hydrixos/types.h>
#include <hydrixos/sid.h>
#include <setup.h>
#include <stdio.h>
#include <mem.h>
#include <page.h>
#include <info.h>
#include <sysc.h>
#include <error.h>
#include <sched.h>
#include <current.h>
#include <sysc.h>

uint32_t ksubj_next_unique_process_id = 0;
uint32_t ksubj_next_unique_thread_id = 0;

void ksync_interrupt_other(uint32_t *other);
void ksync_removefrom_waitqueue_error(uint32_t *other, uint32_t *me);

/*
 * ksubj_create_thread(eip, esp)
 *
 * Creates a new thread. The new thread inherits the access list
 * the process membership, the static priority and the scheduling
 * class of the thread that created it. The new thread will created
 * as "frozen thread", so you have to awake it with "awake_subject"
 * before it will do anything.
 *
 * The function won't add the thread to the thread list of the 
 * current process, so it can be used as a low-end implementation
 * for the high-end system calls 'create_thread' and 'create_process',
 * which have to add the threads to different thread lists.
 *
 * Parameters:
 *	eip	instruction pointer of the new thread
 *	esp	stack pointer of the new thread
 *
 * Return-Value:
 *	SID of the new thread
 *
 */
static inline sid_t ksubj_create_thread(uintptr_t eip, uintptr_t esp)
{
	uint32_t *l__descr = NULL;
	sid_t l__retval = 0;
	void* l__kstack = NULL;
	void* l__dadr = NULL;
	
	/* Search an unused SID */
	l__descr = thread_tab;
	
	while (l__retval != THREAD_MAX)
	{
		l__descr = &THREAD(l__retval, 0);
		if (!l__descr[THRTAB_IS_USED]) break;
		l__retval ++;
	}
	
	/* No descriptor found */
	if (l__descr[THRTAB_IS_USED]) 
	{
		SET_ERROR(ERR_NOT_ENOUGH_MEMORY);
		return SID_PLACEHOLDER_INVALID;
	}
	
	/* Calculate its SID */
	l__retval |= SIDTYPE_THREAD;
	
	/* Create a new thread descriptor */
	if ((l__dadr = kinfo_new_descr(l__retval)) == NULL)
	{
		SET_ERROR(ERR_NOT_ENOUGH_MEMORY);
		return SID_PLACEHOLDER_INVALID;
	}
	
	/* Create the kernel stack of this thread */
	if ((l__kstack = ksched_new_stack()) == NULL)
	{
		kinfo_del_descr(l__retval);
		SET_ERROR(ERR_NOT_ENOUGH_MEMORY);
		return SID_PLACEHOLDER_INVALID;
	}
		
	/* 
	 * Configure the new thread
	 *
	 */
	l__descr[THRTAB_SID] = l__retval;
	
	l__descr[THRTAB_IS_USED] = 1;

	l__descr[THRTAB_PROCESS_SID] = current_t[THRTAB_PROCESS_SID];
	l__descr[THRTAB_SYNC_SID] = SID_PLACEHOLDER_NULL;
	l__descr[THRTAB_PROCESS_DESCR] = (uintptr_t)
					current_t[THRTAB_PROCESS_DESCR];
	l__descr[THRTAB_THRSTAT_FLAGS] =   THRSTAT_BUSY 
					 | THRSTAT_KERNEL_MODE 
					 | THRSTAT_FREEZED;
	/* 0xFFFFFFFF means "NO INTERRUPT HANDLED" */
	l__descr[THRTAB_IRQ_RECV_NUMBER] = 0xFFFFFFFF;
	l__descr[THRTAB_FREEZE_COUNTER] = 1;
	l__descr[THRTAB_RUNQUEUE_PREV] = 0;
	l__descr[THRTAB_RUNQUEUE_NEXT] = 0;
	l__descr[THRTAB_SOFTINT_LISTENER_SID] = 0;
	l__descr[THRTAB_EFFECTIVE_PRIORITY] = 0;
	/* The new thread inherits the priority and sched.-policy */
	l__descr[THRTAB_STATIC_PRIORITY] = current_t[THRTAB_STATIC_PRIORITY];
	l__descr[THRTAB_SCHEDULING_CLASS] = current_t[THRTAB_SCHEDULING_CLASS];
	
	l__descr[THRTAB_CUR_SYNC_QUEUE_PREV] = (uintptr_t)NULL;
	l__descr[THRTAB_CUR_SYNC_QUEUE_NEXT] = (uintptr_t)NULL;
	l__descr[THRTAB_OWN_SYNC_QUEUE_BEGIN] = (uintptr_t)NULL;
	
	l__descr[THRTAB_KERNEL_STACK_ADDRESS] = (uintptr_t)l__kstack;	

	/* No timeout settings */
	l__descr[THRTAB_TIMEOUT_LOW] = 0;
	l__descr[THRTAB_TIMEOUT_HIGH] = 0;
	l__descr[THRTAB_TIMEOUT_QUEUE_NEXT] = (uintptr_t)NULL;
	l__descr[THRTAB_TIMEOUT_QUEUE_PREV] = (uintptr_t)NULL;
		
	/* Set the address of the TLS */
	l__descr[THRTAB_X86_TLS_PHYS_ADDRESS] = ((uintptr_t)l__dadr);
	
	/* Set the time of creation */
	l__descr[THRTAB_UNIQUE_ID] = ksubj_next_unique_thread_id ++;	
	
	/* Set up memory access flags */
	THREAD(l__retval, THRTAB_MEMORY_OP_SID) = SID_PLACEHOLDER_NULL;
	THREAD(l__retval, THRTAB_MEMORY_OP_DESTADR) = 0;
	THREAD(l__retval, THRTAB_MEMORY_OP_MAXSIZE) = 0;
	THREAD(l__retval, THRTAB_MEMORY_OP_ALLOWED) = 0;	
	
	/* 
	 * Set EIP, ESP and EFLAGS of the new thread using the kernel stack 
	 *
	 */
	l__descr[THRTAB_X86_KERNEL_POINTER] = 
		ksched_init_stack( (  l__descr[THRTAB_KERNEL_STACK_ADDRESS] 
				     + KERNEL_STACK_SIZE
				   ),
		      	    	  eip,
		      	    	  esp,
		      	    	  KSCHED_USER_MODE
		      	 	 );	
					 			 
	/* Return its SID */
	return l__retval;					 
}


/*
 * sysc_create_thread(eip, esp)
 *
 * (Implementation of the "create_thread" system call)
 *
 * Creates a new thread. The new thread inherits the access list
 * the process membership, the static priority and the scheduling
 * class of the thread that created it. The new thread will created
 * as "frozen thread", so you have to awake it with "awake_subject"
 * before it will do anything.
 *
 * Parameters:
 *	eip	instruction pointer of the new thread
 *	esp	stack pointer of the new thread
 *
 * Return-Value:
 *	SID of the new thread
 *
 */
sid_t sysc_create_thread(uintptr_t eip, uintptr_t esp)
{
	/* Create the new thread */
	sid_t l__retval = ksubj_create_thread(eip, esp);
		
	if (l__retval > 0)
	{
		/* Setup other thread settings */
		THREAD(l__retval, THRTAB_PROCESS_SID) = 
				current_t[THRTAB_PROCESS_SID];
		
		/* Add to thread list of current process */
		uint32_t *l__next = (void*)(uintptr_t)
				current_t[THRTAB_NEXT_THREAD_OF_PROC];
		
		THREAD(l__retval, THRTAB_NEXT_THREAD_OF_PROC) =
					(uintptr_t)l__next;
		THREAD(l__retval, THRTAB_PREV_THREAD_OF_PROC) =
					(uintptr_t)&current_t;
		current_t[THRTAB_NEXT_THREAD_OF_PROC] = 
					(uintptr_t)&THREAD(l__retval, 0);
		
		if (l__next != NULL)
		{
			l__next[THRTAB_PREV_THREAD_OF_PROC] =
					(uintptr_t)&THREAD(l__retval, 0);
		}
		
		/* Setup process settings */
		current_p[PRCTAB_THREAD_COUNT] += 1;
	}
	
	return l__retval;
}
 
/*
 * sysc_create_process(eip, esp)
 *
 * (Implementation of the "create_process" system call)
 *
 * Creates a new process and a new thread for it. The new thread
 * will be started as a frozen thread, so you have to start it
 * with "awake_subject".
 * The new thread will inherit the access list, the priority
 * and the scheduling policy of the thread that called 
 * 'create_process'.
 * The new process will inherit the group list of its creator.
 * The new thread gets the controller thread of the new process.
 *
 * Parameters:
 *	eip	instruction pointer of the new thread
 *	esp	stack pointer of the new thread
 *
 * Return value:
 *	SID of the new process.
 *
 */
sid_t sysc_create_process(uintptr_t eip, uintptr_t esp)
{
	uint32_t *l__descr = NULL;
	uint32_t *l__pdir = NULL;
	sid_t l__retval = 0;
	sid_t l__thread = 0;
	
	/* Search an unused SID */
	l__descr = process_tab;
	
	while (l__retval < PROCESS_MAX)
	{
		l__descr = &PROCESS(l__retval, 0);
		if (!l__descr[PRCTAB_IS_USED]) break;
		l__retval ++;
	}
	
	/* No descriptor found */
	if (l__descr[PRCTAB_IS_USED]) 
	{
		SET_ERROR(ERR_NOT_ENOUGH_MEMORY);
		return SID_PLACEHOLDER_INVALID;
	}
	
	/* Calculate its SID */
	l__retval |= SIDTYPE_PROCESS;
	
	/* Create a new virtual address space */
	l__pdir = kmem_create_space();
	if (l__pdir == NULL)
	{
		SET_ERROR(ERR_NOT_ENOUGH_MEMORY);
		return SID_PLACEHOLDER_INVALID;
	}
	
	/* Create a new process descriptor */
	if (kinfo_new_descr(l__retval) == NULL)
	{
		kmem_destroy_space(l__pdir);
		SET_ERROR(ERR_NOT_ENOUGH_MEMORY);
		return SID_PLACEHOLDER_INVALID;
	}
	
	/* Create a new thread */
	l__thread = ksubj_create_thread(eip, esp);
		
	if (l__thread == SID_PLACEHOLDER_INVALID)
	{
		kinfo_del_descr(l__retval);
		SET_ERROR(ERR_NOT_ENOUGH_MEMORY);
		return SID_PLACEHOLDER_INVALID;
	}
	
	/* The new thread will allow the current thread to access on it */
	THREAD(l__thread, THRTAB_PROCESS_SID) = l__retval;
	THREAD(l__thread, THRTAB_PROCESS_DESCR) = (uintptr_t)l__descr;
	THREAD(l__thread, THRTAB_MEMORY_OP_SID) = current_t[THRTAB_SID];
	THREAD(l__thread, THRTAB_MEMORY_OP_DESTADR) = 0x0;
	THREAD(l__thread, THRTAB_MEMORY_OP_MAXSIZE) = 0xBFFFD000 / 4096;	
	THREAD(l__thread, THRTAB_MEMORY_OP_ALLOWED) = ALLOW_MAP | ALLOW_UNMAP;
	THREAD(l__thread, THRTAB_PREV_THREAD_OF_PROC) = 0;
	THREAD(l__thread, THRTAB_NEXT_THREAD_OF_PROC) = 0;
	
	/*
	 * Configure the new process
	 *
	 */
	l__descr[PRCTAB_IS_USED] = 1;
	l__descr[PRCTAB_IS_DEFUNCT] = 0;
	l__descr[PRCTAB_SID] = l__retval;
	l__descr[PRCTAB_CONTROLLER_THREAD_SID] = l__thread;
	l__descr[PRCTAB_CONTROLLER_THREAD_DESCR] = (uintptr_t)
			&THREAD(l__thread, 0);
	l__descr[PRCTAB_PAGE_COUNT] = 0;
	l__descr[PRCTAB_PAGEDIR_PHYSICAL_ADDR] = (uintptr_t)l__pdir;
	l__descr[PRCTAB_IS_PAGED] = 0;
	l__descr[PRCTAB_IS_ROOT] = current_p[PRCTAB_IS_ROOT];
	l__descr[PRCTAB_THREAD_COUNT] = 1;
	l__descr[PRCTAB_THREAD_LIST_BEGIN] = (uintptr_t)&THREAD(l__thread, 0);
	
	/* Set the time of creation */
	l__descr[PRCTAB_UNIQUE_ID] = ksubj_next_unique_process_id ++;		
	
	
	if (l__descr[PRCTAB_IS_ROOT])
		l__descr[PRCTAB_IO_ACCESS_RIGHTS] |=    IO_ALLOW_IRQ
						      | IO_ALLOW_PORTS;
	
	return l__retval;
}

/*
 * sysc_set_controller(sid)
 *
 * (Implementation of the "set_controller" system call)
 *
 * Changes the SID of the controller thread of the current
 * process. The new controller thread have to be a member of 
 * the current process.
 *
 * Parameters:
 *	sid	SID of the new controller thread
 *
 */
void sysc_set_controller(sid_t sid)
{
	if (!kinfo_isthrd(sid))
	{
		SET_ERROR(ERR_INVALID_SID);
		return;
	}
	
	if (THREAD(sid, THRTAB_PROCESS_SID) != current_p[PRCTAB_SID])
	{
		SET_ERROR(ERR_INVALID_SID);
		return;
	}
		      		      
	current_p[PRCTAB_CONTROLLER_THREAD_SID] = sid;
	current_p[PRCTAB_CONTROLLER_THREAD_DESCR] = (uintptr_t)&THREAD(sid, 0);

		 
	return;
}

/*
 * ksubj_kill_proc(proc);
 *
 * Destroys the process 'proc', by
 *	- Freeing its page tables
 *	- Freeing its page directory
 *	- Destroying its descriptor
 *
 * Parameters:
 *	proc	Pointer to the process descriptor
 *
 */
static void ksubj_kill_proc(uint32_t *proc)
{
	unsigned l__i = 1024;
	uint32_t *l__pdir = (void*)(uintptr_t)
				proc[PRCTAB_PAGEDIR_PHYSICAL_ADDR];	
	
	/* Search existing page tables within the page directory */
	while (l__i --)
	{
		if (l__pdir[l__i] & GENFLAG_PRESENT)
		{
			void *l__ptab = (void*)(uintptr_t)
					(l__pdir[l__i] & (~0xFFFu));
		
			kmem_free_kernel_pageframe(l__ptab);
			l__pdir[l__i] = 0;
		}
	}

	/* Free the page directory */
	kmem_destroy_space(l__pdir);
	
	/* Destroy the process descriptor */
	kinfo_del_descr(proc[PRCTAB_SID]);
	
	return;
}

/*
 * sysc_destroy_subject(sid)
 *
 * (Implementation of the "destroy_subject system call)
 *
 * Destroys a subject.
 *
 * Killing a thread subject means:
 * -------------------------------
 * If a thread subject will be killed, the kernel will
 * destroy its kernel stack, its descriptor and its
 * thread local storage. The thread will be removed from
 * the runqueue of course. 
 * A thread can be only get killed by a thread of the
 * same process or by a thread which has root access rights.
 * Also a thread can't kill itself. If the thread is the
 * process' controller thread, the controller thread of the
 * process will be set to the SID "invalid".
 *
 * Killing a process subject means:
 * --------------------------------
 * If a process should be killed it will only be stopped.
 * All threads of the process would be deactivated and stopped
 * for ever by setting PRCTAB_IS_DEFUNCT to 1. The process
 * self will be only destroyed if its last thread was
 * destroyed. 
 * During the destruction of a process, only its page
 * directories and page tables would be freed. It is the
 * object of the user level to free the page frames of the
 * process first.
 * To prevent the locking of too much memory, a process
 * could be only set into the Defunct state by a root
 * process. Even also its last thread could be only
 * killed by a root process, because a thread couldn't
 * kill itself. So a process manager e.g. can initialize
 * the freeing of a process' page frames using the paging
 * daemon.
 *
 * Parameters:
 *	sid	SID of the affected subject
 *
 */
void sysc_destroy_subject(sid_t sid)
{
	/*
	 * Killing a process...
	 *
	 */
	if ((sid & SIDTYPE_PROCESS) == SIDTYPE_PROCESS)
	{
		/* Only root processes may kill other processes */
		if (!current_p[PRCTAB_IS_ROOT])
		{
			SET_ERROR(ERR_ACCESS_DENIED);
			return;
		}
		
		/* We are not allowed to commit suicide */
		if (sid == current_p[PRCTAB_SID])
		{
			SET_ERROR(ERR_ACCESS_DENIED);
			return;
		}
		
		/* Test the selected SID */
		if (!kinfo_isproc(sid))
		{
			SET_ERROR(ERR_INVALID_SID);
			return;
		}
		
		/* It is not possible to kill PageD */
		if (PROCESS(sid, PRCTAB_IS_PAGED) == 1)
		{
			SET_ERROR(ERR_INVALID_SID);
			return;
		}
		
		/* Just stop it */
		PROCESS(sid, PRCTAB_IS_DEFUNCT) = 1;
		
		/* Remove all of its threads from the runque and freeze them */
		uint32_t *l__thread = (void*)(uintptr_t)
				PROCESS(sid, PRCTAB_THREAD_LIST_BEGIN);
		
		while (l__thread != NULL)
		{
			ksched_stop_thread(l__thread);
			l__thread[THRTAB_THRSTAT_FLAGS] 
			                 |= THRSTAT_PROC_DEFUNC;	
	
			l__thread = (void*)(uintptr_t)
				l__thread[THRTAB_NEXT_THREAD_OF_PROC];
		}
		
		return;
	}
	
	/*
	 * Killing a thread...
	 *
	 */
	if ((sid & SIDTYPE_THREAD) == SIDTYPE_THREAD)
	{
		uint32_t *l__process;
		uint32_t *l__thread;
		
		if (!kinfo_isthrd(sid))
		{
			SET_ERROR(ERR_INVALID_SID);
			return;
		}
		
		l__thread = &THREAD(sid, 0);
		l__process = &PROCESS(THREAD(sid, THRTAB_PROCESS_SID), 0);
		
		/* A thread can't kill itself */
		if (sid == current_t[THRTAB_SID])
		{
			SET_ERROR(ERR_INVALID_SID);
			return;
		}
		
		/*
		 * It is only allowed to kill threads of the same process,
		 * if we are not root
		 */
		if (    (current_t[THRTAB_PROCESS_SID] != l__thread[THRTAB_PROCESS_SID])
		     && (!current_p[PRCTAB_IS_ROOT])
		   )
		{
			SET_ERROR(ERR_ACCESS_DENIED);
			return;
		}

		/* At first, remove it from the runqueue */
		ksched_stop_thread(l__thread);
		
		/* Then remove it from the process thread list */
		if (l__thread[THRTAB_NEXT_THREAD_OF_PROC])
		{
			uint32_t *l__next = (void*)(uintptr_t)
				l__thread[THRTAB_NEXT_THREAD_OF_PROC];
				
			l__next[THRTAB_PREV_THREAD_OF_PROC] =
				l__thread[THRTAB_PREV_THREAD_OF_PROC];
		}
		
		if (l__thread[THRTAB_PREV_THREAD_OF_PROC])
		{
			uint32_t *l__prev = (void*)(uintptr_t)
				l__thread[THRTAB_PREV_THREAD_OF_PROC];
				
			l__prev[THRTAB_NEXT_THREAD_OF_PROC] =
				l__thread[THRTAB_NEXT_THREAD_OF_PROC];
		}
		
		if (    l__process[PRCTAB_THREAD_LIST_BEGIN] 
		    ==  (uintptr_t)l__thread
		   )
		{
			l__process[PRCTAB_THREAD_LIST_BEGIN] = (uintptr_t)
				l__thread[THRTAB_NEXT_THREAD_OF_PROC];
		}
		
		/* Are we our process' controller thread? */
		if (l__thread[THRTAB_SID] == l__process[PRCTAB_CONTROLLER_THREAD_SID])
		{
			/* Set controller therad to invalid */
			l__process[PRCTAB_CONTROLLER_THREAD_SID] = SID_PLACEHOLDER_INVALID;
			l__process[PRCTAB_CONTROLLER_THREAD_DESCR] = 0;
		}
		
		/* Is any thread waiting for us? */
		if (l__thread[THRTAB_OWN_SYNC_QUEUE_BEGIN])
		{
			uint32_t *l__queue = (void*)(uintptr_t)
				l__thread[THRTAB_OWN_SYNC_QUEUE_BEGIN];
								
			while (l__queue != NULL)
			{
				/* Interrupt the other side */
				ksync_interrupt_other(l__queue);
				l__queue = (void*)(uintptr_t)
					l__queue[THRTAB_CUR_SYNC_QUEUE_NEXT];
			}
		
		}

		/* Is the killed thread part of a wait queue? */
		if (    (l__thread[THRTAB_CUR_SYNC_QUEUE_NEXT] != 0)
		     || (l__thread[THRTAB_CUR_SYNC_QUEUE_PREV] != 0)
		   )
		{
			ksync_removefrom_waitqueue_error(
				&THREAD(l__thread[THRTAB_SYNC_SID], 0),
				l__thread
							);
		}
		
		/* Decrement the thread counter of the process */
		l__process[PRCTAB_THREAD_COUNT] --;		
		
		/* Last thread? Kill the process, too. */
		if (l__process[PRCTAB_THREAD_COUNT] == 0)
		{
			ksubj_kill_proc(l__process);
		}
		
		/* Free the threads kernel stack */
		ksched_del_stack((void*)(uintptr_t)
				 	l__thread[THRTAB_KERNEL_STACK_ADDRESS]
				 );
		
		/* Is it waiting for/handling IRQs? */
		if (l__thread[THRTAB_IRQ_RECV_NUMBER] != 0xFFFFFFFF)
		{
			/* Re-enable it */
			kio_reenable_irq(l__thread[THRTAB_IRQ_RECV_NUMBER]);
		}
		
		/* Is there a thread listening to our softints? */
		if (kinfo_isthrd(l__thread[THRTAB_SOFTINT_LISTENER_SID]))
		{
			uint32_t *l__waitthr = (void*)(uintptr_t)
			  &THREAD(l__thread[THRTAB_SOFTINT_LISTENER_SID], 0);

			l__waitthr[THRTAB_RECEIVED_SOFTINT] = 0xFFFFFFFF;
			  			
			/* Remove the timeout */
			if (l__waitthr[THRTAB_THRSTAT_FLAGS] & THRSTAT_TIMEOUT)
			{
				ksched_del_timeout(l__waitthr);
				l__waitthr[THRTAB_THRSTAT_FLAGS] &= (~THRSTAT_TIMEOUT);
			}
		
			ksched_start_thread(l__waitthr);
		}

		/* Is the thread listenting to the softints of sb. else? */
		if (kinfo_isthrd(l__thread[THRTAB_RECV_LISTEN_TO]))
		{
			uint32_t *l__other = (void*)(uintptr_t)
				l__thread[THRTAB_RECV_LISTEN_TO];
			
			/* Clear Softint listening information */
			l__other[THRTAB_SOFTINT_LISTENER_SID] = 0;
		}
    
		/* Did we set any timeout? */
		if (l__thread[THRTAB_THRSTAT_FLAGS] & THRSTAT_TIMEOUT)
		{
			ksched_del_timeout(l__thread);
		}

		/* Destroy its descriptor */
		kinfo_del_descr(sid);
		
		/* Okay */
	}

	return;
}


/*
 * ksysc_create_idle
 *
 * Creates the idle process and thread
 *
 */
int ksysc_create_idle()
{
	sid_t l__sid = 0;

	#ifdef DEBUG_MODE
	kprintf("Creating idle...");
	#endif
	
	/* Set process 0 as "idle process" and "idle thread" */
	current_p = &process_tab[0];
	current_t = &thread_tab[0];

	/* Create the idle process */
	l__sid = sysc_create_process(0, 0);
	
	/* Set memeberships of the idle process to everybody and root */
	
	#ifdef DEBUG_MODE
	kprintf("idle task created with SID: 0x%X\n",(long)l__sid);
	#endif
	
	PROCESS(l__sid, PRCTAB_IS_ROOT) = 1;
	
	ksched_idle_thread = 
		&THREAD(PROCESS(l__sid, PRCTAB_CONTROLLER_THREAD_SID), 0);
	
	ksched_idle_thread[THRTAB_X86_KERNEL_POINTER] = 
		ksched_init_stack( (  ksched_idle_thread[THRTAB_KERNEL_STACK_ADDRESS] 
				     + KERNEL_STACK_SIZE
				   ),
				  (uintptr_t)&ksched_idle_loop,
		      	    	  0,
		      	    	  KSCHED_KERNEL_MODE
		      	 	 );	
		
	/* Unfreeze it and change its priority */
	sysc_set_priority(
			  PROCESS(l__sid, PRCTAB_CONTROLLER_THREAD_SID),
			  0, 
			  0
			 );
	sysc_awake_subject(PROCESS(l__sid, PRCTAB_CONTROLLER_THREAD_SID));
	
	/* We've to reinitialize the run queue */
	ksched_idle_thread[THRTAB_RUNQUEUE_PREV] = (uintptr_t)NULL;
	ksched_idle_thread[THRTAB_RUNQUEUE_NEXT] = (uintptr_t)NULL;
	
	return 0;
}

/*
 * ksysc_create_init()
 *
 * Loads and initializes the init process
 *
 */
int ksysc_create_init()
{
	sid_t l__initsid = 0;
	
	#ifdef DEBUG_MODE
	kprintf("Creating init...");
	#endif

	/* Create the init process */
	l__initsid = sysc_create_process(0x1000, 0x1000);
	
	#ifdef DEBUG_MODE
	kprintf("Init created with SID: 0x%X\n", (long)l__initsid);
	#endif

	/* Map the init module buffer into the address space of init */
	unsigned l__pages = ((  
	                        grub_modules_list[0].end 
	                      - grub_modules_list[0].start
			     ) / 4096);
	
	if (
	     (   grub_modules_list[0].end 
	       - grub_modules_list[0].start
	     ) % 4096
	   )
	{
	 	l__pages ++;
	}
			
	uint32_t l__start = grub_modules_list[0].start;
	
	#ifdef DEBUG_MODE
	kprintf("Switching into the init address space...");
	#endif
	
	ksched_switch_space(l__initsid);
	
	#ifdef DEBUG_MODE
	kprintf("( DONE )\n");
	#endif
	
	#ifdef DEBUG_MODE
	kprintf("Map init image from 0x%X (%i pages = %i Byte) to 0x1000...", 
		l__start,
		l__pages,
		  grub_modules_list[0].end 
	        - grub_modules_list[0].start
		);
	#endif

	kmem_map_page_frame_cur(l__start, 
		  	        0x1000, 
			    	l__pages,
			    	  GENFLAG_PRESENT
				| GENFLAG_USER_MODE
				| GENFLAG_READABLE
				| GENFLAG_WRITABLE
				| GENFLAG_EXECUTABLE
			   );
			   
	/* Change page usage counter */
	uint32_t l__padr = l__start;
	uint32_t l__cnt = l__pages;
	
	while (l__cnt --)
	{
		PAGE_BUF_TAB(l__padr).usage ++;
		
		l__padr += 4096;
	}
			   
	#ifdef DEBUG_MODE
	kprintf("( DONE )");
	#endif
	
	/* Unfreeze it and change its priority */
	sysc_set_priority(
			  PROCESS(l__initsid, PRCTAB_CONTROLLER_THREAD_SID),
			  20, 
			  0
			 );
	sysc_awake_subject(PROCESS(l__initsid, PRCTAB_CONTROLLER_THREAD_SID));
	
	/* Setup the init registers */
	sysc_write_regs(PROCESS(l__initsid, PRCTAB_CONTROLLER_THREAD_SID), 
		        REGS_X86_GENERIC, 
		     	0x1000,
		        l__pages,
		        0,
		        0
		       );
	
	#ifdef DEBUG_MODE
	kprintf(" - Init started.\n");
	#endif
			
	return 0;
}

