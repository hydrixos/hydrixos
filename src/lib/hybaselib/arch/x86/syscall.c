/*
 *
 * syscall.c
 *
 * (C)2005 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library').   
 *
 * Implementation of the hymk system calls
 *
 */
#include <hydrixos/types.h>
#include <hydrixos/hymk.h>
#include <hydrixos/tls.h>

void hymk_alloc_pages(void* adr, unsigned pages)
{
	__asm__ __volatile__("int $0xC0\n"
		     	     "addl $0, %%esp\n"
		     	     : "=a" (*tls_errno)
		     	     : "a" ((uintptr_t)adr),
		     	       "b" (pages)
		     	     : "memory"
		     	    );
	
}

sid_t hymk_create_thread(void* ip, void* sp)
{
	sid_t l__retval = 0;

	__asm__ __volatile__("int $0xC1\n"
		     	     : "=a" (*tls_errno),
		     	       "=b" (l__retval)
		     	     : "a" ((uintptr_t)ip),
		     	       "b" ((uintptr_t)sp)
		     	     : "memory"
		     	    );
	   
	return l__retval;
}

sid_t hymk_create_process(void* ip, void* sp)
{
	sid_t l__retval = 0;

	__asm__ __volatile__("int $0xC2\n"
		     	     : "=a" (*tls_errno),
		     	       "=b" (l__retval)
		     	     : "a" ((uintptr_t)ip),
		     	       "b" ((uintptr_t)sp)
		     	     : "memory"
		     	    );
	   
	return l__retval;
}

void hymk_set_controller(sid_t ctl)
{
	__asm__ __volatile__("int $0xC3\n"
		     	     : "=a" (*tls_errno)
		     	     : "a" (ctl)
		     	     : "memory"
	   );
}

void hymk_destroy_subject(sid_t subj)
{
	__asm__ __volatile__("int $0xC4\n"
		     	     : "=a" (*tls_errno)
		     	     : "a" (subj)
		     	     : "memory"
	   );
}

void hymk_chg_root(sid_t subj, unsigned mode)
{
	__asm__ __volatile__("int $0xC5\n"
		     	     : "=a" (*tls_errno)
		     	     : "a" (subj),
		     	       "b" (mode)
		     	     : "memory"
	   );
}

void hymk_freeze_subject(sid_t subj)
{
	__asm__ __volatile__("int $0xC6\n"
	                     : "=a" (*tls_errno)
	                     : "a" (subj)
	                     : "memory"
	                    );
}

void hymk_awake_subject(sid_t subj)
{
	__asm__ __volatile__("int $0xC7\n"
	                     : "=a" (*tls_errno)
	                     : "a" (subj)
	                     : "memory"
	                    );
}

void hymk_yield_thread(sid_t recv)
{
	__asm__ __volatile__("int $0xC8\n"
	                     : "=a" (*tls_errno)
	                     : "a" (recv)
	                     : "memory"
	                    );
}

void hymk_set_priority(sid_t subj, unsigned prior, unsigned cls)
{
	__asm__ __volatile__("int $0xC9\n"
	                     : "=a" (*tls_errno)
	                     : "a" (subj),
	                       "b" (prior),
	                       "c" (cls)
	                     : "memory"
	                    );
}

void hymk_allow(sid_t subj, sid_t me, void* adr, unsigned pages, unsigned op)
{
	__asm__ __volatile__("int $0xCA\n"
                 	    : "=a" (*tls_errno)
	                     : "a" (subj),
	                       "b" (me),
	                       "c" ((uintptr_t)adr),
	                       "d" (pages),
	                       "D" (op)
	                     : "memory"
	                    );
}

void hymk_map(sid_t subj, void* adr, unsigned pages, unsigned flags, uintptr_t dest_offset)
{
	__asm__ __volatile__("int $0xCB\n"
	                     : "=a" (*tls_errno)
	                     : "a" (subj),
	                       "b" ((uintptr_t)adr),
	                       "c" (pages),
	                       "d" (flags),
	                       "D" (dest_offset)
	                     : "memory"
	                    );
}

