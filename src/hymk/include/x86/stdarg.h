/*
 *
 * stdarg.h
 *
 * (C)2001, 2002 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g.
 * in the file 'copying'). 
 *
 * Implementation of ISO-C <stdarg.h>
 *
 */
#ifndef _STDARG_H
#define _STDARG_H

#include <hydrixos/types.h>

typedef char *va_list;

#define va_start(ap,v)  ap = (va_list)&v + sizeof(v)
#define va_arg(ap,t)    ( (t *)(ap += sizeof(t)) )[-1]
#define va_end(ap)      ap = NULL

#endif 

