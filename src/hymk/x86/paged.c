/*
 *
 * paged.c
 *
 * (C)2004 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying'). 
 *
 * Paging daemon related functions
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

sid_t paged_thr_sid = 0;

/*
 * sysc_set_paged()
 *
 * (Implementation of the "set_paged" system call)
 *
 * Installs the current thread as PageD.
 *
 */
void sysc_set_paged(void)
{
	if (current_p[PRCTAB_IS_ROOT] != 1)
	{
		SET_ERROR(ERR_NOT_ROOT);
		return;
	}
	
	if (paged_thr_sid)
	{
		SET_ERROR(ERR_PAGING_DAEMON);
		return;
	}
		   
	/* Set paged */
	paged_thr_sid = current_t[THRTAB_SID];
	current_p[PRCTAB_IS_PAGED] = 1;
	
	#ifdef DEBUG_MODE
	kprintf("SET P 0x%X : T 0x%X as hyPageD\n",
		current_p[PRCTAB_SID],
		current_t[THRTAB_SID]
	       );
	#endif
	
	return;
}

/*
 * kpaged_message_send()
 *
 * Synchronizes the current thread to the PageD and
 * and freezes it.
 *
 * Return:
 *	2	No PageD installed
 *	1	Can't sync to installed PageD (Error-Code)
 *	0	Success
 *
 */
static int kpaged_message_send(void)
{
	if (paged_thr_sid == 0) return 2;
	
	/* Synchronize with PageD */
	if (!sysc_sync(paged_thr_sid, 0, 0))
	{
		return 1;
	}
	
	/* And sleep */
	sysc_freeze_subject(current_t[THRTAB_SID]);
	
	return 0;
}


/*
 * kpaged_exception_to_usermode(number, code, ip)
 *
 * Transfers the exception handling to the paging daemon
 * or a thread that is listening to the current threads'
 * softints.
 *
 * Return value:
 *
 *	1	Exception stays in kernel mode
 *	0	Exception transfered to user mode
 *
 */
static int kpaged_exception_to_usermode(uint32_t number, uint32_t code, uintptr_t ip)
{
	/* Translate exception number */
	switch(number)
	{
		case (EXC_X86_DIVISION_BY_ZERO):
		{
			current_t[THRTAB_LAST_EXCPT_NUMBER] =
				EXC_DIVISION_BY_ZERO;
			break;
		}
	
		case (EXC_X86_INVALID_TSS):
		case (EXC_X86_SEGMENT_NOT_PRESENT):
		case (EXC_X86_GENERAL_PROTECTION_FAULT):
		{
			current_t[THRTAB_LAST_EXCPT_NUMBER] =
				EXC_PROTECTION_FAULT;
			break;
		}
		
		case (EXC_X86_DEVICE_NOT_AVAILABLE):
		case (EXC_X86_FLOATING_POINT_ERROR):
		case (EXC_X86_SIMD_FLOATING_POINT_ERROR):
		case (EXC_X86_COPROCESSOR_SEGMENT_OVERRUN):
		{
			current_t[THRTAB_LAST_EXCPT_NUMBER] =
				EXC_FLOATING_POINT;
			break;
		}
		
		case (EXC_X86_INVALID_OPCODE):
		{
			current_t[THRTAB_LAST_EXCPT_NUMBER] =
				EXC_INVALID_INSTRUCTION;
			break;
		}
		
		case (EXC_X86_STACK_FAULT):
		{
			current_t[THRTAB_LAST_EXCPT_NUMBER] =
				EXC_STACK_FAULT;
			break;
		}
		
		case (EXC_X86_DEBUG_EXCEPTION):
		case (EXC_X86_BREAKPOINT):
		case (EXC_X86_BOUND_RANGE_EXCEEDED):
		{
			current_t[THRTAB_LAST_EXCPT_NUMBER] =
				EXC_TRAP;
			break;
		}
		
		case (EXC_X86_PAGE_FAULT):
		{
			current_t[THRTAB_LAST_EXCPT_NUMBER] =
				EXC_INVALID_PAGE;
			break;
		}
		
		case (EXC_X86_OVERFLOW_EXCEPTION):
		{
			current_t[THRTAB_LAST_EXCPT_NUMBER] =
				EXC_OVERFLOW;
			break;
		}		
				
		case (EXC_X86_NOT_MASKABLE_INTERRUPT):
		case (EXC_X86_DOUBLE_FAULT):
		case (EXC_X86_UNKNOWN):
		case (EXC_X86_ALIGNMENT_CHECK):
		case (EXC_X86_MACHINE_CHECK):
		{
			current_t[THRTAB_LAST_EXCPT_NUMBER] =
				EXC_SPECIFIC;
			break;
		}		
	}	

	/* Write exception datas */
	current_t[THRTAB_LAST_EXCPT_NR_PLATTFORM] = number;
	current_t[THRTAB_LAST_EXCPT_ERROR_CODE] = code;
	current_t[THRTAB_LAST_EXCPT_ADDRESS] = ip;
	
	/* Redirect exception? */
	if (current_t[THRTAB_SOFTINT_LISTENER_SID] != 0)
	{
		kremote_received(number);
		/*if (kremote_received(number) == 0)
			return 1;*/
	}
	 else /* No redirection. Pass the exception to the PageD */
	{
		/* No PageD installed, so the exception stays in kernel mode */
		if (paged_thr_sid == 0) return 1;	
				
		if (kpaged_message_send())
		{
			return 1;
		}
	}
	
	return 0;
}

