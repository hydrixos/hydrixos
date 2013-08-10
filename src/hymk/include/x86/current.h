/*
 *
 * current.h
 *
 * (C)2001, 2005 by Friedrich Gr�ter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying').  
 *
 * Current status and scheduling functions
 *
 */
#ifndef _CURRENT_H
#define _CURRENT_H

#include <hydrixos/types.h>
#include <mem.h>
#include <info.h>
#include <sched.h>

#include <stdio.h>

/*
#define GET_FRAME_STACK_FRAME_RETURN_ESP	\
	(*((long*)(((uintptr_t)i386_saved_last_block) + 60)))
#define GET_FRAME_STACK_FRAME_RETURN_EIP	\
	(*((long*)(((uintptr_t)i386_saved_last_block) + 48)))
*/

#define GET_CURRENT_STACK_FRAME_RETURN_ESP	\
	(*((long*)(((uintptr_t)current_t->cframe) + 60)))
#define GET_CURRENT_STACK_FRAME_RETURN_EIP	\
	(*((long*)(((uintptr_t)current_t->cframe) + 48)))

extern void i386_yield_kernel_thread(void);
#define SYSCALL_YIELDS_THREAD()		i386_yield_kernel_thread()

/*
 * ksched_switch_space(pid)
 *
 * Switches the current address space to the process
 * that has the SID 'pid'. This function won't test
 * the availability of the process 'pid'.
 *
 */
static inline void ksched_switch_space(sid_t pid)
{
	/* Load the address of the new pagedir */
	i386_current_pdir = (void*)(uintptr_t)
		PROCESS(pid, PRCTAB_PAGEDIR_PHYSICAL_ADDR);
	
	/* Set the new page directory */
	__asm__ __volatile__("movl %%eax, %%cr3\n\t"
			     :
		     	     :"a" (i386_current_pdir) 
		     	     :"memory"
		     	    );
}

void ksched_next_thread(void);

#define KSCHED_TRY_RESCHED()	{if (ksched_change_thread) ksched_next_thread();}

/*
 * Counters of active resp. inactive threads
 *
 */
extern long ksched_active_threads;

#endif

