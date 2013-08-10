/*
 *
 * intr.c
 *
 * (C)2004 by Friedrich Gr�ter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying').  
 *
 * Interrupt initialization and handling
 *
 */
#include <hydrixos/types.h>
#include <mem.h>
#include <stdio.h>
#include <setup.h>
#include <sched.h>
#include <current.h>
#include <info.h>
#include <hymk/x86-io.h>
#include <sysc.h>
#include <error.h>
#include <page.h>

/* PIC IRQ mask */
static uint32_t	ksched_irqmask;		

/* Calculate the IDT entry of an IRQ */
#define	IRQ(irqnr)			(0xA0 + irqnr)

#define _MASTER_SLAVE 			2
#define _SLAVE_IRQ 			8

/*
 * SoftINT redirection
 *
 */
int kremote_received(uint32_t intr);

/*
 * Kernel interrupt table
 *
 */
struct ksched_irqt_ps ksched_irqt_s[16];

/*
 * ksched_set_irq(irn, offs)
 *
 * Changes the interrupt handler of the IDT of the INT 'irn' 
 * to handle an IRQ with the function at offset 'offs'.
 *
 * Please remember that 'irn' is the number of the used
 * IDT entry (or software interrupt) and NOT the IRQ
 * number!
 *
 * Use the IRQ macro to calculate the IRQ number
 *
 */
static void ksched_set_irq(unsigned irn, uintptr_t offs)
{
	i386_idt_s[irn].offs_l = offs & 0xffff;
	i386_idt_s[irn].sel = 0x18;
	i386_idt_s[irn].wcount = 0;
	i386_idt_s[irn].access = 0x8E;
	i386_idt_s[irn].offs_h = ((offs & 0xffff0000) >> 16);

	return;
}

/*
 * ksched_set_sysc(irn, offs)
 *
 * Changes the interrupt handler of the IDT of the INT 'irn'
 * to handle an IRQ with the function at offset 'offs'.
 *
 */
static void ksched_set_sysc(unsigned irn, uintptr_t offs)
{
	i386_idt_s[irn].offs_l = offs & 0xffff;
	i386_idt_s[irn].sel = 0x18;
	i386_idt_s[irn].wcount = 0;
	i386_idt_s[irn].access = 0xEE;
	i386_idt_s[irn].offs_h = ((offs & 0xffff0000) >> 16);

	return;
}

/*
 * ksched_set_exc(irn, offs)
 *
 * Changes the interrupt handler of the IDT of the INT 'irn'
 * to handle an exception with the function at offset 'offs'.
 *
 */
static void ksched_set_exc(unsigned irn, uintptr_t offs)
{
	i386_idt_s[irn].offs_l = offs & 0xffff;
	i386_idt_s[irn].sel = 0x18;		
	i386_idt_s[irn].wcount = 0;
	i386_idt_s[irn].access = 0x8F;
	i386_idt_s[irn].offs_h = ((offs & 0xffff0000) >> 16);

	return;
}

/*
 * ksched_set_exc_usr(irn, offs)
 *
 * Changes the interrupt handler of the IDT of the INT 'irn'
 * to handle an exception with the function at offset 'offs',
 * that can be also called from user space.
 *
 */
static void ksched_set_exc_usr(unsigned irn, uintptr_t offs)
{
	i386_idt_s[irn].offs_l = offs & 0xffff;
	i386_idt_s[irn].sel = 0x18;		
	i386_idt_s[irn].wcount = 0;
	i386_idt_s[irn].access = 0xEE;
	i386_idt_s[irn].offs_h = ((offs & 0xffff0000) >> 16);

	return;
}

/*
 * ksched_disable_irq(irqnr)
 *
 * Disables the IRQ 'irqnr' on the PIC.
 *
 */
void ksched_disable_irq(irq_t irqnr)
{
	ksched_irqmask |= (1 << irqnr);

	if ((ksched_irqmask & 0xff00) == 0xff00) 
		ksched_irqmask |= (1 << _MASTER_SLAVE);

	outb(0x21, ksched_irqmask & 0xFF);
	outb(0xA1, (ksched_irqmask >> 8) & 0xFF);

	return;
}