/*
 * kpaged_send_pagefault(number, code, ip, adr)
 *
 * Transfer the handling of a page fault to the paging daemon.
 * The function won't determine the page faults address. It
 * will receive it by the parameter "adr" from its caller
 * (in normal case this would be kpaged_pagefault_to_usermode
 *  or sysc_unmap).
 *
 * Return value:
 *
 *	1	Exception stays in kernel mode
 *	0	Exception transfered to user mode
 */
int kpaged_send_pagefault(uint32_t number, uint32_t code, uint32_t ip, uint32_t adr)
{
	/* Load the descriptor of the page */
	long l__offs = (adr / 4096) & 0x3ff;
	uint32_t *l__tab;	
	uint32_t l__descr;
	
	l__tab = kmem_get_table(i386_current_pdir, adr, false);
	if (l__tab == NULL) 
	{
		l__descr = l__tab[adr / (4096 * 1024)];
	}
	 else
	{
		l__descr = l__tab[l__offs];
	}
	
	/* Write exception informations */
	current_t[THRTAB_LAST_EXCPT_NUMBER] = EXC_INVALID_PAGE;	
	current_t[THRTAB_LAST_EXCPT_NR_PLATTFORM] = number;
	current_t[THRTAB_LAST_EXCPT_ERROR_CODE] = code;
	current_t[THRTAB_LAST_EXCPT_ADDRESS] = ip;
	current_t[THRTAB_PAGEFAULT_LINEAR_ADDRESS] = adr;
	current_t[THRTAB_PAGEFAULT_DESCRIPTOR] = l__descr;

	/* Redirect exception? */
	if (current_t[THRTAB_SOFTINT_LISTENER_SID] != 0)
	{
		kremote_received(number);
	
		/*if (kremote_received(number) == 1)
			return 1;*/
	}
	 else /* No redirection. Pass the exception to the PageD */
	{
		/* No PageD installed, so the exception stays in kernel mode */
		if (paged_thr_sid == 0) return 1;	
				
		if (kpaged_message_send())
		{
			return 1;
		}
	}
	
	return 0;
}

/*
 * kpaged_pagefault_to_usermode(number, code, ip)
 *
 * Transfers the handling of a page fault to the paging daemon
 * or a thread that is listening to the current thread's 
 * softints.
 *
 * This function will determine the address of the page fault
 * by loading the content of the CR2 register. 
 *
 * Return value:
 *
 *	1	Exception stays in kernel mode
 *	0	Exception transfered to user mode
 *
 */
static int kpaged_pagefault_to_usermode(uint32_t number, uint32_t code, uint32_t ip)
{
	uint32_t l__adr;
	
	/* Load the address of the page fault */
	asm ("movl %%cr2, %%eax": "=a" (l__adr));
	
	/* Send the exception */
	return kpaged_send_pagefault(number, code, ip, l__adr);
}
 
/*
 * kpaged_handle_exception(number, code, ip)
 *
 * Handles an user mode exception and translates it
 * - if occurred - to an page fault. If no other
 * thread receives exceptions from the current thread
 * the exception will be transfered to the PageD.
 *
 * Return value:
 *
 *	1	Exception stays in kernel mode
 *	0	Exception handled by user mode
 *
 */
