/*
 *
 * string.c
 *
 * (C)2001, 2002 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g.
 * in the file 'copying'). 
 *
 * Kernel internal string functions.
 *
 */
#include <hydrixos/types.h>
#include <string.h>

/***********************************************************************/
/* strcpy  							       */
/***********************************************************************/
char *strcpy(char *dest, const char *src)
{
	char* l__retval = dest;

	while((*dest++ = *src++) != '\0')
		;
	return l__retval;
}


/***********************************************************************/
/* strlen   							       */
/***********************************************************************/
size_t strlen(const char *s)
{
   const char *l__retval;

   for (l__retval = s; *l__retval != '\0'; ++l__retval)
   		;
   return l__retval - s;
}

/***********************************************************************/
/* memcpy   							       */
/***********************************************************************/
void *memcpy(void *dest, const void *src, size_t n)
{
	void* l__retval = dest;

	while(n--)
		*((unsigned char*)(dest++)) = *((unsigned char*)(src++));

	return l__retval;
}

