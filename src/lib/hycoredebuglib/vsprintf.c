/*
 *
 * vsprintf.c
 *
 * (C)2006 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library').   
 *
 * Simplified implementation of vsnprintf
 *
 */
#include <hydrixos/types.h>
#include <hydrixos/stdfun.h>
#include "./hycoredbg.h"
#include <coredbg/cdebug.h>


static const char *null_text = "( NULL )";

#define va_start(ap,v)  ap = (va_list)&v + sizeof(v)
#define va_arg(ap,t)    ( (t *)(ap += sizeof(t)) )[-1]
#define va_end(ap)      ap = NULL

/*
 * int_to_num(buf, prec, flags, val, base)
 *
 * Converts the number "val" into a string and stores it in
 * "buf". The number will be displayed to the basis "base".
 * If prec>0 the string will have at least "prec" digits (unused digits. 
 * will be filled with zeros).
 *
 * You can modifiy the behaviour of the function with the
 * parameter"flags":
 *	INTTONUM_SMALL_DIGITS		Use small digits (otherwise big)
 *	INTTONUM_SIGNED			Number is signed
 *
 * Return value:
 *	Number of printed charracters
 *
 */
#define INTTONUM_SMALL_DIGITS		1
#define INTTONUM_SIGNED			2
static int int_to_num(char* buf, int prec, int flags, int val, int base)
{
	const char *l__digits = "0123456789ABCDEF";
	int l__dnum = 0;
	int l__ctr = 0;
	
	if ((buf == NULL) || (base < 2) || (base > 16)) return 0;
	
	/* Using small digits? */
	if (flags & INTTONUM_SMALL_DIGITS)
		l__digits = "0123456789abcdef";
	
	/* Signed number? */
	if (flags & INTTONUM_SIGNED)
	{
		/* We need a sign? Put it... */
		if (val < 0)
		{
			*buf ++ = '-';
			l__ctr ++;
			val *= -1;
		}
	}
	
	unsigned l__tmpval = val;
	
	/* Count the digits */
	while (l__tmpval > 0)
	{
		l__tmpval /= base;
		
		l__dnum ++;
	}
	
	/* Write the output */
	if (l__dnum < prec)
	{
		/* Fill it with zeros */
		int l__diff = prec - l__dnum;
		
		while (l__diff --)
		{
			*buf ++ = '0';
			l__ctr ++;
		}
	}
			
	buf += l__dnum - 1;
	l__tmpval = val;
			
	while (l__dnum --)
	{
		*buf = l__digits[l__tmpval % base];
		
		l__tmpval /= base;
		l__ctr ++;
		buf --;
	}
	
	return l__ctr;
}

/*
 * vsnprintf(bf, sz, fm, ap)
 *
 * Prints the format string "fm" with its extended parameters
 * stored in "ap" into the buffer "bf". The function terminates
 * automatically if 'sz' bytes have been written.
 *
 * Remarks:
 *	This is a simplified version of the original vsnprintf.
 *	We support:
 *		- Precision
 *		- Type specifiers: c s u i X o
 *		- Length modifiers: l L
 *
 *	As a difference to the ISO-C vsnprintf our length-modifier
 *	for string defines a minimum size!
 *
 * Return value:
 *	Number of printed chars.
 *
 */
int vsnprintf(char *bf, size_t sz, const char *fm, va_list ap)
{
	int l__ctr = 0;
	int l__prec = 1;
		
	/* Do it until "fm" == 0 */
	while (*fm)
	{
		l__prec = 1;
		sz --;
		
		/* Reached the end of the output buffer? */
		if (sz == 0) 
		{
			*bf = '\0';
			break;
		}
		
		if (*fm != '%') 
		{
			/* No format charracter, just print it... */
			*bf ++ = *fm ++;
			l__ctr ++;
		}
		 else /* Okay. We have a format string */
		{
			/* Analize the next char */
			fm ++;
			
			/* End of string */
			if (*fm == 0) break;	
			
			/* No format - just % */
			if (*fm == '%')		
			{
				*bf ++ = '%';
				l__ctr ++;
				continue;
			}
		
			/* Precision information? */
			if (*fm == '.')
			{
				/* Okay get the precision information from the next char */
				fm ++;
				l__prec = 0;
				
				while(1)
				{
					if (*fm == 0) break;
				
					/* Get it */
					if ((*fm >= '0') && (*fm <= '9'))
					{
						l__prec *= 10;
						l__prec += *fm - '0';
						fm ++;
						
					}
				 	 else break;
				 }
			}
			
			/* Output */
			switch (*fm)
			{
				/* Single charracter */
				case ('c'):
				{
					char l__char = va_arg(ap, int);
					*bf ++ = l__char;
					l__ctr ++;
					break;
				}
				
				/* String */
				case ('s'):
				{
					const char* l__str = va_arg(ap, const char*);
					if (l__str == NULL) l__str = null_text;
					
					/* Copy it */
					while (*l__str != 0)
					{
						*bf ++ = *l__str ++;
						l__ctr ++;
						l__prec --;
					}
					
					/* Fill with some spaces? */
					if (l__prec > 0)
					{
						while (l__prec --)
						{
							*bf ++ = ' ';
							l__ctr ++;
						}
					}
					
					break;
				}
				
				/* Signed integer */
				case ('i'):
				{
					long l__val = va_arg(ap, long);
					int l__tctr = int_to_num(bf, l__prec, INTTONUM_SIGNED, l__val, 10);
					
					bf += l__tctr;
					l__ctr += l__tctr;
					break;
				}
				
				/* Unsigned integer */
				case ('u'):
				{
					unsigned long l__val = va_arg(ap, long);
					int l__tctr = int_to_num(bf, l__prec, 0, (signed)l__val, 10);
					
					bf += l__tctr;
					l__ctr += l__tctr;
					break;
				}	
							
				/* Unsigned integer hex (small letters) */
				case ('x'):
				{
					unsigned long l__val = va_arg(ap, long);
					int l__tctr = int_to_num(bf, l__prec, INTTONUM_SMALL_DIGITS, (signed)l__val, 16);
					
					bf += l__tctr;
					l__ctr += l__tctr;
					break;
				}		
									
				/* Unsigned integer hex (big letters)*/
				case ('X'):
				{
					unsigned long l__val = va_arg(ap, long);
					int l__tctr = int_to_num(bf, l__prec, 0, (signed)l__val, 16);
					
					bf += l__tctr;
					l__ctr += l__tctr;
					break;
				}
														
				/* Unsigned integer octal */
				case ('o'):
				{
					unsigned long l__val = va_arg(ap, long);
					int l__tctr = int_to_num(bf, l__prec, 0, (signed)l__val, 8);
					
					bf += l__tctr;
					l__ctr += l__tctr;
					break;
				}														
			}
			
			fm ++;
		}
	}
	
	*bf = 0;
	l__ctr ++;	
	
	return l__ctr;
}