int kpaged_handle_exception(uint32_t number, uint32_t code, uintptr_t ip)
{
	/* No thread currently active (seems to be a kernel bug) */
	if ((current_t == NULL) || (current_p == NULL)) return 1;
	
	/* Exception of the paging daemon */
	if (current_t[THRTAB_SID] == paged_thr_sid) return 1;
	
	/* Make a difference between exception and page fault */
	if (number != EXC_X86_PAGE_FAULT)
	{
		/* Send exception to user mode */
		if (kpaged_exception_to_usermode(number, code, ip))
			return 1;
	}
	 else
	{
		/* Send pagefault to user mode */
		if (kpaged_pagefault_to_usermode(number, code, ip))
			return 1;
	}
		
	/* Additional debug message */
	#ifdef STRONG_DEBUG_MODE
	kprintf("\n\n\n");
	kprintf("--------------------------------\n");
	kprintf(" EXCEPTION OCCURED IN USER MODE \n");
	kprintf("--------------------------------\n");
	kprintf("Excpt. Number: 0x%X\n", i386_saved_error_num);
	kprintf("Excpt. Code  : 0x%X\n", i386_saved_error_code);
	kprintf("At CS:EIP    : 0x%X:0x%X\n", i386_saved_error_cs,
					    i386_saved_error_eip
               );
	kprintf("Last ESP     : 0x%X\n", i386_saved_error_esp);
	
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
	kprintf("  EXCEPTION PASSED TO HYPAGED.  \n");
	kprintf("--------------------------------\n");	
	#endif

	return 0;
}

/*
 * sysc_test_page(adr, sid)
 *
 * Returns the access flags of a page "adr" in the address 
 * space of the proces "sid". If "sid" is invalid or null,
 * the function will access to the current process address
 * space. If the executing process has not root priviledges
 * it may not get information about other address spaces.
 *
 * Return value:
 *	Access flags: PGA_READ, PGA_WRITE, PGA_EXECUTE
 *
 */
unsigned sysc_test_page(uintptr_t adr, sid_t sid)
{
	unsigned l__retval = 0;
	uint32_t *l__pdir_d = NULL;
	uint32_t *l__ptab_d = NULL;
	unsigned l__offs = (adr / 4096)	& 0x3ff;
	
	/* Test SID */
	if ((sid == SID_PLACEHOLDER_NULL) || (sid == SID_PLACEHOLDER_INVALID))
		sid = current_p[PRCTAB_SID];
	
	/* Do we have the rights to do that? */
	if ((sid != current_p[PRCTAB_SID]) && (current_p[PRCTAB_IS_ROOT] == false))
	{
		SET_ERROR(ERR_NOT_ROOT);
		return 0;
	}
	
	/* Is it a valid address? */
	if (adr > VAS_KERNEL_START)
	{
		SET_ERROR(ERR_INVALID_ADDRESS);
		return 0;
	}
	
	/* Is it a thread subject */
	if (sid & SIDTYPE_THREAD)
	{
		if (!kinfo_isthrd(sid))
		{
			SET_ERROR(ERR_INVALID_SID);
			return 0;
		}
		
		/* Get its process SID */
		sid = THREAD(sid, THRTAB_PROCESS_SID);
	}
	
	/* Is it a valid address space? */
	if (!kinfo_isproc(sid))
	{
		SET_ERROR(ERR_INVALID_SID);
		return 0;
	}
	
	/* Try to get the page */
	l__pdir_d = (void*)(uintptr_t)
			PROCESS(sid,
				PRCTAB_PAGEDIR_PHYSICAL_ADDR
			       );	
	l__ptab_d = kmem_get_table(l__pdir_d, adr, 0);
	
	if (l__ptab_d[l__offs] & GENFLAG_PRESENT)
	{
		if ((l__ptab_d[l__offs] & GENFLAG_READABLE) == GENFLAG_READABLE)	l__retval |= PGA_READ;
		if ((l__ptab_d[l__offs] & GENFLAG_WRITABLE) == GENFLAG_WRITABLE)	l__retval |= PGA_WRITE;
		if ((l__ptab_d[l__offs] & GENFLAG_EXECUTABLE) == GENFLAG_EXECUTABLE)	l__retval |= PGA_EXECUTE;
	}
	
	return l__retval;
}