void hymk_unmap(sid_t subj, void* adr, unsigned pages, unsigned flags)
{
	__asm__ __volatile__("int $0xCC\n"
	                     : "=a" (*tls_errno)
	                     : "a" (subj),
	                       "b" ((uintptr_t)adr),
	                       "c" (pages),
	                       "d" (flags)
	                     : "memory"
	                    );
}

void hymk_move(sid_t subj, void* adr, unsigned pages, unsigned flags, uintptr_t dest_offset)
{
	__asm__ __volatile__("int $0xCD\n"
	                     : "=a" (*tls_errno)
	                     : "a" (subj),
	                       "b" ((uintptr_t)adr),
	                       "c" (pages),
	                       "d" (flags),
	                       "D" (dest_offset)
	                     : "memory"
	                    );
}

sid_t hymk_sync(sid_t subj, unsigned tm, unsigned resync)
{
	sid_t l__retval = 0;
	
	__asm__ __volatile__("int $0xCE\n"
	                     : "=a" (*tls_errno),
	                       "=b" (l__retval)
	                     : "a" (subj),
	                       "b" (tm),
	                       "c" (resync)
	                     : "memory"
	                    );
	   
	return l__retval;
}

void hymk_io_allow(sid_t subj, unsigned flags)
{
	__asm__ __volatile__("int $0xCF\n"
	                     : "=a" (*tls_errno)
	                     : "a" (subj),
	                       "b" (flags)
	                     : "memory"
	                    );
}

void hymk_io_alloc(uintptr_t phys, void* adr, unsigned pages, unsigned flags)
{
	__asm__ __volatile__("int $0xD0\n"
	                     : "=a" (*tls_errno)
	                     : "a" (phys),
	                       "b" ((uintptr_t)adr),
	                       "c" (pages),
	                       "d" (flags)
	                     : "memory"
	                    );
}

void hymk_recv_irq(unsigned irqn)
{
	__asm__ __volatile__("int $0xD1\n"
	                     : "=a" (*tls_errno)
	                     : "a" (irqn)
	                     : "memory"
	                    );
}

unsigned hymk_recv_softints(sid_t subj, unsigned tm, unsigned flags)
{
	int l__retval;
	
	__asm__ __volatile__("int $0xD2\n"
	                     : "=a" (*tls_errno),
	                       "=b" (l__retval)
	                     : "a" (subj),
	                       "b" (tm),
	                       "c" (flags)
	                     : "memory"
	                    );
	   
	return l__retval;
}

reg_t hymk_read_regs(sid_t subj, unsigned tp)
{
	reg_t l__retval;
	
	__asm__ __volatile__("int $0xD3\n"
	                     : "=a" (*tls_errno),
	                       "=b" (l__retval.regs[0]),
	                       "=c" (l__retval.regs[1]),
	                       "=d" (l__retval.regs[2]),
	                       "=S" (l__retval.regs[3])
	                     : "a" (subj),
	                       "b" (tp)
	                     : "memory"
	                    );

	return l__retval;
}

void hymk_write_regs(sid_t subj, unsigned tp, reg_t rv)
{
	__asm__ __volatile__("int $0xD4\n"
	                     : "=a" (*tls_errno)
	                     : "a" (subj),
	                       "b" (tp),
	                       "c" (rv.regs[0]),
	                       "d" (rv.regs[1]),
	                       "S" (rv.regs[2]),
	                       "D" (rv.regs[3])
	                     : "memory"
	                    );
}

void hymk_set_paged(void)
{
	__asm__ __volatile__("int $0xD5\n"
	                     : "=a" (*tls_errno)
	                     : 
	                     : "memory"
	                    );
}

unsigned hymk_test_page(uintptr_t adr, sid_t sid)
{
	unsigned l__retval = 0;
	
	__asm__ __volatile__("int $0xD6\n"
	                     : "=a" (*tls_errno),
	                       "=b" (l__retval)
	                     : "a" (adr),
	                       "b" (sid)
	                     : "memory"
	                    );
	   
	return l__retval;
}