/*
 * ksched_enable_irq(irqnr)
 *
 * Enables the IRQ 'irqnr' on the PIC.
 *
 */
void ksched_enable_irq(irq_t irqnr)
{
	ksched_irqmask &= ~(1 << irqnr);

	if(irqnr >= _SLAVE_IRQ) 
		ksched_irqmask &= ~(1 << _MASTER_SLAVE);

	outb(0x21, ksched_irqmask & 0xFF);
	outb(0xA1, (ksched_irqmask >> 8) & 0xFF);

	return;
}

/*
 * ksched_init_timer
 *
 * Initializes the system timer
 *
 */
static void ksched_init_timer(void)
{
	uint32_t l__tmp;
	uint32_t l__freq = TIMER_FREQUENCY;

	l__tmp = (1193182 + (l__freq / 2)) / l__freq;

        outb(0x43, 0x34); 
        outb(0x40, (uint8_t) (l__tmp & 0xFF));
        outb(0x40, (uint8_t) ((l__tmp & 0xFF00) >> 8));
        		
	return;
}

/*
 * ksched_init_ints
 *
 * Initializes the PIC and the IDT.
 *
 *
 */
int ksched_init_ints(void)
{
	irq_t	l__irqn = 16;
	unsigned l__i = 256;
	
	#ifdef DEBUG_MODE
		kprintf("Initializing the IDT...");
	#endif
	/*
	 * Init IDT
	 *
	 */
	/* Inactive software interrupts */
	while (l__i --)
	ksched_set_sysc(l__i, (uintptr_t)&i386_emptyint_handler);
	
	/* System calls */
	ksched_set_sysc(0xC0, (uintptr_t)&i386_sysc_alloc_pages);
	
	ksched_set_sysc(0xC1, (uintptr_t)&i386_sysc_create_thread);
	ksched_set_sysc(0xC2, (uintptr_t)&i386_sysc_create_process);
	ksched_set_sysc(0xC3, (uintptr_t)&i386_sysc_set_controller);
	ksched_set_sysc(0xC4, (uintptr_t)&i386_sysc_destroy_subject);

	ksched_set_sysc(0xC5, (uintptr_t)&i386_sysc_chg_root);

	ksched_set_sysc(0xC6, (uintptr_t)&i386_sysc_freeze_subject);
	ksched_set_sysc(0xC7, (uintptr_t)&i386_sysc_awake_subject);
	ksched_set_sysc(0xC8, (uintptr_t)&i386_sysc_yield_thread);
	ksched_set_sysc(0xC9, (uintptr_t)&i386_sysc_set_priority);

	ksched_set_sysc(0xCA, (uintptr_t)&i386_sysc_allow);
	ksched_set_sysc(0xCB, (uintptr_t)&i386_sysc_map);
	ksched_set_sysc(0xCC, (uintptr_t)&i386_sysc_unmap);
	ksched_set_sysc(0xCD, (uintptr_t)&i386_sysc_move);

	ksched_set_sysc(0xCE, (uintptr_t)&i386_sysc_sync);

	ksched_set_sysc(0xCF, (uintptr_t)&i386_sysc_io_allow);
	ksched_set_sysc(0xD0, (uintptr_t)&i386_sysc_io_alloc);
	ksched_set_sysc(0xD1, (uintptr_t)&i386_sysc_recv_irq);

	ksched_set_sysc(0xD2, (uintptr_t)&i386_sysc_recv_softints);
	ksched_set_sysc(0xD3, (uintptr_t)&i386_sysc_read_regs);
	ksched_set_sysc(0xD4, (uintptr_t)&i386_sysc_write_regs);

	ksched_set_sysc(0xD5, (uintptr_t)&i386_sysc_set_paged);	 
	ksched_set_sysc(0xD6, (uintptr_t)&i386_sysc_test_page);	 
	
	/* IRQs */
	ksched_set_irq(IRQ(0x0), (uintptr_t)&i386_irqhandleasm_0);
	ksched_set_irq(IRQ(0x1), (uintptr_t)&i386_irqhandleasm_1);
	ksched_set_irq(IRQ(0x2), (uintptr_t)&i386_irqhandleasm_2);
	ksched_set_irq(IRQ(0x3), (uintptr_t)&i386_irqhandleasm_3);
	ksched_set_irq(IRQ(0x4), (uintptr_t)&i386_irqhandleasm_4);
	ksched_set_irq(IRQ(0x5), (uintptr_t)&i386_irqhandleasm_5);
	ksched_set_irq(IRQ(0x6), (uintptr_t)&i386_irqhandleasm_6);
	ksched_set_irq(IRQ(0x7), (uintptr_t)&i386_irqhandleasm_7);
	ksched_set_irq(IRQ(0x8), (uintptr_t)&i386_irqhandleasm_8);
	ksched_set_irq(IRQ(0x9), (uintptr_t)&i386_irqhandleasm_9);
	ksched_set_irq(IRQ(0xA), (uintptr_t)&i386_irqhandleasm_10);
	ksched_set_irq(IRQ(0xB), (uintptr_t)&i386_irqhandleasm_11);
	ksched_set_irq(IRQ(0xC), (uintptr_t)&i386_irqhandleasm_12);
	ksched_set_irq(IRQ(0xD), (uintptr_t)&i386_irqhandleasm_13);
	ksched_set_irq(IRQ(0xE), (uintptr_t)&i386_irqhandleasm_14);
	ksched_set_irq(IRQ(0xF), (uintptr_t)&i386_irqhandleasm_15);
	
	/* Exceptions */
	ksched_set_exc(0x0, (uintptr_t)&i386_exhandleasm_0);
	ksched_set_exc(0x1, (uintptr_t)&i386_exhandleasm_1);
	ksched_set_exc(0x2, (uintptr_t)&i386_exhandleasm_2);
	ksched_set_exc_usr(0x3, (uintptr_t)&i386_exhandleasm_3);
	ksched_set_exc_usr(0x4, (uintptr_t)&i386_exhandleasm_4);
	ksched_set_exc_usr(0x5, (uintptr_t)&i386_exhandleasm_5);
	ksched_set_exc(0x6, (uintptr_t)&i386_exhandleasm_6);
	ksched_set_exc(0x7, (uintptr_t)&i386_exhandleasm_7);
	ksched_set_exc(0x8, (uintptr_t)&i386_exhandleasm_8);
	ksched_set_exc(0x9, (uintptr_t)&i386_exhandleasm_9);
	ksched_set_exc(0xA, (uintptr_t)&i386_exhandleasm_10);
	ksched_set_exc(0xB, (uintptr_t)&i386_exhandleasm_11);
	ksched_set_exc(0xC, (uintptr_t)&i386_exhandleasm_12);
	ksched_set_exc(0xD, (uintptr_t)&i386_exhandleasm_13);
	ksched_set_exc(0xE, (uintptr_t)&i386_exhandleasm_14);
	ksched_set_exc(0xF, (uintptr_t)&i386_exhandleasm_15);
	ksched_set_exc(0x10, (uintptr_t)&i386_exhandleasm_16);
	ksched_set_exc(0x11, (uintptr_t)&i386_exhandleasm_17);
	ksched_set_exc(0x12, (uintptr_t)&i386_exhandleasm_18);
	ksched_set_exc(0x13, (uintptr_t)&i386_exhandleasm_19);
	ksched_set_exc(0x14, (uintptr_t)&i386_exhandleasm_20);
	ksched_set_exc(0x15, (uintptr_t)&i386_exhandleasm_21);
	ksched_set_exc(0x16, (uintptr_t)&i386_exhandleasm_22);
	ksched_set_exc(0x17, (uintptr_t)&i386_exhandleasm_23);
	ksched_set_exc(0x18, (uintptr_t)&i386_exhandleasm_24);
	ksched_set_exc(0x19, (uintptr_t)&i386_exhandleasm_25);
	ksched_set_exc(0x1A, (uintptr_t)&i386_exhandleasm_26);
	ksched_set_exc(0x1B, (uintptr_t)&i386_exhandleasm_27);
	ksched_set_exc(0x1C, (uintptr_t)&i386_exhandleasm_28);
	ksched_set_exc(0x1D, (uintptr_t)&i386_exhandleasm_29);
	ksched_set_exc(0x1E, (uintptr_t)&i386_exhandleasm_30);
	ksched_set_exc(0x1F, (uintptr_t)&i386_exhandleasm_31);

	#ifdef DEBUG_MODE
		kprintf("( DONE )\n");
		kprintf("Initializing the PIC...");
	#endif
	
	/*
	 * Init PIC
	 *
	 */
	outb(0x20, 0x11);
	outb(0x21, IRQ(0));	/* Set 0xA0 as IRQ ints of the master PIC */
	outb(0x21, 0x04);
	outb(0x21, 0x01);
	outb(0x21, 0x00);

	outb(0xA0, 0x11);
	outb(0xA1, IRQ(8));	/* Set 0xA8 as IRQ ints of the slave PIC */
	outb(0xA1, 0x02);
	outb(0xA1, 0x01);
	outb(0xA1, 0x00);

	/* Read the current IRQ mask of the PIC */
	ksched_irqmask = inb(0x21);	
   	ksched_irqmask = ksched_irqmask | (inb(0xa1)<<8);
	
	/*
	 * Disable every IRQ and init the kernel IRQ table
	 *
	 */
	l__irqn = 16;

	while (l__irqn--)
	{
		ksched_disable_irq(l__irqn);
		ksched_irqt_s[l__irqn].pid = 0;
		ksched_irqt_s[l__irqn].tid = 0;
		ksched_irqt_s[l__irqn].used = 0;
	}

	/* Ensure the IRQs are masked by the CPU */
	__asm__ __volatile__("cli");
	
	/* Initialize the PIT */
	ksched_init_timer();
	
	/* Enable the RTC IRQ */
	ksched_enable_irq(0);

	#ifdef DEBUG_MODE
		kprintf("( DONE )\n");
	#endif	
		
	return 0;
}

