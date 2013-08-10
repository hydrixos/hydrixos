/*
 *
 * init.c
 *
 * (C)2004 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying'). 
 *
 * Kernel high-level startup code.
 *
 */
#include <stdio.h>
#include <kcon.h>
#include <setup.h>
#include <hydrixos/types.h>
#include <mem.h>
#include <info.h>
#include <sched.h>
#include <sysc.h>
#include <page.h>

static void kinit_failure(int num)
{
	kprintf("Failed to initialize the HydrixOS microkernel.\n");
	kprintf("Error Code: %i", num);
	while(1) ksched_do_panic();
}

int kdebug_no_boot_mode = 0;

int main(void)
{
	/*
	 * Kernel initialization
	 *
	 */
	/* Clear screen */
	kclrscr();
	
	/* Load GRUB modules list */
	if (kmod_init()) kinit_failure(0);

	/* Initialize the x86 CPU */
	if (kinfo_init_x86_cpu()) kinit_failure(1);	
		
	/* Initialize the kernel address space */
	if (kmem_init_tables()) kinit_failure(2);
	
	/* Switch to the kernel address space */
	if (kmem_switch_krnl_spc()) kinit_failure(3);
	
	/* Initialize the x87 FPU */
	if (kinfo_init_387_fpu()) kinit_failure(4);
	
	/* Initialize the main TSS */
	if (ksched_init_tss()) kinit_failure(5);
	
	/* Initialize the info pages */
	if (kinfo_init()) kinit_failure(6);

	/* Initialize the PIC, IDT and PIT */
	if (ksched_init_ints()) kinit_failure(7);
	
	/* Initialize the scheduler */
	if (ksysc_create_idle()) kinit_failure(8);
	
	/* Initialize the init process */
	if (ksysc_create_init()) kinit_failure(9);

	/* Kernel debugger: Leaving boot mode */
	kdebug_no_boot_mode = 1;
	
	/* Enter normal execution */
	ksched_enter_main_loop();
	
	/* Never reached. */
	while(1);
	
	return 0;
}
