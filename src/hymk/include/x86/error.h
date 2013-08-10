/*
 *
 * error.h
 *
 * (C)2004 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g.
 * in the file 'copying'). 
 *
 *  System call error handling
 *
 */
#ifndef _ERROR_H
#define _ERROR_H

#include <hydrixos/types.h>
#include <hydrixos/errno.h>
#include <stdio.h>
#include <setup.h>

/*
 * The last error code produced by a system call
 *
 */
extern errno_t sysc_error;

/*
 * If wanted, write an error message on the screen
 *
 */
#ifdef DEBUG_MODE
	#define ERROR_OUT(___num)\
	({\
		kprintf("System call error: 0x%X (0x%X)\n", ___num, current_t[THRTAB_SID]);\
		kprintf("Occured in: %s\n", __FUNCTION__);\
		___num;\
	})
#else
	#define ERROR_OUT(___num)	({___num;})
#endif
	
/*
 * Set the error return value of the current system call
 *
 */
#define SET_ERROR(___num)	\
({\
	sysc_error = ___num;\
	if (___num != 0)\
	{\
		ERROR_OUT(___num);\
	}\
	___num;\
})

#endif
