/*
 *
 * hymk.h
 *
 * (C)2005 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library'). 
 *
 * HydrixOS microkernel system call interfaces
 *
 */ 
#ifndef _HYMK_H
#define _HYMK_H

#include <hydrixos/types.h>
#include <hydrixos/hysys.h>

/*
 * x86-specific Implementation of the kernel API
 *
 */
#ifdef HYDRIXOS_x86
	#include <hymk/syscall.h>
#endif

#endif
