/*
 *
 * remote.c
 *
 * (C)2004 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g.
 * in the file 'copying').
 *
 * Remote controll of other threads
 *
 */
#include <hydrixos/types.h>
#include <stdio.h>
#include <mem.h>
#include <info.h>
#include <error.h>
#include <setup.h>
#include <sched.h>
#include <current.h>
#include <sysc.h>

/*
 * sysc_read_regs(sid, regtype)
 *
 * (Implementation of the "read_regs" system call)
 *
 * Reads the register contents of another thread. The calling
 * thread has to be root. The return value will be written
 * to the registers EBX (1), ECX (2), EDX (3), ESI (4) of the
 * calling thread.
 *
 * Parameters:
 *	sid		The thread
 *	regtype		Registers to read
 *			REGS_X86_GENERIC
 *				EAX => 1
 *				EBX => 2
 *				ECX => 3
 *				EDX => 4
 * 			REGS_X86_INDEX
 *				ESI => 1
 *				EDI => 2
 *				EBP => 3
 * 			REGS_X86_POINTERS
 * 				ESP => 1
 *				EIP => 2
 *			REGS_X86_EFLAGS
 *				EFLAGS => 1
 *
 * Return value:
 *	Passed directly to the registers of the calling
 *	thread. Registers EBX contains value 1, ECX value 2
 *	EDX value 3 and ESI value 4.
 *
 */
void sysc_read_regs(sid_t sid, unsigned regtype)
{
	uint32_t l__outreg[4] = {0, 0, 0, 0};
	uint32_t *l__srcstack = NULL;
	uint32_t *l__deststack = NULL;
	
	/* Are we allowed to do that? */
	if (current_p[PRCTAB_IS_ROOT] == 0)
	{
		SET_ERROR(ERR_NOT_ROOT);
		return;
	}
	
	/* Select dest stack */
	l__deststack = (void*)(uintptr_t)
			(
			    current_t[THRTAB_KERNEL_STACK_ADDRESS]
			  + KERNEL_STACK_SIZE
			  - (17 * 4)
			);
			
	/* Find source stack */
	if (    (!kinfo_isthrd(sid))
	     || (sid == 0x1000000)
	   )
	{
		SET_ERROR(ERR_INVALID_SID);
		return;
	}
	
	l__srcstack = (void*)(uintptr_t)
			(
			    (THREAD(sid, THRTAB_KERNEL_STACK_ADDRESS))
			  + KERNEL_STACK_SIZE
			  - (17 * 4)
			);
	
	/* Read registers from source */
	switch(regtype)
	{
		case (REGS_X86_GENERIC):
		{
			l__outreg[0] = l__srcstack[7]; /* eax */
			l__outreg[1] = l__srcstack[4]; /* ebx */
			l__outreg[2] = l__srcstack[6]; /* ecx */
			l__outreg[3] = l__srcstack[5]; /* edx */
			break;
		}
		
		case (REGS_X86_INDEX):
		{
			l__outreg[0] = l__srcstack[1]; /* esi */
			l__outreg[1] = l__srcstack[0]; /* edi */
			l__outreg[2] = l__srcstack[2]; /* ebp */
			break;
		}		
		
		case (REGS_X86_POINTERS):
		{
			l__outreg[0] = l__srcstack[15]; /* esp */
			l__outreg[1] = l__srcstack[12]; /* eip */
			break;
		}		
		
		case (REGS_X86_EFLAGS):
		{
			l__outreg[0] = l__srcstack[14]; /* eflags */

			break;
		}		
	}
	
	/* Write registers to destination */
	l__deststack[4] = l__outreg[0]; /* To ebx */
	l__deststack[6] = l__outreg[1]; /* To ecx */
	l__deststack[5] = l__outreg[2]; /* To edx */
	l__deststack[1] = l__outreg[3]; /* To esi */
	
	return;
}

/*
 * sysc_write_regs(sid, regtype, reg_a, reg_b, reg_c, reg_d)
 *
 * (Implementation of the "write_regs" system call)
 *
 * Modifies the register contents of another thread. The calling
 * thread has to be root.
 *
 * Parameters:
 *	sid		The thread
 *	regtype		Registers to read
 *			REGS_X86_GENERIC
 *				reg_a => EAX
 *				reg_b => EBX
 *				reg_c => ECX
 *				reg_d => EDX
 * 			REGS_X86_INDEX
 *				reg_a => ESI
 *				reg_b => EDI
 *				reg_c => EBP
 * 			REGS_X86_POINTERS
 * 				reg_a => ESP
 *				reg_b => EIP
 *			REGS_X86_EFLAGS
 *				reg_a => EFLAGS
 *
 *	reg_a - reg_d:	New register values (see above)
 */
