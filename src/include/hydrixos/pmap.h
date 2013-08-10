/*
 *
 * pmap.h
 *
 * (C)2005 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library'). 
 *
 * Managment of the page mapping region
 *
 */ 
#ifndef _PMAP_H
#define _PMAP_H

#include <hydrixos/types.h>

void* pmap_alloc(size_t sz);
void pmap_free(void* ptr);
void* pmap_mapalloc(size_t sz);

#endif