/*
 * i386_handle_emptyint
 *
 * Handles an unused software interrupt 'intno'
 *
 */
void i386_handle_emptyint(unsigned intno)
{
	/*
	 * Catch softints if wanted
	 *
	 */
	if (current_t[THRTAB_SOFTINT_LISTENER_SID] != 0)
	{
		kremote_received(intno);
		/* ksched_next_thread done by sysc_freeze_subject ! */
	}
	 else
	{
		#ifdef STRONG_DEBUG_MODE
		kprintf("Empty Soft-IRQ 0x%X (P/T 0x%X : 0x%X)\n", 
			intno,
			current_p[PRCTAB_SID],
			current_t[THRTAB_SID]
	       		);	
		#endif	
	}
		
	return;
}

/*
 * ksched_handle_irq(irqn)
 *
 * Handles an IRQ with IRQ-number (not software INT!) 'irqn'.
 *
 */
void ksched_handle_irq(irq_t irqn)
{
	/* Internal handler for IRQ0 */
	if (irqn == 0)
	{
		(*kinfo_rtc_ctr) ++;
		
		/* Overflow of the system clock? */
		if (*kinfo_rtc_ctr == 0)
		{
			kprintf("System PANIC! Clock overflow in ksched_handle_irq (intr.c)\n");
			kprintf("Memory malfunction or kernel bug persumed.\n");
			while(1);
		}
		
		/* Detecting & Handling time outs */
		if (timeout_next != 0)
		{
			if (timeout_next <= *kinfo_rtc_ctr)
			{
				/* Start the pending thread */
				ksched_start_thread(timeout_queue);
				timeout_queue[THRTAB_THRSTAT_FLAGS] &= (~THRSTAT_TIMEOUT);
				ksched_del_timeout(timeout_queue);
			}
		}
		
		/* Reducing thread priority */
		if (*kinfo_eff_prior == 0)
		{
			ksched_change_thread = true;
		}		
			else
		{		
			(*kinfo_eff_prior) --;
		}
	}

	/* The other IRQs */
	ksched_start_irq_handler(irqn);

	/* Start next thread, if needed */	
	KSCHED_TRY_RESCHED();

	return;
}