void sysc_write_regs(sid_t sid, 
		     unsigned regtype, 
		     uint32_t reg_a,
		     uint32_t reg_b,
		     uint32_t reg_c,
		     uint32_t reg_d
		    )
{
	uint32_t *l__deststack = NULL;
	
	/* Are we allowed to do that? */
	if (current_p[PRCTAB_IS_ROOT] == 0)
	{
		SET_ERROR(ERR_NOT_ROOT);
		return;
	}
	
	/* Find destination stack */
	if (    (!kinfo_isthrd(sid))
	     || (sid == 0x1000000)
	   )
	{
		SET_ERROR(ERR_INVALID_SID);
		return;
	}
	
	l__deststack = (void*)(uintptr_t)
			(
			    (THREAD(sid, THRTAB_KERNEL_STACK_ADDRESS))
			  + KERNEL_STACK_SIZE
			  - (17 * 4)
			);
	
	/* Read registers from source */
	switch(regtype)
	{
		case (REGS_X86_GENERIC):
		{
			l__deststack[7] = reg_a; /* eax */
			l__deststack[4] = reg_b; /* ebx */
			l__deststack[6] = reg_c; /* ecx */
			l__deststack[5] = reg_d; /* edx */
			break;
		}
		
		case (REGS_X86_INDEX):
		{
			l__deststack[1] = reg_a; /* esi */
			l__deststack[0] = reg_b; /* edi */
			l__deststack[2] = reg_c; /* ebp */
			break;
		}		
		
		case (REGS_X86_POINTERS):
		{
			l__deststack[15] = reg_a; /* esp */
			l__deststack[12] = reg_b; /* eip */
			break;
		}		
		
		case (REGS_X86_EFLAGS):
		{
			/* eflags, only the lowest 8 bit will be changed */
			l__deststack[14] &= (~0xFF);
			l__deststack[14] |= reg_a;

			break;
		}		
	}
	
	return;
}		

/*
 * sysc_recv_softints(sid, timeout)
 *
 * (Implementation of the "recv_softints" system call)
 *
 * Waits for software interrupts (system calls; exceptions) 
 * of another thread and redirects them if occured. 
 * The traced thread will be stopped by freeze_thread. The 
 * caller has to be root. 
 *
 * The caller can define a timeout. If this timeout is 
 * reached, the waiting for incomming system calls will end. 
 * The timeout will be unlimited if set 0.
 *
 * Parameters:
 *	sid		The traced thread
 *	timeout		The used timeout. Unlimited, if 0xFFFFFFFF
 *					  No timeout, if 0
 *	flags		Flags
 *			RECV_AWAKE_OTHER	The observed thread
 *						should be awaked
 *						before the beginning
 *						of the observation 
 *						using sysc_awake_thread.
 *
 * Return Value:
 *	Number of the traced software interrupt.
 *	You can get the parameters of the software interrupt
 *	using read_regs and simulate a return value using
 *	write_regs. Exception codes etc. are stored in the
 *	thread descriptor.
 *
 */
