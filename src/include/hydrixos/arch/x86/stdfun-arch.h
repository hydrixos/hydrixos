/*
 *
 * stdfun-arch.h
 *
 * (C)2005 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library'). 
 *
 * x86: Architecture-dependend implementation of
 *      the common helper functions
 *
 */ 
#ifndef _STDFUN_ARCH_H
#define _STDFUN_ARCH_H

#include <hydrixos/types.h>
#include <hydrixos/hysys.h>
#include <hydrixos/tls.h>
#include <hydrixos/errno.h>

#ifndef HYDRIXOS_x86
	#error "This architecture-dependend implementation of stdfun.h is only valid for x86 systems."
#endif

#ifndef HYDRIXOS_USE_STDFUN_ARCH
	#error "There is no valid architecture-dependend implementation of stdfun available."
#endif

void *memcpy(void *dest, const void *src, size_t n);

/*
 * buf_copy(dest, src, num)
 *
 * Copies "num" bytes of the content of the buffer "src" to the
 * buffer "dest".
 *
 * Return value:
 *	Number of the bytes.
 *
 * |-- x86 optimized implementation --|
 *
 */
#define HAVE_STDFUN_ARCH_BUF_COPY
static inline size_t buf_copy(void* dest, const void* src, size_t num)
{
	if ((dest == NULL) || (src == NULL))
	{
		*tls_errno = ERR_INVALID_ADDRESS;
		return 0;
	}
	
	if (!(num % 4))
	{
		int d1, d2, d3;
		__asm__ __volatile__ ("cld\n"
			      	      "rep movsl"
			      	      : "=D" (d1), "=S" (d2), "=c" (d3)
			      	      : "c" (num / 4),
			      	        "S" (src),
			      	        "D" (dest)
			      	      : "memory"
			      	     );
	   	return num;
	}
	
	if (!(num % 2))
	{
		int d1, d2, d3;
		__asm__ __volatile__ ("cld\n"
			      	      "rep movsw"
			      	      : "=D" (d1), "=S" (d2), "=c" (d3)
			      	      : "c" (num / 2),
			      	        "S" (src),
			      	        "D" (dest)
			      	      : "memory"
			      	     );
	   	return num;	
	}
	
	int d1, d2, d3;
	__asm__ __volatile__ ("cld\n"
			      "rep movsb"
			      : "=D" (d1), "=S" (d2), "=c" (d3)
			      : "c" (num),
			        "S" (src),
			        "D" (dest)
			      : "memory"
			     );
	return num;		
}

/*
 * buf_fill(dest, num, val)
 *
 * Fills the buffer "dest" with the value "val". The function will repeat 
 * this filling operation until "num" bytes have been written.
 *
 * Return value:
 *	Number of the written bytes.
 *
 * |-- x86 optimized implementation --|
 *
 */
#define HAVE_STDFUN_ARCH_BUF_FILL
static inline size_t buf_fill(void* dest, size_t num, uint8_t val)
{
	int d1, d3;
	
	if (dest == NULL)
	{
		*tls_errno = ERR_INVALID_ADDRESS;
		return 0;
	}
	
	__asm__ __volatile__ ("cld\n"
		      	      "rep stosb\n"
		      	      : "=D" (d1), "=c" (d3)
		      	      : "c" (num),
		      	        "D" (dest),
		      	        "a" (val)
		      	      : "memory"
		      	     );
		     
	return num;
}

/*
 * buf_compare(dest, src, num)
 *
 * Compares the buffers "dest" and "src" until "num" bytes
 * have been compared or the content of "dest" and "src"
 * differs. 
 *
 * Return value:
 *	"dest" is graeter (>0), smaller (<0) or equal to src (= 0)
 *
 * |-- x86 optimized implementation --|
 *
 */
#define HAVE_STDFUN_ARCH_BUF_COMPARE
static inline int buf_compare(const void* dest, const void* src, size_t num)
{
	const uint8_t *l__dest = dest;
	const uint8_t *l__src = src;
	int d1;
	
	if ((dest == NULL) || (src == NULL))
	{
		*tls_errno = ERR_INVALID_ADDRESS;
		return 0;
	}
	
	__asm__ __volatile__("cld\n"
		      	     "1:\n"
		      	     "cmpsb\n"
		      	     "loope 1b\n"
		      	     : "=D" (l__dest),
		      	       "=S" (l__src),	    
		      	       "=c" (d1)
		      	     : "c" (num),
		      	       "D" (l__dest),
		      	       "S" (l__src)
		      	     : "memory"
	   	            );
	   	    
	l__dest --;
	l__src --;
	   	    
	return *l__dest - *l__src;
}

