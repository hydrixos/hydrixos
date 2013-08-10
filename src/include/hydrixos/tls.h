/*
 *
 * tls.h
 *
 * (C)2005 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library'). 
 *
 * Managment functions for thread local storage
 *
 */ 
#ifndef _TLS_H
#define _TLS_H

#include <hydrixos/types.h>

void** tls_global_alloc(void);
void** tls_local_alloc(void);

/* Predefined global TLS variables */
extern errno_t *tls_errno;

#endif
