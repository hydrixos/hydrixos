/*
 *
 * i386 Task managment
 *
 * tss.c
 *
 * (C)2003 by Friedrich Gräter and others (see below)
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in
 * the file 'copying').
 *
 * TSS initialization
 *
 */ 
#include <hydrixos/types.h>
#include <stdio.h>
#include <setup.h>
#include <mem.h>
#include <sched.h>

/* Kernel TSS */
struct i386_tss_ps *i386_tss_struct;

/* The address of the ESP0 stack pointer */
uint32_t i386_esp0_ret;

/*
 * ksched_init_tss
 *
 * Initiailzes and loads the system TSS. 
 * Also it sets the pointer to the ESP0 stack pointer
 * that is part of the system TSS and is needed for 
 * entering the kernel mode.
 *
 */
int ksched_init_tss()
{
	#ifdef DEBUG_MODE
		kprintf("Loading system TSS...");
	#endif
	
	i386_tss_struct = (void*)(uintptr_t)kmem_alloc_kernel_pageframe();
	if (i386_tss_struct == NULL)
	{
		return -1;
	}

	/* We don't want an I/O-Map */
	i386_tss_struct->backl = 0x8000;		
	i386_tss_struct->esp0 = 0;
	i386_tss_struct->ss0 = DS_KERNEL;
	i386_tss_struct->esp1 = 0;
	i386_tss_struct->ss1 = DS_USER;
	i386_tss_struct->esp2 = 0;
	i386_tss_struct->ss2 = DS_USER | 0x3;
	i386_tss_struct->cr3 = (uintptr_t)ikp_start;
	i386_tss_struct->eip = 0;
	i386_tss_struct->eflags = 0;
	i386_tss_struct->eax = 0;
	i386_tss_struct->ecx = 0;
	i386_tss_struct->edx = 0;
	i386_tss_struct->ebx = 0;
	i386_tss_struct->esp = 0;
	i386_tss_struct->ebp = 0;
	i386_tss_struct->esi = 0;
	i386_tss_struct->edi = 0;
	i386_tss_struct->es = DS_USER;
	i386_tss_struct->cs = CS_USER;
	i386_tss_struct->ss = DS_USER;
	i386_tss_struct->ds = DS_USER;
	i386_tss_struct->fs = DS_USER;
	i386_tss_struct->gs = DS_USER;
	i386_tss_struct->ldt = 0;
	i386_tss_struct->t = 0;
	i386_tss_struct->io_base = 5000;
	
	i386_esp0_ret = (uintptr_t)&(i386_tss_struct->esp0);
	
	i386_gdt_s[7].base_l = (uintptr_t)i386_tss_struct + 0xC0000000;
	i386_gdt_s[7].base_lh = 
		(((uintptr_t)i386_tss_struct + 0xC0000000) >> 16) & 0xFF;	
	i386_gdt_s[7].base_h = 
		(((uintptr_t)i386_tss_struct + 0xC0000000) >> 24) & 0xFF;	
	
	/* Set it as TSS */
	__asm__ __volatile__("ltr %%ax":: "a" (TSS_SELECTOR):"memory");

	#ifdef DEBUG_MODE
		kprintf("( DONE )\n\n");
	#endif
		
	return 0;
}