/*
 * snprintf(str, sz, fm, ...)
 *
 * Simplified implementation of "snprintf"
 *
 */
int snprintf(char* str, size_t sz, const char* fm, ...)
{
	va_list args;

	va_start(args, fm);
	int l__i = vsnprintf(str, sz, fm, args);
    	va_end(args);
    	
    	return l__i;
}

/*
 * dbglib_atoul(str, buf, base)
 *
 * Converts the content of "str" to an unsigend integer according
 * to the base "base". If it was converted sucessfully the converted
 * value will be stored to "buf".
 *
 * Valid bases are: 2, 8, 10, 16
 *
 * Return value:
 *	number -> *buf
 *	0	If successful
 *	1	If not a valid number
 * 
 */
int dbglib_atoul(utf8_t *str, uint32_t *buf, int base)
{
	uint32_t l__retval = 0;
	
	/* Valid buffer and base? */
	if (   (buf == NULL) 
	     && ((base == 2) || (base == 8) || (base == 10) || (base == 16))
	   )
		return 1;
	
	/* Calculate */
	while (*str != '\0')
	{
		utf8_t l__chr = *str;
		/* Small letters to big letters, please */
		if ((l__chr >= 'a') && (l__chr <= 'f'))
			l__chr -= 'a' - 'A';
		
		/* Filter invalid chars... */
		if (	(l__chr < '0')
		     || ((l__chr > '9') && (base <= 10))
		     || ((l__chr > '9') && (l__chr > 'F'))
		     || ((l__chr > '9') && (l__chr < 'A'))
		   )
		{
			return 1;
		}
		
		/* Calculate number */
		if (l__chr <= '9') l__chr -= '0';
		if (l__chr >= 'A') l__chr = (l__chr - 'A') + 10;
		
		l__retval *= base;
		l__retval += l__chr;
		
		str ++;
	}	
	
	*buf = l__retval;
	
	return 0;
}

/*
 * dbglib_atosl(str, buf, base)
 *
 * Converts the content of "str" to an sigend integer according
 * to the base "base". If it was converted sucessfully the converted
 * value will be stored to "buf".
 *
 * Valid bases are: 2, 8, 10, 16
 *
 * Return value:
 *	number -> *buf
 *	0	If successful
 *	1	If not a valid number
 * 
 */
int dbglib_atosl(utf8_t *str, int32_t *buf, int base)
{
	int32_t l__retval = 0;
	int l__sign = 1;
	
	/* Valid buffer and base? */
	if (   (buf == NULL) 
	     && ((base == 2) || (base == 8) || (base == 10) || (base == 16))
	   )
		return 1;
	
	/* Sign at the beginning? */
	if (*str == '-')
	{
		l__sign *= -1;
		str ++;
	}
		
	/* Calculate */
	while (*str != '\0')
	{
		utf8_t l__chr = *str;
		/* Small letters to big letters, please */
		if ((l__chr >= 'a') && (l__chr <= 'f'))
			l__chr -= 'a' - 'A';
		
		/* Filter invalid chars... */
		if (	(l__chr < '0')
		     || ((l__chr > '9') && (base <= 10))
		     || ((l__chr > '9') && (l__chr > 'F'))
		     || ((l__chr > '9') && (l__chr < 'A'))
		   )
		{
			return 1;
		}
		
		/* Calculate number */
		if (l__chr <= '9') l__chr -= '0';
		if (l__chr >= 'A') l__chr = (l__chr - 'A') + 10;
		
		l__retval *= base;
		l__retval += l__chr;
		
		str ++;
	}	
	
	*buf = (l__retval * l__sign);
	
	return 0;
}
