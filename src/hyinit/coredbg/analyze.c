/*
 *
 * analyze.c
 *
 * (C)2006 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying').   
 *
 * Execution analysis
 *
 */
#include <hydrixos/types.h>
#include <hydrixos/tls.h>
#include <hydrixos/errno.h>
#include <hydrixos/blthr.h>
#include <hydrixos/mem.h>
#include <hydrixos/stdfun.h>
#include <hydrixos/system.h>

#include <coredbg/cdebug.h>

#include "../hyinit.h"
#include "coredbg.h"

const utf8_t *dbg_str_invalid = "(invalid)";

/*
 * dbg_analyze_syscall(sysc, client)
 *
 * Analyzes the system call softint "sysc" of the client "client".
 * The output will be returned into a string.
 *
 * Return value:
 *	!= NULL   Information string
 *	== NULL	  Invalid interrupt or error
 *
 */ 
utf8_t* dbg_analyze_syscall(int sysc, sid_t client)
{
	dbg_registers_t l__regs = dbg_get_registers(client);
	if (*tls_errno) return NULL;
	
	utf8_t *l__buf = mem_alloc(1000);
	int l__len = 0;
	if (l__buf == NULL) return NULL;
	
	switch (sysc)
	{
		/* alloc_pages */
		case (0xC0):
		{
			l__len = snprintf(l__buf, 1000, "C0: alloc_pages(start = 0x%X, num = %u)", l__regs.eax, l__regs.ebx);
			break;
		}
		
		/* create_thread */
		case (0xC1):
		{
			l__len = snprintf(l__buf, 1000, "C1: create_thread(start = 0x%X, stack = 0x%X) => tSID-->EBX", l__regs.eax, l__regs.ebx);
			break;
		}
		
		/* create_process */
		case (0xC2):
		{
			l__len = snprintf(l__buf, 1000, "C2: create_process(start = 0x%X, stack = 0x%X) => tSID-->EBX", l__regs.eax, l__regs.ebx);
			break;
		}		
		
		/* set_controller */
		case (0xC3):
		{
			l__len = snprintf(l__buf, 1000, "C3: set_controller(sid = 0x%X)", l__regs.eax);
			break;
		}	
		
		/* destroy_subject */
		case (0xC4):
		{
			l__len = snprintf(l__buf, 1000, "C4: destroy_subject(sid = 0x%X)", l__regs.eax);
			break;
		}			
			
		/* chg_root */
		case (0xC5):
		{
			const utf8_t *l__flagstr;
			
			if (l__regs.ebx == 0)
			{
				l__flagstr = "CHGROOT_LEAVE";
			}
			 else if (l__regs.ebx == 1)
			{
				l__flagstr = "CHGROOT_ENTER";
			}
			 else
			{
				l__flagstr = dbg_str_invalid;
			}
			
			l__len = snprintf(l__buf, 1000, "C5: chg_root(sid = 0x%X, op = %u = %s)", l__regs.eax, l__regs.ebx, l__flagstr);
			break;
		}	
				
		/* freeze_subject */
		case (0xC6):
		{
			l__len = snprintf(l__buf, 1000, "C6: freeze_subject(sid = 0x%X)", l__regs.eax);
			break;
		}	
		
		/* awake_subject */
		case (0xC7):
		{
			l__len = snprintf(l__buf, 1000, "C7: awake_subject(sid = 0x%X)", l__regs.eax);
			break;
		}		
					
		/* yield_thread */
		case (0xC8):
		{
			l__len = snprintf(l__buf, 1000, "C8: yield_thread(sid = 0x%X)", l__regs.eax);
			break;
		}	
				
		/* set_priority */
		case (0xC9):
		{
			const utf8_t *l__flagstr;
			
			if (l__regs.ecx == 0)
			{
				l__flagstr = "SCHED_REGULAR";
			}
			 else
			{
				l__flagstr = dbg_str_invalid;
			}			
			
			l__len = snprintf(l__buf, 1000, "C9: set_priority(sid = 0x%X, pri = %u, cls = %u = %s)", l__regs.eax, l__regs.ebx, l__regs.ecx, l__flagstr);
			break;
		}
			
		/* allow */
		case (0xCA):
		{
			utf8_t l__flagstr[100];
			int l__pos = 0;
			
			/* Analyze flags */
			if (l__regs.edi == 0)
			{
				str_copy(l__flagstr, "(empty)", 10);
			}
			
			if (l__regs.edi & ALLOW_MAP)
			{
				str_copy(l__flagstr, "ALLOW_MAP", 10);
				l__pos += 9;
			}
			
			if (l__regs.edi & ALLOW_UNMAP)
			{
				if (l__pos != 0)
				{
					str_copy(&l__flagstr[l__pos], "|", 2);
					l__pos += 1;
				}
				str_copy(&l__flagstr[l__pos], "ALLOW_UNMAP", 12);
				l__pos += 11;
			}
			
			if ((l__regs.edi & ALLOW_REVERSE) == ALLOW_REVERSE)
			{
				if (l__pos != 0)
				{
					str_copy(&l__flagstr[l__pos], "|", 2);
					l__pos += 1;
				}
				str_copy(&l__flagstr[l__pos], "ALLOW_REVERSE", 14);
				l__pos += 13;
			}
			
			/* Remember: ALLOW_REVERSE = 4 | ALLOW_MAP */
			if ((l__regs.edi & ALLOW_REVERSE) == (ALLOW_REVERSE & (~ALLOW_MAP)))
			{
				if (l__pos != 0)
				{
					str_copy(&l__flagstr[l__pos], "|", 2);
					l__pos += 1;
				}
				str_copy(&l__flagstr[l__pos], "INVALID_REVERSE", 16);
				l__pos += 15;
			}			
			
			/* Any invalid flags? */
			if (l__regs.edi & (~(ALLOW_MAP | ALLOW_UNMAP | ALLOW_REVERSE)))
			{
				if (l__pos != 0)
				{
					str_copy(&l__flagstr[l__pos], "|", 2);
					l__pos += 1;
				}
				str_copy(&l__flagstr[l__pos], "(invalid flag)", 15);
				l__pos += 14;
			}
			
			/* Output function information */
			l__len = snprintf(l__buf, 1000, "CA: allow(dsid = 0x%X, ssid = 0x%X, adr = 0x%X, num = %u, op = 0x%X = %s)", l__regs.eax, l__regs.ebx, l__regs.ecx, l__regs.edx, l__regs.edi, l__flagstr);
			break;
		}
		
		/* map */
		case (0xCB):
		{
			utf8_t l__flagstr[200];
			int l__pos = 0;
			
			/* Analyze flags */
			if (l__regs.edx == 0)
			{
				str_copy(l__flagstr, "(empty)", 10);
			}
			
			if (l__regs.edx & MAP_READ)
			{
				str_copy(l__flagstr, "MAP_READ", 9);
				l__pos += 8;
			}
			
			if (l__regs.edx & MAP_WRITE)
			{
				if (l__pos != 0)
				{
					str_copy(&l__flagstr[l__pos], "|", 2);
					l__pos += 1;
				}
				str_copy(&l__flagstr[l__pos], "MAP_WRITE", 10);
				l__pos += 9;
			}
			
			if (l__regs.edx & MAP_EXECUTABLE)
			{
				if (l__pos != 0)
				{
					str_copy(&l__flagstr[l__pos], "|", 2);
					l__pos += 1;
				}
				str_copy(&l__flagstr[l__pos], "MAP_EXECUTABLE", 15);
				l__pos += 14;
			}			
						
			if (l__regs.edx & MAP_COPYONWRITE)
			{
				if (l__pos != 0)
				{
					str_copy(&l__flagstr[l__pos], "|", 2);
					l__pos += 1;
				}
				str_copy(&l__flagstr[l__pos], "MAP_COPYONWRITE", 16);
				l__pos += 15;
			}						
						
			if (l__regs.edx & MAP_PAGED)
			{
				if (l__pos != 0)
				{
					str_copy(&l__flagstr[l__pos], "|", 2);
					l__pos += 1;
				}
				str_copy(&l__flagstr[l__pos], "MAP_PAGED", 10);
				l__pos += 9;
			}						
			
			if (l__regs.edx & MAP_REVERSE)
			{
				if (l__pos != 0)
				{
					str_copy(&l__flagstr[l__pos], "|", 2);
					l__pos += 1;
				}
				str_copy(&l__flagstr[l__pos], "MAP_REVERSE", 12);
				l__pos += 11;
			}			
			
			/* Any undefined bits set? */
			if (l__regs.edx & (~(MAP_READ|MAP_WRITE|MAP_EXECUTABLE|MAP_COPYONWRITE|MAP_PAGED|MAP_REVERSE)))
			{
				if (l__pos != 0)
				{
					str_copy(&l__flagstr[l__pos], "|", 2);
					l__pos += 1;
				}
				str_copy(&l__flagstr[l__pos], "(invalid flag)", 15);
				l__pos += 14;
			}			
			
			/* Output informations about mem */
			l__len = snprintf(l__buf, 1000, "CB: map(sid = 0x%X, srcadr = 0x%X, num = %u, flags = 0x%X = %s, offs = 0x%X)", l__regs.eax, l__regs.ebx, l__regs.ecx, l__regs.edx, l__flagstr, l__regs.edi);
			break;
		}					
		
		/* unmap */
		case (0xCC):
		{
			const utf8_t *l__flagstr;
			
			/* Remember, we have no flags for unmap, just values */
			switch(l__regs.edx)
			{
				case(UNMAP_COMPLETE):
				{
					l__flagstr = "UNMAP_COMPLETE";
					break;
				}
				
			 	case (UNMAP_AVAILABLE):
				{
					l__flagstr = "UNMAP_AVAILABLE";
					break;
				}
				
 			 	case (UNMAP_WRITE):
				{
					l__flagstr = "UNMAP_WRITE";
					break;
				}			
 			 
 			 	case (UNMAP_EXECUTE):
				{
					l__flagstr = "UNMAP_EXECUTE";
					break;
				}			
			 
			 	default:
				{
					l__flagstr = dbg_str_invalid;
				}			
			}
			
			l__len = snprintf(l__buf, 1000, "CC: unmap(dsid = 0x%X, adr = 0x%X, num = %u, op = 0x%X = %s)", l__regs.eax, l__regs.ebx, l__regs.ecx, l__regs.edx, l__flagstr);
			break;
		}
			
		/* move */
		case (0xCD):
		{
			utf8_t l__flagstr[200];
			int l__pos = 0;
			
			/* Analyze the MAP flags */
			if (l__regs.edx == 0)
			{
				str_copy(l__flagstr, "(empty)", 10);
			}
			
			if (l__regs.edx & MAP_READ)
			{
				str_copy(l__flagstr, "MAP_READ", 9);
				l__pos += 8;
			}
			
			if (l__regs.edx & MAP_WRITE)
			{
				if (l__pos != 0)
				{
					str_copy(&l__flagstr[l__pos], "|", 2);
					l__pos += 1;
				}
				str_copy(&l__flagstr[l__pos], "MAP_WRITE", 10);
				l__pos += 9;
			}
			
			if (l__regs.edx & MAP_EXECUTABLE)
			{
				if (l__pos != 0)
				{
					str_copy(&l__flagstr[l__pos], "|", 2);
					l__pos += 1;
				}
				str_copy(&l__flagstr[l__pos], "MAP_EXECUTABLE", 15);
				l__pos += 14;
			}			
						
			if (l__regs.edx & MAP_COPYONWRITE)
			{
				if (l__pos != 0)
				{
					str_copy(&l__flagstr[l__pos], "|", 2);
					l__pos += 1;
				}
				str_copy(&l__flagstr[l__pos], "MAP_COPYONWRITE", 16);
				l__pos += 15;
			}						
						
			if (l__regs.edx & MAP_PAGED)
			{
				if (l__pos != 0)
				{
					str_copy(&l__flagstr[l__pos], "|", 2);
					l__pos += 1;
				}
				str_copy(&l__flagstr[l__pos], "MAP_PAGED", 10);
				l__pos += 9;
			}						
			
			if (l__regs.edx & MAP_REVERSE)
			{
				if (l__pos != 0)
				{
					str_copy(&l__flagstr[l__pos], "|", 2);
					l__pos += 1;
				}
				str_copy(&l__flagstr[l__pos], "MAP_REVERSE", 12);
				l__pos += 11;
			}			
			
			/* Any undefined flags? */
			if (l__regs.edx & (~(MAP_READ|MAP_WRITE|MAP_EXECUTABLE|MAP_COPYONWRITE|MAP_PAGED|MAP_REVERSE)))
			{
				if (l__pos != 0)
				{
					str_copy(&l__flagstr[l__pos], "|", 2);
					l__pos += 1;
				}
				str_copy(&l__flagstr[l__pos], "(invalid flag)", 15);
				l__pos += 14;
			}			
			
			l__len = snprintf(l__buf, 1000, "CD: move(dsid = 0x%X, adr = 0x%X, num = %u, op = 0x%X = %s)", l__regs.eax, l__regs.ebx, l__regs.ecx, l__regs.edx, l__flagstr);
			break;
		}			
		
		/* sync */
		case (0xCE):
		{
			l__len = snprintf(l__buf, 1000, "CE: sync(sid = 0x%X, time = 0x%X, re = %u) => SID -> EBX", l__regs.eax, l__regs.ebx, l__regs.ecx);
			break;
		}	
		
		/* io_allow */
		case (0xCF):
		{
			utf8_t l__flagstr[100];
			int l__pos = 0;
			
			/* Analyze flags */
			if (l__regs.ebx == 0)
			{
				str_copy(l__flagstr, "(empty)", 10);
			}
			
			if (l__regs.ebx & IO_ALLOW_IRQ)
			{
				str_copy(l__flagstr, "IO_ALLOW_IRQ", 13);
				l__pos += 12;
			}
			
			if (l__regs.ebx & IO_ALLOW_PORTS)
			{
				if (l__pos != 0)
				{
					str_copy(&l__flagstr[l__pos], "|", 2);
					l__pos += 1;
				}
				str_copy(&l__flagstr[l__pos], "IO_ALLOW_PORTS", 15);
				l__pos += 14;
			}			
			
			/* Any undefined flags? */
			if (l__regs.ebx & (~(IO_ALLOW_IRQ|IO_ALLOW_PORTS)))
			{
				if (l__pos != 0)
				{
					str_copy(&l__flagstr[l__pos], "|", 2);
					l__pos += 1;
				}
				str_copy(&l__flagstr[l__pos], "(invalid flag)", 15);
				l__pos += 14;
			}			
			
			l__len = snprintf(l__buf, 1000, "CF: io_allow(sid = 0x%X, flags = 0x%X = %s)", l__regs.eax, l__regs.ebx, l__flagstr);
			break;
		}	
		
		/* io_alloc */
		case (0xD0):
		{
			utf8_t l__flagstr[100];
			int l__pos = 0;
			
			/* Analyze flags */
			if (l__regs.edx == 0)
			{
				str_copy(l__flagstr, "(empty)", 10);
			}
			
			if (l__regs.edx & IOMAP_READ)
			{
				str_copy(l__flagstr, "IOMAP_READ", 11);
				l__pos += 10;
			}
			
			if (l__regs.edx & IOMAP_WRITE)
			{
				if (l__pos != 0)
				{
					str_copy(&l__flagstr[l__pos], "|", 2);
					l__pos += 1;
				}
				str_copy(&l__flagstr[l__pos], "IOMAP_WRITE", 12);
				l__pos += 11;
			}			
			
			if (l__regs.edx & IOMAP_EXECUTE)
			{
				if (l__pos != 0)
				{
					str_copy(&l__flagstr[l__pos], "|", 2);
					l__pos += 1;
				}
				str_copy(&l__flagstr[l__pos], "IOMAP_EXECUTE", 14);
				l__pos += 13;
			}	
					
			if (l__regs.edx & IOMAP_WITH_CACHE)
			{
				if (l__pos != 0)
				{
					str_copy(&l__flagstr[l__pos], "|", 2);
					l__pos += 1;
				}
				str_copy(&l__flagstr[l__pos], "IOMAP_WITH_CACHE", 17);
				l__pos += 16;
			}					
			
			/* Any undefined flags? */
			if (l__regs.edx & (~(IOMAP_READ|IOMAP_WRITE|IOMAP_EXECUTE|IOMAP_WITH_CACHE)))
			{
				if (l__pos != 0)
				{
					str_copy(&l__flagstr[l__pos], "|", 2);
					l__pos += 1;
				}
				str_copy(&l__flagstr[l__pos], "(invalid flag)", 15);
				l__pos += 14;
			}
						
			l__len = snprintf(l__buf, 1000, "D0: io_alloc(ioadr = 0x%X, dadr = 0x%X, num = %u, flags = 0x%X = %s)", l__regs.eax, l__regs.ebx, l__regs.ecx, l__regs.edx, l__flagstr);
			break;
		}	
		
		/* recv_irq */
		case (0xD1):
		{
			l__len = snprintf(l__buf, 1000, "D1: recv_irq(sid = 0x%X)", l__regs.eax);
			break;
		}
		
		/* recv_softints */
		case (0xD2):
		{
			utf8_t l__flagstr[100];
			int l__pos = 0;
			
			if (l__regs.ecx == 0)
			{
				str_copy(l__flagstr, "(empty)", 10);
			}
			
			if (l__regs.ecx & RECV_AWAKE_OTHER)
			{
				str_copy(l__flagstr, "RECV_AWAKE_OTHER", 17);
				l__pos += 16;
			}
			
			if (l__regs.ecx & RECV_TRACE_SYSCALL)
			{
				if (l__pos != 0)
				{
					str_copy(&l__flagstr[l__pos], "|", 2);
					l__pos += 1;
				}
				str_copy(&l__flagstr[l__pos], "RECV_TRACE_SYSCALL", 19);
				l__pos += 18;
			}			
			
			if (l__regs.ecx & (~(RECV_AWAKE_OTHER|RECV_TRACE_SYSCALL)))
			{
				if (l__pos != 0)
				{
					str_copy(&l__flagstr[l__pos], "|", 2);
					l__pos += 1;
				}
				str_copy(&l__flagstr[l__pos], "(invalid flag)", 15);
				l__pos += 14;
			}			
			
			l__len = snprintf(l__buf, 1000, "D2: recv_softints(sid = 0x%X, time = 0X%X, flags = 0x%X = %s) => intr->EBX", l__regs.eax, l__regs.ebx, l__regs.ecx, l__flagstr);
			break;
		}
		
		/* read_regs */
		case (0xD3):
		{
			const utf8_t *l__flagstr;
			const utf8_t *l__retvalstr;
						
			if (l__regs.ebx == REGS_X86_GENERIC)
			{
				l__flagstr = "REGS_X86_GENERIC";
				l__retvalstr = "<eax>->EBX, <ebx>->ECX, <ecx>->EDX, <edx>->ESI";
			}
			 else if (l__regs.ebx == REGS_X86_INDEX)
			{
				l__flagstr = "REGS_X86_INDEX";
				l__retvalstr = "<esi>->EBX, <edi>->ECX, <ebp>->EDX";
			}
 			 else if (l__regs.ebx == REGS_X86_POINTERS)
			{
				l__flagstr = "REGS_X86_POINTERS";
				l__retvalstr = "<esp>->EBX, <eip>->ECX";
			}			
 			 else if (l__regs.ebx == REGS_X86_EFLAGS)
			{
				l__flagstr = "REGS_X86_EFLAGS";
				l__retvalstr = "<eflags>->EBX";
			}			
			 else
			{
				l__flagstr = dbg_str_invalid;
				l__retvalstr = "(empty)";
			}				
			
			l__len = snprintf(l__buf, 1000, "D3: read_regs(sid=0x%X, regtp = 0x%X = %s) => %s", l__regs.eax, l__regs.ebx, l__flagstr, l__retvalstr);
			break;
		}	
		
		/* write_regs */
		case (0xD4):
		{
			const utf8_t *l__flagstr;
			const utf8_t *l__reg1;
			const utf8_t *l__reg2;
			const utf8_t *l__reg3;
			const utf8_t *l__reg4;
			
			if (l__regs.ebx == REGS_X86_GENERIC)
			{
				l__flagstr = "REGS_X86_GENERIC";
				l__reg1 = "<eax>";
				l__reg2 = "<ebx>";
				l__reg3 = "<ecx>";
				l__reg4 = "<edx>";
			}
			 else if (l__regs.ebx == REGS_X86_INDEX)
			{
				l__flagstr = "REGS_X86_INDEX";
				l__reg1 = "<esi>";
				l__reg2 = "<edi>";
				l__reg3 = "<ebp>";
				l__reg4 = "<xxx>";				
			}
 			 else if (l__regs.ebx == REGS_X86_POINTERS)
			{
				l__flagstr = "REGS_X86_POINTERS";
				l__reg1 = "<esp>";
				l__reg2 = "<eip>";
				l__reg3 = "<xxx>";
				l__reg4 = "<xxx>";				
			}			
 			 else if (l__regs.ebx == REGS_X86_EFLAGS)
			{
				l__flagstr = "REGS_X86_EFLAGS";
				l__reg1 = "<eflags>";
				l__reg2 = "<xxx>";
				l__reg3 = "<xxx>";
				l__reg4 = "<xxx>";				
			}			
			 else
			{
				l__flagstr = dbg_str_invalid;
				l__reg1 = "<xxx>";
				l__reg2 = "<xxx>";
				l__reg3 = "<xxx>";
				l__reg4 = "<xxx>";				
			}				
			
			l__len = snprintf(l__buf, 1000, "D4: write_regs(sid=0x%X, regtp = 0x%X = %s, %s=0x%X, %s=0x%X, %s=0x%X, %s=0x%X)", l__regs.eax, l__regs.ebx, l__flagstr, l__reg1, l__regs.ecx, l__reg2, l__regs.edx, l__reg3, l__regs.esi, l__reg4, l__regs.edi);
			break;
		}
		
		/* set_paged */
		case (0xD5):
		{
			l__len = snprintf(l__buf, 1000, "D5: set_paged()");
			break;
		}	
		
		/* test_page */
		case (0xD6):
		{
			l__len = snprintf(l__buf, 1000, "D6: test_page(adr=0x%X, sid = 0x%X) => flags->EBX", l__regs.eax, l__regs.ebx);
			break;
		}	
		
		/* Invalid system call */
		default:
		{
			l__len = snprintf(l__buf, 1000, "%X: invalid syscall !", sysc);
			break;
		}
	}
	
	l__buf = mem_realloc(l__buf, l__len + 1);
	if (l__buf == NULL) return NULL;
	l__buf[l__len] = 0;
	
	return l__buf;
}

