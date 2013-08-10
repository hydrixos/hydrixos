/*
 *
 * frame.c
 *
 * (C)2004 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g.
 * in the file 'copying').
 *
 * Kernel stack managment
 *
 */
#include <hydrixos/types.h>
#include <setup.h>
#include <mem.h>
#include <sched.h>
#include <stdio.h>

/*
 * ksched_new_stack
 *
 * Creates a new kernel stack and returns the pointer of it.
 *
 * NULL = Not enough memory.
 *
 */
void* ksched_new_stack(void)
{
	void *l__retval = (void*)(uintptr_t)kmem_alloc_kernel_pageframe();
		
	return l__retval;
}

/*
 * ksched_del_stack(frame)
 *
 * Deletes and frees the kernel stack 'frame'.
 *
 * NULL = Not enough memory.
 *
 */
void ksched_del_stack(void* frame)
{
	kmem_free_kernel_pageframe(frame);
	return;
}

/*
 * ksched_init_stack(new_sp, eip, mode, esp)
 *
 * Initializes the kernel stack frame 'new_sp' for the
 * instruction pointer 'eip' in mode 'mode'.
 * In user mode the parameter 'esp' will be used as
 * destination stack pointer. In kernel mode it will
 * be ignored.
 *
 *  MODE might be:
 *		KSCHED_KERNEL_MODE
 *		KSCHED_USER_MODE
 *
 * Returns the pointer to the end of the stack entry.
 *
 */
uintptr_t ksched_init_stack(uintptr_t nsp, 
		      	    uintptr_t eip,
		      	    uintptr_t esp,
		      	    int mode
		      	   )
{
	uint32_t *l__nsp = (void*)(nsp - (17 * 4));
	uintptr_t onsp = (nsp - (17 * 4));

	l__nsp[0] = 0;				/* GPRs*/
	l__nsp[1] = 0;
	l__nsp[2] = 0;
	l__nsp[3] = 0;
	l__nsp[4] = 0;
	l__nsp[5] = 0;
	l__nsp[6] = 0;
	l__nsp[7] = 0;
	if (mode == KSCHED_KERNEL_MODE)
	{
		l__nsp[8] = DS_KERNEL;		/* Segments */
		l__nsp[9] = DS_KERNEL;
		l__nsp[10] = DS_KERNEL;
		l__nsp[11] = DS_KERNEL;		
		l__nsp[12] = eip;		/* EIP */
		l__nsp[13] = CS_KERNEL;		/* CS */
		l__nsp[14] = 0;			/* Eflags: No interrupts allowed */	
	}
	 else if (mode == KSCHED_USER_MODE)
	{
		l__nsp[8] = DS_USER  | 0x3;
		l__nsp[9] = DS_USER  | 0x3;
		l__nsp[10] = DS_USER | 0x3;
		l__nsp[11] = DS_USER | 0x3;	
		l__nsp[12] = eip;		/* EIP */
		l__nsp[13] = CS_USER | 0x3;	/* CS */
		l__nsp[14] = 512;		/* Eflags: Interrupts allowed */	
		l__nsp[15] = esp;		/* ESP */
		l__nsp[16] = DS_USER | 0x3;	/* SS */
	}	
	
	return onsp;
}

/*
 * ksched_debug_stack
 *
 * Outputs the content of the current kernel stack 
 * before leaving the kernel mode
 *
 *
 #include <current.h>
void ksched_debug_stack(void)
{
	uint32_t *l__esp;

	l__esp = (uint32_t*)*i386_new_stack_pointer;
	
	kprintf("BEGIN OF STACK OUTPUT AT 0x%X\n", l__esp);
	kprintf("---------------------\n");
	kprintf("GS: 0x%X\n", l__esp[8]);
	kprintf("FS: 0x%X\n", l__esp[9]);
	kprintf("ES: 0x%X\n", l__esp[10]);
	kprintf("DS: 0x%X\n", l__esp[11]);
	kprintf("EIP: 0x%X\n", l__esp[12]);
	kprintf("CS: 0x%X\n", l__esp[13]);
	kprintf("EFLAGS: 0x%X\n", l__esp[14]);
	kprintf("ESP: 0x%X\n", l__esp[15]);
	kprintf("SS: 0x%X\n", l__esp[16]);
	kprintf("---------------------\n");
}
*/
