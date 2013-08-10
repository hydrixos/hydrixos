/*
 *
 * string.h
 *
 * (C)2001, 2002 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g.
 * in the file 'copying'). 
 *
 */
#ifndef _STRING_H
#define _STRING_H

#include <hydrixos/types.h>


size_t strlen(const char *s);
char *strcpy(char *dest, const char *src);
void* memcpy(void *dest, const void *src, size_t n);


#endif


