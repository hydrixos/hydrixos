/*
 *
 * tls.c
 *
 * (C)2005 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library').   
 *
 * Thread local storage support
 *
 */
#include <hydrixos/types.h>
#include <hydrixos/tls.h>
#include <hydrixos/errno.h>
#include "../../hybaselib.h"

/* Pointer to the next free global TLS entry */
static void **tls_global_pointer = NULL;

/* End of the global TLS area */
static void **tls_global_end = NULL;

/* Pointer to the pointer to the next free local TLS entry */
static void ***tls_local_pointer = NULL;

/* End of the local TLS area */
static void **tls_local_end = NULL;

/* The thread error status */
errno_t *tls_errno;

/*
 * lib_init_global_tls()
 *
 * Initializes the global TLS managment and the global TLS areas
 * of the hyBaseLib. This function will be called by 
 * lib_init_hybaselib during the library initialization.
 *
 * Return value:
 *	0	Operation successful.
 *	1	Operation failed.
 *
 */
int lib_init_global_tls()
{
	tls_global_pointer = (void*)(uintptr_t)0xBFFFF008;
	tls_global_end = (void*)(uintptr_t)0xBFFFF800;
	
	tls_local_pointer = (void*)(uintptr_t)0xBFFFF800;
	tls_local_end = (void*)(uintptr_t)0xC0000000;
	*tls_local_pointer = tls_global_end;
	
	tls_errno = (void*)(uintptr_t)0xBFFFF004;
	*tls_errno = 0;
	
	return 0;
}

/*
 * tls_global_alloc()
 *
 * Allocates a global TLS area. This function will increment
 * the tls_global_pointer by sizeof(void**) after allocation
 * of the area. The global TLS reaches its end, if tls_global_pointer
 * is the same as tls_global_end before the incrementation of
 * tls_global_pointer.
 *
 * Return value:
 *	Pointer to the allocated global TLS area (which is
 *	an pointer to a pointer "void*"). NULL if failed.
 *
 */
void** tls_global_alloc(void)
{
	void **l__retval = tls_global_pointer;
	
	if (tls_global_pointer == tls_global_end)
	{
		*tls_errno = ERR_NOT_ENOUGH_MEMORY;
		return NULL;
	}
	
	tls_global_pointer += 4;
	
	return l__retval;
}

/*
 * tls_local_alloc()
 *
 * Allocates a local TLS area. This function will increment
 * the tls_local_pointer by sizeof(void**) after allocation
 * of the area. The allocation failes, if the local pointer
 * tls_local_pointer is the same as tls_local_end before
 * the incrementation of tls_local_pointer.
 *
 * Return value:
 *	Pointer to the allocated global TLS area (which is
 *	an pointer to a pointer "void*"). NULL if failed.
 *
 */
void** tls_local_alloc(void)
{
	void **l__retval = *tls_local_pointer;
	
	if (*tls_local_pointer == tls_local_end)
	{
		*tls_errno = ERR_NOT_ENOUGH_MEMORY;
		return NULL;
	}
	
	tls_local_pointer += 4;
	
	return l__retval;
}
