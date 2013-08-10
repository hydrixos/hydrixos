/*
 *
 * hybaselib.h
 *
 * (C)2005 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library').   
 *
 * Helper functions of the hyBaseLib implementation
 *
 */ 
#include <hydrixos/blthr.h>

#ifndef _HYBASELIB_H
#define _HYBASELIB_H

/* TLS managment (arch/tls.c) */
int lib_init_global_tls(void);

/* Region managment (region.c) */
int lib_init_regions(void);

/* Heap managment (memalloc.c) */
int lib_init_heap(void);

/* BlThread initialization (blthrd.c) */
int lib_init_blthreads(void* stack);
uintptr_t lib_blthr_setup_stack_arch(thread_t *thr);

/* Library initialization (libinit.c) */
void* lib_init_hybaselib(void);

/* Mapping region managment */
int lib_init_pmap(void);

/* Simple debugging console */
int iprintf(utf8_t* fm, ...);
extern volatile utf8_t* i__screen;
extern int i__is_online;

#endif