uint32_t sysc_recv_softints(sid_t sid, unsigned timeout, int flags)
{
	/* Only root is allowed to... */
	if (current_p[PRCTAB_IS_ROOT] != 1)
	{
		SET_ERROR(ERR_NOT_ROOT);
		return 0xFFFFFFFF;
	}
	
	/* Invalid thread? */
	if (    (kinfo_isthrd(sid) == 0)
	     || (sid == 0x1000000)
	   )
	{
		SET_ERROR(ERR_INVALID_SID);
		return 0xFFFFFFFF;
	}
	
	/* Is always a listner active? */
	if (THREAD(sid, THRTAB_SOFTINT_LISTENER_SID) != 0)
	{
		SET_ERROR(ERR_RESOURCE_BUSY);
		return 0xFFFFFFFF;
	}
	
	/* Awake the other side, if wanted */
	if (    (flags & RECV_AWAKE_OTHER)
	     && (THREAD(sid, THRTAB_FREEZE_COUNTER) > 0)
	   )
	{
		/* Reduce its freeze counter */
		THREAD(sid, THRTAB_FREEZE_COUNTER) --;
		
		THREAD(sid, THRTAB_THRSTAT_FLAGS) &= (~THRSTAT_FREEZED);
		
		/* Can we put it into the run queue? */
		if (     (THREAD(sid, THRTAB_FREEZE_COUNTER) == 0)
		     &&  (!(THREAD(sid, THRTAB_THRSTAT_FLAGS) & THRSTAT_OTHER_FREEZE))
		   )
		{
			ksched_start_thread(&(THREAD(sid, 0)));
		}
	}
	
	/* No timeout, just exit */
	if (timeout == 0)
	{
		SET_ERROR(ERR_TIMED_OUT);
		return 0xFFFFFFFF;
	}
	
	/* Setup observated thread */
	THREAD(sid, THRTAB_SOFTINT_LISTENER_SID) = current_t[THRTAB_SID];
	THREAD(sid, THRTAB_RECEIVED_SOFTINT) = 0xFFFFFFFF;
	
	/* Set one of the different observation methods: "trace softints only" or "redirect softints" */
	if (flags & RECV_TRACE_SYSCALL)
		THREAD(sid, THRTAB_THRSTAT_FLAGS) |= THRSTAT_TRACE_ONLY;
	else
		THREAD(sid, THRTAB_THRSTAT_FLAGS) &= (~THRSTAT_TRACE_ONLY);
	
	/* Setup listening thread */
	current_t[THRTAB_RECV_LISTEN_TO] = sid;
	current_t[THRTAB_THRSTAT_FLAGS] |= THRSTAT_RECV_SOFTINT;
		
	/* Change to sleep mode */
	if (timeout != 0xFFFFFFFF)
	{
		ksched_add_timeout(current_t, timeout);	
		current_t[THRTAB_THRSTAT_FLAGS] |= THRSTAT_TIMEOUT;
	}
				
	ksched_stop_thread(current_t);
		
	ksched_change_thread = true;
	ksched_next_thread();
	i386_yield_kernel_thread();

	MSYNC();

	current_t[THRTAB_THRSTAT_FLAGS] &= (~THRSTAT_RECV_SOFTINT);
	current_t[THRTAB_RECV_LISTEN_TO] = 0;
	
	/* No software interrupt received */
	if (THREAD(sid, THRTAB_RECEIVED_SOFTINT) == 0xFFFFFFFF)
	{
		THREAD(sid, THRTAB_SOFTINT_LISTENER_SID) = 0;
		SET_ERROR(ERR_TIMED_OUT);
		return 0xFFFFFFFF;
	}		
	
	THREAD(sid, THRTAB_SOFTINT_LISTENER_SID) = 0;
		
	return THREAD(sid, THRTAB_RECEIVED_SOFTINT);
}

/*
 * kremote_received(intr)
 *
 * The current thread caused an software interrupt 'intr' that
 * should be handled by another thread, that is currently
 * receiving it.
 *
 * Return value:
 *
 *	0 - Handled
 *	1 - Not Handled (Trace-Only, Start Syscall)
 *
 */
int kremote_received(uint32_t intr)
{
	sid_t l__receiver = current_t[THRTAB_SOFTINT_LISTENER_SID];
	
	/* Is the receiver still active? */
	if (kinfo_isthrd(l__receiver) == 0)
	{
		kprintf("PANIC: Listener removed, without repairing the thread table.\n");
		while(1);
	}
	
	/* Inform the receiver */
	current_t[THRTAB_RECEIVED_SOFTINT] = intr;
	
	/* Restart the receiver */
	if (THREAD(l__receiver, THRTAB_THRSTAT_FLAGS) & THRSTAT_TIMEOUT)
	{
		ksched_del_timeout(&THREAD(l__receiver, 0));
		THREAD(l__receiver, THRTAB_THRSTAT_FLAGS) &= (~THRSTAT_TIMEOUT);
	}
		
	ksched_start_thread(&THREAD(l__receiver, 0));
	
	/* Comment: Trace-Only exists only for system calls */
	if ((current_t[THRTAB_THRSTAT_FLAGS] & THRSTAT_TRACE_ONLY) && (intr >= KFIRST_SYSCALL) && (intr <= KLAST_SYSCALL))
	{
		/* In trace-only mode, we have to yield us manually */
		sysc_freeze_subject(current_t[THRTAB_SID]);
		i386_yield_kernel_thread();
			
		return 1;	/* Continue with the execution of the system call */
	}
	 else 
	{
		/* In normal mode we can just freeze and jump out */
		sysc_freeze_subject(current_t[THRTAB_SID]);	
		
		return 0;	/* Don't execute the system call */
	}
}

