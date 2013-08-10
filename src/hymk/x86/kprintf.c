/*
 *
 * kprintf.c
 *
 * (C)2001, 2002 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying'). 
 *
 * Functions used by kernel internal for simple 
 * console support during the initialization.
 *
 */
#include <hydrixos/types.h>
#include <string.h>
#include <stdio.h>
#include <kcon.h>

static const char *null_text = "( NULL )";

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
					const char* l__str = va_arg(ap, char*);
					if (l__str == NULL) l__str = null_text;
					
					/* Copy it */
					while (*l__str != 0)
					{
						*bf ++ = *l__str ++;
						l__ctr ++;
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
					unsigned long l__val = va_arg(ap, unsigned long);
					int l__tctr = int_to_num(bf, l__prec, 0, (signed long)l__val, 10);
					
					bf += l__tctr;
					l__ctr += l__tctr;
					break;
				}	
							
				/* Unsigned integer hex (small letters) */
				case ('x'):
				{
					unsigned long l__val = va_arg(ap, unsigned long);
					int l__tctr = int_to_num(bf, l__prec, INTTONUM_SMALL_DIGITS, (signed long)l__val, 16);
					
					bf += l__tctr;
					l__ctr += l__tctr;
					break;
				}		
									
				/* Unsigned integer hex (big letters)*/
				case ('X'):
				{
					unsigned long l__val = va_arg(ap, unsigned long);
					int l__tctr = int_to_num(bf, l__prec, 0, (signed long)l__val, 16);
					
					bf += l__tctr;
					l__ctr += l__tctr;
					break;
				}
														
				/* Unsigned integer octal */
				case ('o'):
				{
					unsigned long l__val = va_arg(ap, unsigned long);
					int l__tctr = int_to_num(bf, l__prec, 0, (signed long)l__val, 8);
					
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

volatile unsigned int l__line = 2;
volatile unsigned int l__column = 0;

/*
 * kgetcurpos()
 *
 * Loads the current cursor position to the line pointer
 * And resets the column pointer. This may be usefull, to
 * make user-mode Outputs still readable.
 *
 */
static inline void kgetcursorpos(void)
{
	uint8_t l__low = 0, l__hi = 0;
	long l__cp;
	
	outb(0x3D4, 0xE);
	l__hi = inb(0x3D5);
	
	outb(0x3D4, 0xF);
	l__low = inb(0x3D5);

	l__cp = l__low + (l__hi << 8);
	l__line = l__cp / 80;
	l__column = l__cp % 80;
		
	return;
}	


/*
 *
 * int kprintf(char *fm, ...);
 *
 * Output to console in kernel mode, without abstraction layer.
 * THIS FUNCTION SHOULD ONLY BE USED DURING INITIALIZATION OR
 * WHEN A PANIC APPEARS (or to debug some kernel code), because
 * it is not save to use this function !
 *
 */
int kprintf(const char* fm, ...)
{
	char			buf[1000];
	unsigned long	bufptr;
	int i;
	va_list args;

	kgetcursorpos();

	va_start(args, fm);
	i = vsnprintf(buf, 1000, fm, args);
    	va_end(args);

	for (bufptr=0; buf[bufptr]!=0; bufptr++)
	{
		switch (buf[bufptr])
		{
			case	'\n'	: l__KCON_NEW_LINE; break;
			default		: {
			 			l__KCON_CHAR_OUT(buf[bufptr]); 
						l__KCON_NEXT_CHAR;
						break;
	 				  }
	 	}
	}

	return i;
}


void kclrscr(void)
{
	l__KCON_CLEAR_SCREEN;
}
