/*
 *
 * mem.h
 *
 * (C)2005 by Friedrich Gr�ter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library').  
 *
 * Memory managment functions
 *
 */ 
#ifndef _MEM_H
#define _MEM_H

#include <hydrixos/types.h>
#include <hydrixos/list.h>
#include <hydrixos/mutex.h>

/*
 * Region managment
 *
 */
/* The memory region data structure */
typedef struct 
{
	utf8_t		name[32];         /* Name of the region */
	unsigned	id;               /* ID of the region */

	void*		start;            /* Start address */
	unsigned	pages;            /* Size of the region in pages */

	int		flags;            /* Flags (see REGFLAGS_*) */
	unsigned	usable_pages;     /* Count of usable pages */
	unsigned	readable_pages;   /* Count of readable pages */
	unsigned	writeable_pages;  /* Count of writeable pages */
	unsigned	executable_pages; /* Count of executable pages */
	unsigned	shared_pages;     /* Count of shared pages */

	void*	(*alloc)(size_t sz);     /* Allocator function */
	void	(*free)(void* ptr);      /* Free function */

	unsigned	chksum;		/* Descriptor checksum */
	list_t		ls;		/* (Linked list) */
}region_t;

#define REGFLAGS_READABLE		1
#define REGFLAGS_WRITEABLE		2
#define REGFLAGS_EXECUTABLE		4
#define REGFLAGS_SHARED			8

/* Region managment functions */
region_t* reg_create(region_t region);
void reg_destroy(region_t *region);

/* Region managment: global informations */
extern region_t *regions; 	/* Linked list of memory regions */
extern region_t *code_region;	/* Descriptor of the code region */
extern region_t *data_region; 	/* Descriptor of the data region */
extern region_t *stack_region;	/* Descriptor of the stack region */
extern region_t *heap_region;	/* Descriptor of the heap region */

mtx_t reg_mutex; 		/* Mutex of the region table */

/*
 * Heap managment
 *
 */
/* Heap managment: background functions */
void mem_heap_inc(unsigned pages);
void mem_heap_dec(unsigned pages);

/* Heap managment API */
void* mem_alloc(size_t sz);
void* mem_realloc(void* mem, size_t nsz);
void mem_free(void* mem);

size_t mem_size(void *area);

/*
 * Stack managment
 *
 */
void* mem_stack_alloc(size_t sz);
void mem_stack_free(void* stack);


#endif