/*
 * ksched_do_panic()
 *
 * Stops the system with a kernel panic message.
 * This function should be the only proper way
 * to stop the system.
 *
 */
void ksched_do_panic(void)
{
	/* Be creative and write a nicer panic message */
	kprintf("Kernel Panic. Please reboot now.");
	while(1);
}

/*
 * ksched_kernel_mode_except()
 *
 * Handles an kernel mode exception by halting the system
 *
 */
static void ksched_kernel_mode_except(void)
{
	kprintf("\n\n\n");
	kprintf("--------------------------------\n");
	kprintf("EXCEPTION OCCURED IN KERNEL MODE\n");
	kprintf("--------------------------------\n");
	kprintf("Excpt. Number: 0x%X\n", i386_saved_error_num);
	kprintf("Excpt. Code  : 0x%X\n", i386_saved_error_code);
	
	if (i386_saved_error_num == 0xE)
	{
		uintptr_t l__adr = 0;
		
		kprintf("- Page fault informations -\n");
		
		__asm__ __volatile__ ("movl %%cr2, %%eax\n": "=a" (l__adr));
		
		kprintf("Access to    :  0x%X\n", l__adr);
		
		uint32_t *l__pdir = (void*)(uintptr_t)
					current_p[PRCTAB_PAGEDIR_PHYSICAL_ADDR];
		uint32_t *l__tab = kmem_get_table(l__pdir, l__adr, false);
		
		kprintf("PDir/n=>Entry: 0x%X/0x%X => 0x%X\n", 
			&l__pdir[0], 
			(l__adr >> 22),
			l__pdir[(l__adr >> 22)]
		       );
				
		if (l__tab != NULL)
		{
			kprintf("Tab=>Entry   : 0x%X => 0x%X\n", 
				&l__tab[0], 
				l__tab[(l__adr / 4096) & 0x3FF]
			       );
		}
		else
		{
			kprintf("Tab=>Entry   : Can't resolve\n");
		}
		
		kprintf("- Page fault informations -\n");
	}	
	
	kprintf("At CS:EIP    : 0x%X:0x%X\n", i386_saved_error_cs,
					    i386_saved_error_eip
               );
	kprintf("Last User-ESP: 0x%X\n", i386_saved_error_esp);
	kprintf("Last Kern-ESP: 0x%X\n", i386_error_kernel_esp);
	
	if ((current_p != NULL) && (current_t != NULL))
	{
		kprintf("P-SID | T-SID: 0x%X | 0x%X\n", current_p[PRCTAB_SID],
						        current_t[THRTAB_SID]
	       	       );
	} 
	 else
	{
		kprintf("P-SID | T-SID: UNKOWN STATE\n");
	}
	
	kprintf("\n");
	kprintf("--------------------------------\n");
	kprintf("SYSTEM HOLDED - ABANDON ALL HOPE\n");
	kprintf("--------------------------------\n");

	while(1) ksched_do_panic();
}