/*
 * dbg_analyze_exception(sysc, client)
 *
 * Analyzes the exception "intr" of the client "client".
 * The output will be returned into a string.
 *
 * Return value:
 *	!= NULL   Information string
 *	== NULL	  Invalid interrupt or error
 *
 */ 
utf8_t* dbg_analyze_exception(int intr, sid_t client)
{
	/* Invalid SID? */
	if (!hysys_thrtab_read(client, THRTAB_IS_USED))
	{
		return NULL;
	}
	
	/* Get the exception informations */
	uint32_t l__excpt_address = hysys_thrtab_read(client, THRTAB_LAST_EXCPT_ADDRESS);
	uint32_t l__excpt_errcode = hysys_thrtab_read(client, THRTAB_LAST_EXCPT_ERROR_CODE);
	uint32_t l__excpt_pfault_desc = hysys_thrtab_read(client, THRTAB_PAGEFAULT_DESCRIPTOR);
	uint32_t l__excpt_pfault_adr = hysys_thrtab_read(client, THRTAB_PAGEFAULT_LINEAR_ADDRESS);
	
	utf8_t *l__buf = mem_alloc(1000);
	int l__len = 0;
	if (l__buf == NULL) return NULL;
	
	switch (intr)
	{
		case (EXC_X86_DIVISION_BY_ZERO):
		{
			l__len = snprintf(l__buf, 1000, "00: #DE DIVISION_BY_ZERO (adr 0x%X)", l__excpt_address);
			break;
		}
		case (EXC_X86_DEBUG_EXCEPTION):
		{
			l__len = snprintf(l__buf, 1000, "01: #DB DEBUG_EXCEPTION (adr 0x%X)", l__excpt_address);
			break;
		}
		case (EXC_X86_NOT_MASKABLE_INTERRUPT):
		{
			l__len = snprintf(l__buf, 1000, "02: NMI NOT_MASKABLE_INTERRUPT (adr 0x%X)", l__excpt_address);
			break;
		}	
		case (EXC_X86_BREAKPOINT):
		{
			l__len = snprintf(l__buf, 1000, "03: #BP BREAK_POINT (adr 0x%X)", l__excpt_address);
			break;
		}	
		case (EXC_X86_OVERFLOW_EXCEPTION):
		{
			l__len = snprintf(l__buf, 1000, "04: #OF OVERFLOW (adr 0x%X)", l__excpt_address);
			break;
		}	
		case (EXC_X86_BOUND_RANGE_EXCEEDED):
		{
			l__len = snprintf(l__buf, 1000, "05: #BR BOUND_RANGE_EXCEEDED (adr 0x%X)", l__excpt_address);
			break;
		}	
		case (EXC_X86_INVALID_OPCODE):
		{
			l__len = snprintf(l__buf, 1000, "06: #UD INVALID_OPCODE (adr 0x%X)", l__excpt_address);
			break;
		}		
		case (EXC_X86_DEVICE_NOT_AVAILABLE):
		{
			l__len = snprintf(l__buf, 1000, "07: #NM FPU_DEVICE_NOT_AVAILABLE (adr 0x%X)", l__excpt_address);
			break;
		}					
		case (EXC_X86_DOUBLE_FAULT):
		{
			l__len = snprintf(l__buf, 1000, "08: #DF DOUBLE_FAULT (adr 0x%X)", l__excpt_address);
			break;
		}	
		case (EXC_X86_COPROCESSOR_SEGMENT_OVERRUN):
		{
			l__len = snprintf(l__buf, 1000, "09: XXX COPROCESSOR_SEGMENT_OVERRUN (adr 0x%X)", l__excpt_address);
			break;
		}	
		case (EXC_X86_INVALID_TSS):
		{
			l__len = snprintf(l__buf, 1000, "0A: #TS INVALID_TSS (adr 0x%X, code 0x%X)", l__excpt_address, l__excpt_errcode);
			break;
		}
		case (EXC_X86_SEGMENT_NOT_PRESENT):
		{
			l__len = snprintf(l__buf, 1000, "0B: #NP SEGMENT_NOT_PRESENT (adr 0x%X, code 0x%X)", l__excpt_address, l__excpt_errcode);
			break;
		}	
		case (EXC_X86_STACK_FAULT):
		{
			l__len = snprintf(l__buf, 1000, "0C: #SS STACK_FAULT (adr 0x%X, code 0x%X)", l__excpt_address, l__excpt_errcode);
			break;
		}	
		case (EXC_X86_GENERAL_PROTECTION_FAULT):
		{
			l__len = snprintf(l__buf, 1000, "0D: #GP GENERAL_PROTECTION_FAULT (adr 0x%X, code 0x%X)", l__excpt_address, l__excpt_errcode);
			break;
		}	
		case (EXC_X86_PAGE_FAULT):
		{
			l__len = snprintf(l__buf, 1000, "0E: #PF PAGE_FAULT (adr 0x%X, code 0x%X, page 0x%X, descr 0x%X)", l__excpt_address, l__excpt_errcode, l__excpt_pfault_adr, l__excpt_pfault_desc);
			break;
		}													
		case (EXC_X86_FLOATING_POINT_ERROR):
		{
			l__len = snprintf(l__buf, 1000, "10: #MF DOUBLE_FAULT (adr 0x%X)", l__excpt_address);
			break;
		}
		case (EXC_X86_ALIGNMENT_CHECK):
		{
			l__len = snprintf(l__buf, 1000, "11: #AC ALIGNMENT_CHECK (adr 0x%X)", l__excpt_address);
			break;
		}	
		case (EXC_X86_MACHINE_CHECK):
		{
			l__len = snprintf(l__buf, 1000, "12: #MC MACHINE_CHECK (adr 0x%X)", l__excpt_address);
			break;
		}	
		case (EXC_X86_SIMD_FLOATING_POINT_ERROR):
		{
			l__len = snprintf(l__buf, 1000, "13: #XF SIMD_FLOATING_POINT_ERROR (adr 0x%X)", l__excpt_address);
			break;
		}				
		default:
		{
			l__len = snprintf(l__buf, 1000, "%X: invalid or empty exception !", intr);
			break;
		}
	}
	
	l__buf = mem_realloc(l__buf, l__len + 1);
	if (l__buf == NULL) return NULL;
	l__buf[l__len] = 0;
	
	return l__buf;
}
/*
 * dbg_analyze_softing(term, intr, client)
 *
 * Analyzes a software interrupt "intr" of the client "client".
 * The output will be returned to a string.
 *
 * Return value:
 *	!= NULL   Information string
 *	== NULL	  Invalid interrupt or error
 *
 */
