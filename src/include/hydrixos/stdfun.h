/*
 *
 * stdfun.h
 *
 * (C)2005 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library'). 
 *
 * Common helper functions
 *
 */ 
#ifndef _STDFUN_H
#define _STDFUN_H

#include <hydrixos/types.h>
#include <hydrixos/hysys.h>

#ifndef HYDRIXOS_USE_STDFUN_ARCH
	/* Buffer manipulation */
	size_t buf_copy(void* dest, const void* src, size_t num);
	size_t buf_fill(void* dest, size_t num, uint8_t val);
	int buf_compare(const void* dest, const void* src, size_t num);
	void* buf_find_uint8(const void* dest, size_t num, uint8_t val);
	void* buf_find_uint16(const void* dest, size_t num, uint16_t val);
	void* buf_find_uint32(const void* dest, size_t num, uint32_t val);
	void* buf_find_uint64(const void* dest, size_t num, uint64_t val);
	void* buf_find_buf(const void* dest, size_t dnum, const void *src, size_t snum);

	/* Manipulation of zero terminated strings*/
	size_t str_copy(utf8_t* dest, const utf8_t* src, size_t max);
	int str_compare(const utf8_t* dest, const utf8_t* src, size_t max);
	size_t str_len(const utf8_t* dest, size_t max);
	utf8_t* str_char(const utf8_t* dest, char sgn, size_t max);
	utf8_t* str_find(const utf8_t* dest, size_t dmax, const utf8_t* src, size_t smax);
	
	void *memcpy(void *dest, const void *src, size_t n);
#else
	#ifdef HYDRIXOS_x86
		#include <hydrixos/arch/x86/stdfun-arch.h>
	#endif
#endif


#endif
