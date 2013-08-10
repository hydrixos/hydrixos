/*
 *
 * stack.c
 *
 * (C)2005 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library').   
 *
 * Managment of the stack
 *
 */
#include <hydrixos/types.h>
#include <hydrixos/tls.h>
#include <hydrixos/hymk.h>
#include <hydrixos/errno.h>
#include <hydrixos/mem.h>
#include <hydrixos/mutex.h>
#include <hydrixos/stdfun.h>
#include "hybaselib.h"


/*
 * mem_stack_alloc(sz)
 *
 * Allocates a stack region of "sz" bytes
 * and returns the pointer to the upper
 * end of the stack. (You can get the
 * stack pointer to the beginnig of the
 * stack by adding sz Bytes to the returned 
 * address.)
 *
 * Return value: The stack address.
 *
 * TODO: This function allocates the stack
 *       area on the heap. This is a bad
 *       idea, because you can't trace
 *       stack overruns and you are creating
 *	 a lot of unuseable heap areas.
 *
 */
void* mem_stack_alloc(size_t sz)
{
	return mem_alloc(sz);
}

/*
 * mem_stack_free(stack)
 *
 * Frees the stack area "stack". The
 * pointer points to the upper end of
 * the stack. The usage of a normal
 * stack pointer is not allowed.
 *
 * TODO: See mem_stack_alloc.
 *
 */
void mem_stack_free(void* stack)
{
	mem_free(stack);
	
	return;
}