utf8_t* dbg_analyze_softint(int intr, sid_t client)
{
	if (intr > 0xFF)
	{
		return NULL;
	}
	
	if (intr <= 0x1F) /* Exceptions */
	{
		return dbg_analyze_exception(intr, client);
	}
	 else if (intr == 0xB0)
	{
		utf8_t *l__buf = mem_alloc(100);
		if (l__buf == NULL) return NULL;
		int l__len = snprintf(l__buf, 100, "Debugger instruction");
		l__buf = mem_realloc(l__buf, l__len + 1);
		if (l__buf == NULL) return NULL;
		l__buf[l__len] = 0;
		
		return l__buf;
	}
	 else if ((intr >= 0xA0) && (intr <= 0xAF)) /* IRQs */
	{
		utf8_t *l__buf = mem_alloc(100);
		if (l__buf == NULL) return NULL;
		int l__len = snprintf(l__buf, 100, "Empty-Softint 0x%X (used for IRQ 0x%X)", intr, intr - 0xA0);
		l__buf = mem_realloc(l__buf, l__len + 1);
		if (l__buf == NULL) return NULL;
		l__buf[l__len] = 0;
		
		return l__buf;
	}
	 else if ((intr < 0xC0) || (intr > 0xD6)) /* Empty-Ints */
	{
		utf8_t *l__buf = mem_alloc(100);
		if (l__buf == NULL) return NULL;
		int l__len = snprintf(l__buf, 100, "Empty-Softint 0x%X (unused)", intr, intr - 0xA0);
		mem_realloc(l__buf, l__len + 1);
		if (l__buf == NULL) return NULL;
		l__buf[l__len] = 0;
		
		return l__buf;
	}
	 else if ((intr >= 0xC0) && (intr <= 0xD6)) /* Syscalls */
	{
		return dbg_analyze_syscall(intr, client);
	}

	return NULL;
}

