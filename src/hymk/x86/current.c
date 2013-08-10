/*
 *
 * current.h
 *
 * (C)2001, 2002 by Friedrich Gr�ter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying').  
 *
 * Current status and scheduling functions
 *
 */
#include <hydrixos/types.h>
#include <mem.h>
#include <info.h>
#include <sched.h>
#include <current.h>
#include <page.h>
#include <stdio.h>

uint32_t i386_initial_kernel_stack = 0;

/*
 * ksched_switch_fpu_state(old, new)
 *
 * Saves the content of the current FPU stack to the
 * thread status of 'old' and loads the new FPU stack
 * from the thread status of 'new'.
 *
 */
extern long i387_fsave;

static inline void ksched_switch_fpu_state(uint32_t *old, uint32_t *new)
{
	switch (i387_fsave)
	{
		/* Save and restore FPU stack only */
		case (1):
			__asm__ __volatile__("fsave %0\n\t"
				     	     "fwait\n\t"
				     	     "frstor %1\n\t"
				     	     : "=m"(old[THRTAB_X86_FPU_STACK]),
				     	       "=m"(new[THRTAB_X86_FPU_STACK])
				     	     :
				     	     :"memory"
				     	    );
			break;
	
		/* Save and restore FPU, MMX and SSE etc. */
		case (2):
			__asm__ __volatile__("fxsave %0\n\t"
				     	     "fclex\n\t"
				     	     "fxrstor %1\n\t"
				     	     : "=m"(old[THRTAB_X86_FPU_STACK]),
				     	       "=m"(new[THRTAB_X86_FPU_STACK])
				     	     :
				     	     :"memory"
				     	    );
	}
}


/*
 * ksched_set_local_storage(sid)
 *
 * Switches the thread local storage (TLS) of the current
 * virtual address spaceto the TLS of the TLS of the current
 * thread.
 *
 */
static inline void ksched_set_local_storage(void)
{
	/* Remap the page */
	kmem_map_page_frame_cur(current_t[THRTAB_X86_TLS_PHYS_ADDRESS], 
		  	        VAS_THREAD_LOCAL_STORAGE, 
			    	1,
			    	  GENFLAG_PRESENT
				| GENFLAG_READABLE
				| GENFLAG_WRITABLE
				| GENFLAG_USER_MODE
			   );	
	
	return;
}

/* 
 * ksched_next_thread()
 *
 * Changes the current thread and process context to the next
 * one of the run queue if "ksched_change_thread == true".
 * 
 */
void ksched_next_thread(void)
{
	uint32_t *l__next = NULL;		/* The next thread */
		
	/* Is there a need of a thread switch? */
	if (!ksched_change_thread) return;	/* If not, return */

	/* Are we at the end of the run queue? */	
	if (current_t[THRTAB_RUNQUEUE_NEXT] == NULL)
	{
		if (ksched_active_threads == 1)
		{
			/*
		 	 * If no (=one) thread is active, the idle thread will
		 	 * be executed.
		 	 *
			 */
			l__next = (void*)(uintptr_t)
					ksched_idle_thread;
		}
		 else
		{
			/*
			 * Else we go to the begin of the runqueue again...
			 *
			 */
			l__next = (void*)(uintptr_t)
					ksched_idle_thread[THRTAB_RUNQUEUE_NEXT];
		}	
	}	
	 else
	{
		/* 
		 * Execute the next thread if we are somewhere within the
		 * runqueue
		 */
		l__next = (void*)(uintptr_t)
				current_t[THRTAB_RUNQUEUE_NEXT];
	}

	i386_new_stack_pointer =
		&l__next[THRTAB_X86_KERNEL_POINTER];
	i386_old_stack_pointer =
		&current_t[THRTAB_X86_KERNEL_POINTER];

	/* Switch the current address space */
	if (l__next[THRTAB_PROCESS_SID] != current_t[THRTAB_PROCESS_SID]) 
	{
		ksched_switch_space(l__next[THRTAB_PROCESS_SID]);
		current_p = &PROCESS(l__next[THRTAB_PROCESS_SID], 0);
		main_info[MAININFO_CURRENT_PROCESS] = l__next[THRTAB_PROCESS_SID];
	}

	/* Save the FPU stack */
	ksched_switch_fpu_state(current_t, l__next);

	/* Switch the current stack */	
//kprintf("SWITCH: 0x%X => 0x%X :", current_t[THRTAB_SID], l__next[THRTAB_SID]);
	current_t = l__next;

	/* Map the new local storage */
	ksched_set_local_storage();

	/* Update the main info page */
	main_info[MAININFO_CURRENT_THREAD] = l__next[THRTAB_SID];
	
	main_info[MAININFO_PROC_TABLE_ENTRY] = 
		((uintptr_t)&PROCESS(l__next[THRTAB_PROCESS_SID], 0))
		+ 0xC0000000;
	
	main_info[MAININFO_THRD_TABLE_ENTRY] = 
		((uintptr_t)l__next)
		+ 0xC0000000;

	/* Pointer to the effective priority buffer */
	kinfo_eff_prior = &current_t[THRTAB_EFFECTIVE_PRIORITY];
	
	/* Pointer to the buffer with I/O permissions */
	kinfo_io_map = &current_p[PRCTAB_IO_ACCESS_RIGHTS];

	/* Refresh the thread's effective priority if needed */
	if (*kinfo_eff_prior == 0)
		*kinfo_eff_prior = (current_t[THRTAB_STATIC_PRIORITY] * 2);
//kprintf(" %i\n", *kinfo_eff_prior);
	return;
}

/*
 * ksched_enter_main_loop
 *
 * This function enters the scheduling loop of the system
 *
 */
void ksched_enter_main_loop(void)
{
	/* Select the first thread */
	ksched_change_thread = true;
	ksched_next_thread();
	
	i386_old_stack_pointer = &i386_initial_kernel_stack;
	
	/* Leave the initial kernel stack */
	__asm__ __volatile__(".extern i386_do_context_switch\n"
	    		     "movl %%eax, %%esp\n"
	    		     "jmp i386_do_context_switch\n"
	    		     :
	    		     : "a" (*i386_new_stack_pointer)
	    		    );
	/* Never reached */
}