/*
 * buf_find_uint8(dest, num, val)
 *
 * Searches the value "val" within the first "num" bytes
 * of the buffer "dest".
 *
 * Return value:
 *	Pointer to the matched byte
 *	NULL if failed.
 *
 * |-- x86 optimized implementation --|
 *
 */
#define HAVE_STDFUN_ARCH_BUF_FIND_UINT8
static inline void* buf_find_uint8(const void* dest, size_t num, uint8_t val)
{
	uint8_t *l__retval = (uint8_t*)dest;
	int d1;
	
	if (dest == NULL)
	{
		*tls_errno = ERR_INVALID_ADDRESS;
		return NULL;
	}
	
	__asm__ __volatile__("cld\n"
		      	     "repnz scasb\n"
		      	     : "=D" (l__retval), "=c" (d1)
		      	     : "D" (dest),
		      	       "a" (val),
		      	       "c" (num)
		      	     : "memory"
		      	    );
	
	if (*(l__retval - 1) != val) return NULL;
	
	return (void*)(l__retval - 1);
}

/*
 * buf_find_uint16(dest, num, val)
 *
 * Searches the value "val" within the first "num" bytes
 * of the buffer "dest". "num" has to be divisible by
 * sizeof(val).
 *
 * Return value:
 *	Pointer to the matched byte
 *	NULL if failed. 
 *
 * |-- x86 optimized implementation --|
 *
 */
#define HAVE_STDFUN_ARCH_BUF_FIND_UINT16
static inline void* buf_find_uint16(const void* dest, size_t num, uint16_t val)
{
	uint16_t *l__retval = (uint16_t*)dest;
	int d1;
	
	if (dest == NULL)
	{
		*tls_errno = ERR_INVALID_ADDRESS;
		return NULL;
	}
	
	/* Is sizeof(val) a multiplier of num? */
	if (num % sizeof(val))
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		return NULL;
	}
	
	num /= sizeof(val);	
	
	__asm__ __volatile__("cld\n"
		      	     "repnz scasw\n"
		      	     : "=D" (l__retval), "=c" (d1)
		      	     : "D" (dest),
		      	       "a" (val),
		      	       "c" (num)
		      	     : "memory"
		      	    );
	
	if (*(l__retval - 1) != val) return NULL;
	
	return (void*)(l__retval - 1);
}

/*
 * buf_find_uint32(dest, num, val)
 *
 * Searches the value "val" within the first "num" bytes
 * of the buffer "dest". "num" has to be divisible by
 * sizeof(val).
 *
 * Return value:
 *	Pointer to the matched byte
 *	NULL if failed. 
 *
 * |-- x86 optimized implementation --|
 *
 */
#define HAVE_STDFUN_ARCH_BUF_FIND_UINT32
static inline void* buf_find_uint32(const void* dest, size_t num, uint32_t val)
{
	uint32_t *l__retval = (uint32_t*)dest;
	int d1;
	
	if (dest == NULL)
	{
		*tls_errno = ERR_INVALID_ADDRESS;
		return NULL;
	}
	
	/* Is sizeof(val) a multiplier of num? */
	if (num % sizeof(val))
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		return NULL;
	}
	
	num /= sizeof(val);	
	
	__asm__ __volatile__("cld\n"
 		 	     "repnz scasl\n"
 		 	     : "=D" (l__retval), "=c" (d1)
 		 	     : "D" (dest),
 		 	       "a" (val),
 		 	       "c" (num)
 		 	     : "memory"
 		 	    );
	
	if (*(l__retval - 1) != val) return NULL;
	
	return (void*)(l__retval - 1);
}

/*
 * There is no plattform-optimized implementation for
 * buf_find_uint64 and buf_find_buf currently available
 */
#undef HAVE_STDFUN_ARCH_BUF_FIND_UINT64
void* buf_find_uint64(const void* dest, size_t num, uint64_t val);

#undef HAVE_STDFUN_ARCH_BUF_FIND_BUF
void* buf_find_buf(const void* dest, size_t dnum, const void *src, size_t snum);

#undef HAVE_STDFUN_ARCH_STR_LEN
size_t str_len(const utf8_t* dest, size_t max);
#undef HAVE_STDFUN_ARCH_STR_COPY
size_t str_copy(utf8_t* dest, const utf8_t* src, size_t max);
#undef HAVE_STDFUN_ARCH_STR_COMPARE
int str_compare(const utf8_t* dest, const utf8_t* src, size_t max);
#undef HAVE_STDFUN_ARCH_STR_CHAR
utf8_t* str_char(const utf8_t* dest, char sgn, size_t max);
#undef HAVE_STDFUN_ARCH_STR_FIND
utf8_t* str_find(const utf8_t* dest, size_t dmax, const utf8_t* src, size_t smax);

#endif