/*
 * ksched_exception_to_user_mode()
 *
 * Tries to transfer an exception to user mode.
 * If there is no valid user mode exception handler,
 * the function will stop the system with an kernel
 * exception message.
 *
 */
static void ksched_exception_to_user_mode(void)
{
	if (kpaged_handle_exception(i386_saved_error_num, 
				    i386_saved_error_code,
				    i386_saved_error_eip
				   ) 
    	     == 1
   	   )
	{
		kprintf("\nCan't handle exception 0x%X (0x%X) in user mode at (0x%X:0x%X) for (P 0x%X T 0x%X).\n", i386_saved_error_num, i386_saved_error_code, i386_saved_error_cs, i386_saved_error_eip, current_p[PRCTAB_SID], current_t[THRTAB_SID]);
		
		#ifndef DEBUG_MODE
		kprintf("There is no paging daemon install. System holded. Abandon all hope.");
		#endif

		while(1);
		/* Can't be handled in user mode */
		ksched_kernel_mode_except();
		
	}

	return;
}

/*
 * ksched_handle_except
 *
 * Handles an exception.
 *
 */
void ksched_handle_except(void)
{
	/*
	 * Catching softints, if wanted
	 *
	 */
	if (current_t[THRTAB_SOFTINT_LISTENER_SID] != 0)
	{
		if (    (i386_saved_error_num == 13)
		     && (i386_saved_error_code & 2)
		   )
		{
			uint32_t l__int;
			
			/* 
		 	* Okay, this is a piece of ugly code. To find
		 	* surley out that the source of an exception
		 	* was an INT instruction, we have to
		 	* lock the exceptions and IRQs from user mode
		 	* access. However if a locked IDT-descriptor
		 	* was called by the INT instruction, an
		 	* exception will be generated, that will
		 	* return to the address of the INT-instruction,
		 	* so the INT-instruction could be repeated.
		 	* However we don't want to repeat it, because
		 	* the softint is already catched by the
		 	* recv_softints-call. So we need to set a
		 	* new return address after the INT instruction.
		 	* Because we are in a bad world, there a three
		 	* INT instruction in the IA-32: INT, INT3, INTO.
		 	* INT has a size of two bytes, INT3 and INTO a size
		 	* of one byte. So we have to find out which
		 	* instruction caused the software interrupt. 
		 	* 
		 	* However, if you find out a better way, it
		 	* would be nice, if you send me a mail.
		 	*
		 	* Friedrich Gr�ter.
		 	*
		 	*/
			uint32_t *l__stack =  (void*)(uintptr_t)
					(
			    	    	current_t[THRTAB_KERNEL_STACK_ADDRESS]
				  	  + KERNEL_STACK_SIZE
			          	- (17 * 4)
					);
			uint8_t l__instr = 0;
			uint8_t l__addval = 0;
			
			/* Get last opcode */
			__asm__ __volatile__(
					     "pushl %%es\n"
		    			     "movw $0x33, %%ax\n"
					     "movw %%ax, %%es\n"
					     "movb %%es:(%%ebx), %%al\n"
					     "popl %%es\n"
					     :"=a" (l__instr)
					     :"b"(l__stack[12])
					     :"memory"
					    );
			   
			/* Is it int, int3 or into? */
			if (l__instr == 0xCD)   
			{
				/* int, we have an parameter */
				l__addval = 2;
			}
		
			if ((l__instr == 0xCC) || (l__instr == 0xCE)) 
			{
				/* int3 or int0, no parameter */
				l__addval = 1;
			}
			
			/* Invalid instruction, kernel bug */
			if ((l__instr < 0xCD) || (l__instr > 0xCE))
			{
				kprintf("PANIC: Unkown softint instruction 0x%X in"
					"ksched_handle_except\n",
					l__instr
			       		);
				while(1) ksched_do_panic();
			}

			/* Lets override the int instruction */
			l__stack[12] += l__addval;		
			
			l__int = ((i386_saved_error_code) - 2) / 8;
			
		
			if (kremote_received(l__int) == 0)
				return;				
			else
				l__stack[12] -= l__addval;
			
		}
		
	}
	
	/*
	 * The regular way of exception handling
	 *
	 */
	/* Handle invalid exceptions as empty system calls */
	if (    (i386_saved_error_num > EXC_LAST_VALID_X86_EXCEPTION)
	     || (i386_saved_error_num == EXC_X86_UNKNOWN)
	   )
	{
		#ifdef DEBUG_MODE
			kprintf("Invalid exception. Transfered to emptyint.\n");
		#endif
		i386_handle_emptyint(i386_saved_error_num);
		return;
	}
		
	/* Test if exception occured in kernel or user mode */
	if (i386_saved_error_cs == 0x8)
	{
		/* Kernel mode */
		ksched_kernel_mode_except();	
	}
	 else
	{
		/* 
		 * Test if exception was produced by the COW-flag
		 *
		 */
		if (    (i386_saved_error_num == EXC_X86_PAGE_FAULT)
		     && (i386_saved_error_code & 0x7)
		   )
		{
			/* Yes. Transfer exception to COW-handler */
			if (kmem_copy_on_write())
			{
				/* COW-handler rejects, move to user mode */
				ksched_exception_to_user_mode();
			}
			 else
			{
				return;
			}
		}
		  else
		{
			/* No. Try to move exception to user mode */
			ksched_exception_to_user_mode();
		}
	}
	
	/* Stop the system execution in DEBUG_MODE */
	#ifdef STRONG_DEBUG_MODE
		kprintf("\nUser mode exception occured.\n");
		kprintf("DEBUG_MODE stopped execution.\n");
		while(1) ksched_do_panic();
	#endif	
}