/*
 * dbg_sh_dumpint
 *
 * Dumps the meaning of an interrupt
 *
 * Usage:
 *	dumpint num -c <client>
 *	 	num		The number of the software interrupt
 *	 	-c sid		Use a special sid (if not defined the current client will be used)
 *
 */
int dbg_sh_dumpint(void)
{
	dbg_shell_t *l__shell = *dbg_tls_shellptr;
	sid_t l__sid = 0;
	unsigned l__num;
		
	/* Right count of parameters? */
	if (l__shell->n_pars < 2)
	{
		dbg_iprintf(l__shell->terminal, "Missing parameters. Try \"help dumpint\" for more informations.\n");
		return -1;
	}
		
	/* Get the interrupt number */
	if (dbglib_atoul(l__shell->pars[1], &l__num, 16))
	{
		dbg_iprintf(l__shell->terminal, "Can't convert parameter - \"%s\".\n", l__shell->pars[1]);
		return -1;
	}
	
	if (l__num > 255)
	{
		dbg_iprintf(l__shell->terminal, "Invalid interrupt number 0x%X.\n", l__num);
		return -1;
	}
	
	/* Was a SID defined? */
	l__sid = dbg_get_sidpar("dumpint", SIDTYPE_THREAD);
	if (l__sid == SID_INVALID) return -1;
	
	/* Load interrupt information */
	utf8_t *l__info = dbg_analyze_softint(l__num, l__sid);
	if (l__info == NULL)
	{
		dbg_iprintf(l__shell->terminal, "Can't retreive informations about interrupt 0x%X of thread 0x%X.\n", l__num, l__sid);
		return -1;
	}
	
	dbg_iprintf(l__shell->terminal, "Informations about interrupt 0x%X of client 0x%X:\n\n%s\n", l__num, l__sid, l__info);
	mem_free(l__info);
	
	return 0;
}

