/*
 *
 * security.c
 *
 * (C)2004 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying'). 
 *
 * Security functions
 *
 */
#include <hydrixos/types.h>
#include <hydrixos/sid.h>
#include <setup.h>
#include <stdio.h>
#include <info.h>
#include <sysc.h>
#include <error.h>
#include <sched.h>
#include <sysc.h>

/*
 * sysc_chg_root(proc, op)
 *
 * (Implementation of the "chg_root" system call)
 *
 * Switches a process to root or non-root mode. This system
 * call is restricted to root-mode processes only. 
 *
 * Parameters:
 *	proc	SID of the affected
 *	op	The wanted operation:
 *			CHGROOT_LEAVE		0	Leave Root-Mode
 *			CHGROOT_ENTER		1	Enter Root-Mode
 *
 */
void sysc_chg_root(sid_t proc, int op)
{
	/* Only root processes are allowed to change the root state */
	if (current_p[PRCTAB_IS_ROOT] != 1)
	{
		SET_ERROR(ERR_ACCESS_DENIED);
		return;
	}

	/* Valid process SID? */
	if (!kinfo_isproc(proc))
	{
		SET_ERROR(ERR_INVALID_SID);
		return;
	}
	
	/* Valid operation type? */
	if ((op != CHGROOT_ENTER) && (op != CHGROOT_LEAVE))
	{
		return;
	}
	
	/* Set */
	PROCESS(proc, PRCTAB_IS_ROOT) = op;
	
	/* Set I/O access rights */
	if (op == CHGROOT_ENTER) 
	{
		/* Set rights */
		PROCESS(proc, PRCTAB_IO_ACCESS_RIGHTS) |=    IO_ALLOW_IRQ
							   | IO_ALLOW_PORTS;	
	}
	 else 
	{
		/* Remove rights */
		PROCESS(proc, PRCTAB_IO_ACCESS_RIGHTS) &= (~(   IO_ALLOW_IRQ
							      | IO_ALLOW_PORTS
							  ));
	}
						   
	return;
}
