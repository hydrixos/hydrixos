/*
 *
 * console.c
 *
 * (C)2006 by Friedrich Gr�ter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying').   
 *
 * Debugger console driver (EGA / AT-Keyboard)
 *
 */
#include <hydrixos/types.h>
#include <hydrixos/tls.h>
#include <hydrixos/errno.h>
#include <hydrixos/blthr.h>
#include <hydrixos/mem.h>
#include <hydrixos/stdfun.h>
#include <hydrixos/system.h>

#include <hymk/x86-io.h>

#include "../hyinit.h"
#include "coredbg.h"

mtx_t dbg_display_mutex = MTX_DEFINE();
thread_t* dbg_keyboard_thread = 0;
dbg_terminals_t *dbg_terminals = NULL;
dbg_terminals_t *dbg_current_term = NULL;

int dbg_parallel_request = 0;

#define CONTROL_LEFT_SHIFT		1
#define CONTROL_RIGHT_SHIFT		2
#define CONTROL_LEFT_ALT		4
#define CONTROL_LEFT_CTRL		8
#define CONTROL_RIGHT_ALT		16
#define CONTROL_RIGHT_CTRL		32
#define CONTROL_SPECIAL			64

#define LIGHTS_SCROLL 		1
#define LIGHTS_NUM 		2
#define LIGHTS_CAPS 		4

void dbg_keyb_driver(thread_t *thr);
int dbg_keyb_control_keys = 0;

/*
 * dbg_setup_cursor()
 *
 * Changes the screen cursor to the terminal
 * cursor.
 *
 */
static void dbg_setup_cursor(void)
{
	unsigned long	l__loc_pos;
	
	l__loc_pos = dbg_current_term->line * 80 + dbg_current_term->column;
	outb(0x3D4, 0x0F);
	outb(0x3D5, (unsigned char) l__loc_pos);
	outb(0x3D4, 0x0E);
	l__loc_pos >>= 8;
	outb(0x3D5, (unsigned char) l__loc_pos);
	
	/* Display the head line, if wanted */
	if (dbg_current_term->headline != NULL)
	{
		size_t l__len = str_len(dbg_current_term->headline, 25);
		int l__pos = 79;
		
		while (l__len --)
		{
			i__screen[l__pos * 2] = dbg_current_term->headline[l__len];
			i__screen[(l__pos * 2) + 1] = 0x4F;
		
			l__pos --;
		}
	}
	
}

/*
 * dbg_init_driver
 *
 * Initializes the display driver.
 *
 */
int dbg_init_driver(void)
{
	/* Initialize the first terminal */
	dbg_terminals = mem_alloc(sizeof(dbg_terminals_t));
	
	dbg_terminals->keynum = 1;						/* F1-Terminal */
	dbg_terminals->buffer = mem_alloc(25 * 80 * 2);
	dbg_terminals->keybuf = 0;
	
	dbg_terminals->headline = mem_alloc(str_len("System Terminal", 25) + 1);
	str_copy(dbg_terminals->headline, "System Terminal", 25);
	
	dbg_terminals->is_current = TRUE;
	dbg_terminals->is_reading = MTX_NEW();
	dbg_terminals->reader = 0;
		
	dbg_terminals->ls.n = NULL;
	dbg_terminals->ls.p = NULL;
	
	dbg_terminals->column = 0;
	dbg_terminals->line = 0;
	
	dbg_terminals->flags = 0;
	dbg_terminals->prompt_col = 0;
	dbg_terminals->prompt_line = 0;
	
	dbg_terminals->output_color = DBGCOL_GREY;
	
	dbg_current_term = dbg_terminals;
	dbg_setup_cursor();
	
	/* Clear the screen */
	uint16_t *l__scr = (void*)i__screen;
	int l__i = 80*25;
	
	while (l__i --) l__scr[l__i] = 0x0700;
	
	/* Init the Keyb-IRQ-Handler */
	dbg_keyboard_thread = blthr_create(dbg_keyb_driver, 4096);
	hymk_set_priority(dbg_keyboard_thread->thread_sid, 40, 0);
	blthr_awake(dbg_keyboard_thread);
	
	/* Update the display */
	dbg_setup_cursor();
	
	return 0;
}

/*
 * dbg_internal_putc(c, t, b)
 *
 * Internal implementation of putc for different kinds of terminals.
 * Puts 'c' to the terminal 't' by using the buffer 'b'.
 *
 */
static void dbg_internal_putc(char c, dbg_terminals_t *term, char *buf)
{
	if (c == '\n')			/* CR */
	{
		term->column = 0;
		term->line ++;
	}
	 else if (c == '\b')		/* BACKSPACE */
	{
		if (term->flags & DBGTERM_PROMPT_ON)
		{
			if (term->column > 0)
			{		
				if (   (term->line > term->prompt_line)
				    || (term->column > term->prompt_col)
				   )
				{
					term->column --;
				}
			}
			 else
			{
				if (    (term->line > 0)
				     && (term->line > term->prompt_line)
				   )
				{
					term->line --;
					term->column = 79;
				}
			}		
		}
		 else
		{
			if (term->column > 0)
			{		
				term->column --;
			}
			 else
			{
				if (term->line > 0)
				{
					term->line --;
					term->column = 79;
				}
			}
		}
		
		buf[(term->column * 2) + (term->line * 160)] = ' ';
		buf[(term->column * 2) + (term->line * 160) + 1] = term->output_color;
	}
	 else if (c == '\t')		/* Tabulator */
	{
		term->column += 4;
		term->column -= (term->column % 4);
		
		if (term->column > 80)
		{
			term->line ++;
			term->column = 0;
		}
	}
	 else				/* NORMAL CHAR */
	{
		buf[(term->column * 2) + (term->line * 160)] = c;
		buf[(term->column * 2) + (term->line * 160) + 1] = term->output_color;
	
		term->column ++;
		
		if (term->column == 80) 
		{
			term->line ++;
			term->column = 0;
		}
	}
		
	/* New screen line? */
	if (term->line == 25)
	{
		term->line --;
		buf_copy((void*)buf, (void*)&buf[80 * 2], 80*24*2);
		uint16_t *l__scr = (void*)&buf[80*24*2];
		int l__i = 80;
		
		while (l__i --) l__scr[l__i] = (term->output_color << 8) | 0x00;	
				
		/* Scroll prompt line */
		if (term->flags & DBGTERM_PROMPT_ON) 
		{
			if (term->prompt_line > 0) 
				term->prompt_line --;
		}
	}		
}

/*
 * dbg_con_putc(c)
 *
 * Puts a charracter on the screen.
 *
 */
static void dbg_con_putc(char c)
{
	dbg_internal_putc(c, dbg_current_term, (char*)i__screen);
	dbg_setup_cursor();
}

/*
 * dbg_keyb_test_control_keys(c)
 *
 * Test if a control key was pressed or released 
 * and changes the control key status flags.
 *
 */
static int dbg_keyb_test_control_keys(unsigned c)
{
	switch(c)
	{
		case (0xE0):	{
					dbg_keyb_control_keys |= CONTROL_SPECIAL;
					return 0;
				}
				
		case (0x1D):	{
					if (dbg_keyb_control_keys & CONTROL_SPECIAL)
					{
						dbg_keyb_control_keys |= CONTROL_RIGHT_CTRL;
						dbg_keyb_control_keys &= (~CONTROL_SPECIAL);
					}
					 else
					{
						dbg_keyb_control_keys |= CONTROL_LEFT_CTRL;
					}
					return 0;
				}
		case (0x2A):	{
					dbg_keyb_control_keys |= CONTROL_LEFT_SHIFT;
					return 0;
				}
		case (0x36) :	{
					dbg_keyb_control_keys |= CONTROL_RIGHT_SHIFT;
					return 0;
				}
		case (0x38) :   {
					if (dbg_keyb_control_keys & CONTROL_SPECIAL)
					{
						dbg_keyb_control_keys |= CONTROL_RIGHT_ALT;
						dbg_keyb_control_keys &= (~CONTROL_SPECIAL);
					}
					 else
					{
						dbg_keyb_control_keys |= CONTROL_LEFT_ALT;
					}
					return 0;
				}

		case (0x9D):	{
					if (dbg_keyb_control_keys & CONTROL_SPECIAL)
					{
						dbg_keyb_control_keys &= ~CONTROL_RIGHT_CTRL;
						dbg_keyb_control_keys &= (~CONTROL_SPECIAL);
					}
					 else
					{
						dbg_keyb_control_keys &= ~CONTROL_LEFT_CTRL;
					}					
					
					return 0;
				}
		case (0xAA) : 	{
					dbg_keyb_control_keys &= ~CONTROL_LEFT_SHIFT;
					return 0;
				}
		case (0xB6) :	{
					dbg_keyb_control_keys &= ~CONTROL_RIGHT_SHIFT;
					return 0;
				}
		case (0xB8) :   {
					if (dbg_keyb_control_keys & CONTROL_SPECIAL)
					{
						dbg_keyb_control_keys &= ~CONTROL_RIGHT_ALT;
						dbg_keyb_control_keys &= (~CONTROL_SPECIAL);
					}
					 else
					{
						dbg_keyb_control_keys &= ~CONTROL_LEFT_ALT;
					}
					
					return 0;
				}
	}
	
	return 1;
}

/*
 * dbg_keyb_translate_key(c)
 *
 * Translates a key code into a ASCII character.
 *
 * Currently used layout: GERMAN KEYBOARD
 *
 */
/*
 * Key-to-console translator
 *
 */ 
static char dbg_keyb_translate_key(unsigned c)
{
	char l__c = 0;
	
	if (    
		((dbg_keyb_control_keys & (CONTROL_LEFT_ALT)) && (dbg_keyb_control_keys & (CONTROL_LEFT_CTRL | CONTROL_RIGHT_CTRL)))
	     || (dbg_keyb_control_keys & (CONTROL_RIGHT_ALT) )
	   )
	{
		switch (c)
		{
			case (0x10) : l__c = '@'; break;
			case (0x1B) : l__c = '~'; break;
			case (0x0C) : l__c = '\\'; break;
			case (0x08) : l__c = '{'; break;
			case (0x09) : l__c = '['; break;
			case (0x0A) : l__c = ']'; break;
			case (0x0B) : l__c = '}'; break;
			case (0x56) : l__c = '|'; break;
			case (0x12) : l__c = 0xEE; break; /* � */
			case (0x32) : l__c = 0xE6; break; /* � */
			
		}
	}
	 else if (dbg_keyb_control_keys & (CONTROL_LEFT_SHIFT | CONTROL_RIGHT_SHIFT))
	{
		switch (c)
		{
			case (0xE)  : l__c = '\b'; break;
			case (0x1E) : l__c = 'A'; break;
			case (0x30) : l__c = 'B'; break;
			case (0x2E) : l__c = 'C'; break;
			case (0x20) : l__c = 'D'; break;
			case (0x12) : l__c = 'E'; break;
			case (0x21) : l__c = 'F'; break;
			case (0x22) : l__c = 'G'; break;
			case (0x23) : l__c = 'H'; break;
			case (0x17) : l__c = 'I'; break;
			case (0x24) : l__c = 'J'; break;
			case (0x25) : l__c = 'K'; break;
			case (0x26) : l__c = 'L'; break;
			case (0x32) : l__c = 'M'; break;
			case (0x31) : l__c = 'N'; break;
			case (0x18) : l__c = 'O'; break;
			case (0x19) : l__c = 'P'; break;
			case (0x10) : l__c = 'Q'; break;
			case (0x13) : l__c = 'R'; break;
			case (0x1C) : l__c = '\n'; break;
			case (0x1F) : l__c = 'S'; break;
			case (0x14) : l__c = 'T'; break;
			case (0x16) : l__c = 'U'; break;
			case (0x2F) : l__c = 'V'; break;
			case (0x11) : l__c = 'W'; break;
			case (0x2D) : l__c = 'X'; break;
			case (0x2C) : l__c = 'Y'; break;
			case (0x15) : l__c = 'Z'; break;
			case (0x02) : l__c = '!'; break;
			case (0x03) : l__c = '"'; break;
			case (0x04) : l__c = 15; break;
			case (0x05) : l__c = '$'; break;
			case (0x06) : l__c = '%'; break;
			case (0x07) : l__c = '&'; break;
			case (0x08) : l__c = '/'; break;
			case (0x09) : l__c = '('; break;
			case (0x0A) : l__c = ')'; break;
			case (0x0B) : l__c = '='; break;
			case (0x0C) : l__c = '?'; break;
			case (0x0D) : l__c = '`'; break;
			case (0x1A) : l__c = 154; break; /* � */
			case (0x1B) : l__c = '*'; break;
			case (0x27) : l__c = 153; break; /* � */
			case (0x28) : l__c = 142; break; /* � */
			case (0x29) : l__c = ' '; break;
			case (0x2B) : l__c = '\''; break;
			case (0x33) : l__c = ';'; break;
			case (0x34) : l__c = ':'; break;
			case (0x35) : l__c = '_'; break;
			case (0x39) : l__c = ' '; break;
			case (0x56) : l__c = '>'; break;
			
			default:      l__c = 0; break;			
		}
	}
	 else
	{
		switch (c)
		{
			case (0xE)  : l__c = '\b'; break;
			case (0x1E) : l__c = 'a'; break;
			case (0x30) : l__c = 'b'; break;
			case (0x2E) : l__c = 'c'; break;
			case (0x20) : l__c = 'd'; break;
			case (0x12) : l__c = 'e'; break;
			case (0x21) : l__c = 'f'; break;
			case (0x22) : l__c = 'g'; break;
			case (0x23) : l__c = 'h'; break;
			case (0x17) : l__c = 'i'; break;
			case (0x24) : l__c = 'j'; break;
			case (0x25) : l__c = 'k'; break;
			case (0x26) : l__c = 'l'; break;
			case (0x32) : l__c = 'm'; break;
			case (0x31) : l__c = 'n'; break;
			case (0x18) : l__c = 'o'; break;
			case (0x19) : l__c = 'p'; break;
			case (0x10) : l__c = 'q'; break;
			case (0x13) : l__c = 'r'; break;
			case (0x1F) : l__c = 's'; break;
			case (0x14) : l__c = 't'; break;
			case (0x16) : l__c = 'u'; break;
			case (0x2F) : l__c = 'v'; break;
			case (0x11) : l__c = 'w'; break;
			case (0x1C) : l__c = '\n'; break;
			case (0x2D) : l__c = 'x'; break;
			case (0x2C) : l__c = 'y'; break;
			case (0x15) : l__c = 'z'; break;
			case (0x02) : l__c = '1'; break;
			case (0x03) : l__c = '2'; break;
			case (0x04) : l__c = '3'; break;
			case (0x05) : l__c = '4'; break;
			case (0x06) : l__c = '5'; break;
			case (0x07) : l__c = '6'; break;
			case (0x08) : l__c = '7'; break;
			case (0x09) : l__c = '8'; break;
			case (0x0A) : l__c = '9'; break;
			case (0x0B) : l__c = '0'; break;
			case (0x0C) : l__c = 225; break; /* � */
			case (0x0D) : l__c = '\''; break;
			case (0x1A) : l__c = 129; break; /* � */
			case (0x1B) : l__c = '+'; break;
			case (0x27) : l__c = 148; break; /* � */
			case (0x28) : l__c = 132; break; /* � */
			case (0x29) : l__c = ' '; break;
			case (0x2B) : l__c = '#'; break;
			case (0x33) : l__c = ','; break;
			case (0x34) : l__c = '.'; break;
			case (0x35) : l__c = '-'; break;
			case (0x39) : l__c = ' '; break;
			case (0x56) : l__c = '<'; break;
			
			/* F-Keys for terminal switching */
			case (0x3B) : {l__c = 0; dbg_switch_window(1); break;}
			case (0x3C) : {l__c = 0; dbg_switch_window(2); break;}
			case (0x3D) : {l__c = 0; dbg_switch_window(3); break;}
			case (0x3E) : {l__c = 0; dbg_switch_window(4); break;}
			case (0x3F) : {l__c = 0; dbg_switch_window(5); break;}
			case (0x40) : {l__c = 0; dbg_switch_window(6); break;}
			case (0x41) : {l__c = 0; dbg_switch_window(7); break;}
			case (0x42) : {l__c = 0; dbg_switch_window(8); break;}
			case (0x43) : {l__c = 0; dbg_switch_window(9); break;}
			case (0x44) : {l__c = 0; dbg_switch_window(10); break;}
			case (0x57) : {l__c = 0; dbg_switch_window(11); break;}
			case (0x58) : {l__c = 0; dbg_switch_window(12); break;}			
			
			default:      l__c = 0; break;
		}
	}

	/* It was a charracter that can be displayed */
	if (l__c != 0)
	{
		if (    (!(dbg_current_term->flags & DBGTERM_ECHO_OFF)) 
		     || (    (dbg_current_term->flags & DBGTERM_PROMPT_ECHO_ON)
		          && (dbg_current_term->reader != 0)
		        )
		   )
		{ 
			dbg_con_putc(l__c);
		}
	
		/* Is there some thread reading from the current terminal? */
		if (dbg_current_term->reader != 0)
		{
			/* Write the key... */
			dbg_current_term->keybuf = l__c;
		
			/* Awake the reader and wait on it again (10 s) */
			hymk_sync(dbg_current_term->reader, 10000, 1);
			if (*tls_errno == ERR_TIMED_OUT)
			{
				dbg_isprintf("Timeout on keyboard client 0x%X.\n", dbg_current_term->reader);
			}
		}
	}

	return l__c;
}

/*
 * dbg_keyb_wait
 *
 * Wait until the keyboard controller gets ready.
 *
 */
static void dbg_keyb_wait(void)
{
	int l__retr = 10000;
	
	while ((l__retr --) || (inb(0x64) & 0x02))
		; 
}

/*
 * dbg_keyb_driver
 *
 * Handles the keybord IRQs.
 *
 */
void dbg_keyb_driver(thread_t *thr __attribute__ ((unused)))
{
	/* Initialize the keyboard by sending ACK to not-handled keys */
	inb(0x60);
	uint8_t l__v = inb(0x61);
   	outb(0x61, l__v | 0x80);
    	outb(0x61, l__v);
    	dbg_keyb_wait();
	
	/* Handle the keyboard interrupt */
	while(1)
	{
		/* Wait for incomming keyboard IRQs */
		hymk_recv_irq(1);
		if (*tls_errno)
		{
			iprintf("Can't handle keyboard IRQ. Error %i. Trying again...\n", *tls_errno);
		}
		 else
		{
			/* Wait until the keyb-controller gets ready */
			dbg_keyb_wait();
			
			/* Reading from the keyboard device */
			uint8_t	l__test = inb(0x64);
			uint8_t l__char = 0;
						
			/* Able to read? */
			if (l__test & 1)
			{
				l__char = inb(0x60);
				if (dbg_keyb_test_control_keys(l__char)) dbg_keyb_translate_key(l__char);
				
				/* Acknowledge key */
				uint8_t l__z = inb(0x61);
   				outb(0x61, l__z | 0x80);
    				outb(0x61, l__z);
			}
			 else
			{
				continue;
			}	
		}
	}
}

/*
 * dbg_create_window(head)
 *
 * Creates a new console window and returns its ID
 * The terminal window gets the headline "head"
 *
 * Return value:	>0	Terminal ID
 *			 0	Operation failed
 *
 */
int dbg_create_window(const char* head)
{
	int l__termid = 2;
	dbg_terminals_t *l__terms;
	
	/* Find a free terminal number */
	mtx_lock(&dbg_display_mutex, -1);
	
	l__terms = dbg_terminals;
	
	while (l__termid < 13)
	{
		if (l__terms->keynum == l__termid) 
		{
			/* Found the ID, restart searching with another one */
			l__termid ++;
			l__terms = dbg_terminals;
		}
		 else
		{
			l__terms = l__terms->ls.n;
			
			/* There is no terminal with this ID in the list */
			if (l__terms == NULL) break;
		}
	}
	
	/* Did we found a valid number? */
	if (l__termid == 13) {mtx_unlock(&dbg_display_mutex); return 0;}
	
	/* Create a new terminal */
	dbg_terminals_t *l__newterm = mem_alloc(sizeof(dbg_terminals_t));
	if (l__newterm == NULL) {mtx_unlock(&dbg_display_mutex); return 0;}
	
	l__newterm->keynum = l__termid;
	
	l__newterm->buffer = mem_alloc(25 * 80 * 2);
	if (l__newterm->buffer == NULL) 
	{
		mem_free(l__newterm); 
		mtx_unlock(&dbg_display_mutex);
		return 0;
	}
	
	l__newterm->keybuf = 0;
	
	/* Setup the head line, if wanted */
	if (head != NULL)
	{
		l__newterm->headline = mem_alloc(str_len(head, 25) + 1);
		if (l__newterm->headline != NULL) str_copy(l__newterm->headline, head, 25);
	}
	 else
	{
		l__newterm->headline = NULL;
	}
	
	l__newterm->is_current = FALSE;
	l__newterm->is_reading = MTX_NEW();
	l__newterm->reader = 0;
	
	l__newterm->ls.n = NULL;
	l__newterm->ls.p = NULL;
	
	l__newterm->column = 0;
	l__newterm->line = 0;
		
	l__newterm->flags = 0;
	l__newterm->prompt_col = 0;
	l__newterm->prompt_line = 0;
	l__newterm->output_color = DBGCOL_GREY;
	
	/* Fill the display buffer */
	uint16_t *l__scr = (void*)l__newterm->buffer;
	int l__i = 80*25;
	
	while (l__i --) l__scr[l__i] = 0x0700;	
	
	/* Add the new terminal */
	lst_add(dbg_terminals, l__newterm);
	mtx_unlock(&dbg_display_mutex);
	
	return l__termid;
}

/*
 * dbg_switch_window(termid)
 *
 * Selects "termid" to the currently displayed window.
 *
 */
void dbg_switch_window(int termid)
{
	/* Search the terminal descriptor */
	mtx_lock(&dbg_display_mutex, -1);	

	dbg_terminals_t *l__term = dbg_terminals;
	
	while(l__term != NULL)
	{
		if (l__term->keynum == termid) 
			break;
		else
			l__term = l__term->ls.n;
	}
	
	/* Did we found it? */
	if (l__term == NULL) {mtx_unlock(&dbg_display_mutex); return;}
	
	/* Copy current the display content of the current terminal into its display buffer */
	buf_copy(dbg_current_term->buffer, (void*)i__screen, 80 * 25 * 2);
	
	/* Copy the content of the selected terminal to the display buffer */
	buf_copy((void*)i__screen, l__term->buffer, 80 * 25 * 2);
	
	/* Switch the current display structure */
	dbg_current_term->is_current = FALSE;
	l__term->is_current = TRUE;
	
	dbg_current_term = l__term;
	
	/* Update the cursor position */
	dbg_setup_cursor();
	
	mtx_unlock(&dbg_display_mutex);
	
	return;
}

/*
 * dbg_destroy_window(termid)
 *
 * Destroys the terminal window "termid".
 *
 * Note: It's not possible to destroy terminal 1.
 *	 Also it is not possible to destroy a terminal
 *	 which is used by a thread as keyboard input.
 *
 */
void dbg_destroy_window(int termid)
{
	if (termid == 1) return;
	
	/* Search the terminal descriptor */
	mtx_lock(&dbg_display_mutex, -1);
	
	dbg_terminals_t *l__term = dbg_terminals;
	
	while(l__term != NULL)
	{
		if (l__term->keynum == termid) 
			break;
		else
			l__term = l__term->ls.n;
	}
	
	/* Did we found it? */
	if (l__term == NULL) {mtx_unlock(&dbg_display_mutex); return;}
	
	/* Is there a waiting reader? */
	if (l__term->reader != 0) {mtx_unlock(&dbg_display_mutex); return;}
	
	/* Is it the current temrinal? */
	if (l__term->is_current == TRUE) dbg_switch_window(1);
		
	/* Remove it from the list */
	lst_del(l__term);
	
	mtx_unlock(&dbg_display_mutex);
	
	mem_free(l__term->buffer);
	if (l__term->headline != NULL) mem_free(l__term->headline);
	mem_free(l__term);
	
	return;
}

/*
 * dbg_set_headline(termid, head)
 *
 * Changes the head line of the terminal "termid".
 *
 */
void dbg_set_headline(int termid, const utf8_t* head)
{
	/* Search the terminal descriptor */
	mtx_lock(&dbg_display_mutex, -1);
	
	dbg_terminals_t *l__term = dbg_terminals;
	
	while(l__term != NULL)
	{
		if (l__term->keynum == termid) 
			break;
		else
			l__term = l__term->ls.n;
	}
	
	/* Did we found it? */
	if (l__term == NULL) {mtx_unlock(&dbg_display_mutex); return;}
		
	/* Change it */
	if (l__term->headline != NULL) mem_free(l__term->headline);
	
	if (head == NULL) 
	{
		l__term->headline = NULL;
	}
	 else
	{	
		l__term->headline = mem_alloc(str_len(head, 25) + 1);
		str_copy(l__term->headline, head, 25);
	}
		
	/* Refresh display, if we are on the current terminal */
	if (l__term == dbg_current_term) dbg_setup_cursor();
		
	mtx_unlock(&dbg_display_mutex); 
		
	return;
}

/*
 * dbg_utf8_to_extascii
 *
 * Converts a charracter in UTF-8 format to ISO-8859-1.
 * Currently there is only support for the German
 * umlauts.
 *
 * THIS IS AN UGLY SOLUTION. WE NEED SOME MORE UTF-8
 * SUPPORT IN HYDRIXOS!
 *
 */
static inline char dbg_utf8_to_extascii(uint32_t c)
{
	if ((c & 0xFF) < 128) 
	{
		return (char)c;
	}

	if ((c & 0xE0) == 0xC0)
	{	
		
		switch (c & 0xFFFFU)
		{
			case (0x9CC3U): return (char)154U; 	/* UTF-8: � */
			case (0x96C3U): return (char)153U;	/* UTF-8: � */
			case (0x84C3U):	return (char)142U;	/* UTF-8: � */
			case (0xBCC3U):	return (char)129U;	/* UTF-8: � */
			case (0xB6C3U):	return (char)148U;	/* UTF-8: � */
			case (0xA4C3U):	return (char)132U;	/* UTF-8: � */
			case (0x94C3U):	return (char)225U;	/* UTF-8: � */
			case (0xB5C2U): return (char)0xE6U;	/* UTF-8: � */
		}
	
		return '?';
	}
	
	if ((c & 0xF0) == 0xE0)
	{
		switch (c & 0xFFFFFFU)
		{
			case (0xAC82E2U): return (char)0xEEU;	/* UTF-8: � */
		}
		
		return '?';
	}
	
	return '?';
}


/*
 * dbg_extascii_to_utf8
 *
 * Converts a charracter in ISO-8859-1 format to UTF-8.
 * Currently there is only support for the German
 * umlauts.
 *
 * THIS IS AN UGLY SOLUTION. WE NEED SOME MORE UTF-8
 * SUPPORT IN HYDRIXOS!
 *
 */
static inline uint32_t dbg_extascii_to_utf8(char c)
{
	switch ((unsigned char)c)
	{
		case (154):	return 0x9CC3U;  /* UTF-8: � */
		case (153):	return 0x96C3U;	/* UTF-8: � */
		case (142):	return 0x84C3U;	/* UTF-8: � */
		case (129):	return 0xBCC3U;	/* UTF-8: � */
		case (148):	return 0xB6C3U;	/* UTF-8: � */
		case (132):	return 0xA4C3U;	/* UTF-8: � */
		case (225):	return 0x94C3U;	/* UTF-8: � */
		case (0xE6):	return 0xB5C2U; /* UTF-8: � */
		case (0xEE):	return 0xAC82E2U; /* UTF-8: � */
	}
	
	return (uint32_t)c;
}

/*
 * dbg_putc(t, c)
 *
 * Puts a charracter on a termial
 *
 */
void dbg_putc(int termid, uint32_t c)
{
	/* Search the terminal descriptor */
	mtx_lock(&dbg_display_mutex, -1);
	
	dbg_terminals_t *l__term = dbg_terminals;
	
	while(l__term != NULL)
	{
		if (l__term->keynum == termid) 
			break;
		else
			l__term = l__term->ls.n;
	}
	
	/* Did we found it? */
	if (l__term == NULL) {mtx_unlock(&dbg_display_mutex); return;}
	
	/* Get its ASCII conversion */
	char l__c = dbg_utf8_to_extascii(c);
	
	/* Is it the current terminal? */
	if (l__term == dbg_current_term) 
	{
		dbg_con_putc(l__c);
	}
	 else
	{
		dbg_internal_putc(l__c, l__term, l__term->buffer);
	}
	
	mtx_unlock(&dbg_display_mutex); 
	
	return;
}

/*
 * dbg_puts(t, s)
 *
 * Puts a string on a termial. This function
 * don't prints the whole string at once, so
 * it is possible to switch the terminals during
 * the printout of the string.
 *
 * The printout string may have a length of 1000
 * charracters.
 *
 */
void dbg_puts(int termid, const utf8_t* str)
{
	size_t l__len = str_len(str, 1000);
	size_t l__i;
	
	for (l__i = 0; l__i < l__len; l__i ++) 
	{
		if ((str[l__i] & 0xE0) == 0xC0)
		{
			/* UTF-8 charracter? (2 octets) */
			if ((l__i + 1) >= l__len)
			{
				/* Invalid UTF-8 charracter */
				dbg_putc(termid, (uint32_t)'?');
				break;
			}
			 else
			{
				dbg_putc(termid, ((unsigned char)str[l__i + 1] << 8) | ((unsigned char)str[l__i]));
				l__i ++;
			}
		}
		 else if ((str[l__i] & 0xF0) == 0xE0) 
		{
			/* UTF-8 charracter? (3 octets) */
			if ((l__i + 2) >= l__len)
			{
				/* Invalid UTF-8 charracter */
				dbg_putc(termid, (uint32_t)'?');
			 	break;
			}
				 else				
			{
				dbg_putc(termid, ((unsigned char)str[l__i + 2] << 16) | ((unsigned char)str[l__i + 1] << 8) | ((unsigned char)str[l__i]));
				l__i += 2;
			}
		} 
		 else
		{
			/* No. Just ASCII / UTF-8 one octet*/
			dbg_putc(termid, (uint32_t)str[l__i]);
		}
	}

	return;
}

/*
 * dbg_getc(t)
 *
 * Reads a charracter from the keyboard input of terminal t.
 * 
 * Return values:
 *	>0		The charracter read from the terminal.
 *	 0		Error
 *
 */
uint32_t dbg_getc(int termid)
{
	uint32_t l__retval = 0;
	
	/* Search the terminal descriptor */
	mtx_lock(&dbg_display_mutex, -1);
	
	dbg_terminals_t *l__term = dbg_terminals;
	
	while(l__term != NULL)
	{
		if (l__term->keynum == termid) 
			break;
		else
			l__term = l__term->ls.n;
	}
	
	/* Did we found it? */
	if (l__term == NULL) {mtx_unlock(&dbg_display_mutex); return 0;}
		
	/* Try to read */
	mtx_lock(&l__term->is_reading, -1);
	
	l__term->reader = hysys_info_read(MAININFO_CURRENT_THREAD);
	
	/* Okay, we got to be the reader, so we don't need the display mutex any more */
	mtx_unlock(&dbg_display_mutex);
	
	/* Synchronize to the keyboard thread */
	hymk_sync(dbg_keyboard_thread->thread_sid, 0xFFFFFFFF, 0);
	
	l__retval = dbg_extascii_to_utf8(l__term->keybuf);
	
	/* Reset the reader-status */
	l__term->reader = 0;
	mtx_unlock(&l__term->is_reading);
	
	/* Signalize the keyboard thread, that we got the wanted informations */
	hymk_sync(dbg_keyboard_thread->thread_sid, 0xFFFFFFFF, 0);
	
	return l__retval;
}

/*
 * dbg_gets(t, n, b)
 *
 * Reads a string from the terminal t into the buffer b until
 * CR was pressed or the buffer is full. The function will
 * add a zero-termination to the buffer string at its end.
 * The terminating CR will not be stored to the buffer.
 * 
 * Return values:
 *	>0		The number of charracter read from the terminal.
 *			(Including the terminating \0)
 *
 *	=0		Error
 *
 */
size_t dbg_gets(int termid, size_t num, utf8_t* buf)
{
	char l__retval = 0;
	size_t l__num;
	
	/* Search the terminal descriptor */
	mtx_lock(&dbg_display_mutex, -1);
	
	dbg_terminals_t *l__term = dbg_terminals;
	
	while(l__term != NULL)
	{
		if (l__term->keynum == termid) 
			break;
		else
			l__term = l__term->ls.n;
	}
	
	/* Did we found it? */
	if (l__term == NULL) {mtx_unlock(&dbg_display_mutex); return 0;}
			
	/* Try to read */
	mtx_lock(&l__term->is_reading, -1);
	
	l__term->reader = hysys_info_read(MAININFO_CURRENT_THREAD);
	
	/* Setup the prompt */
	if (l__term->flags & DBGTERM_PROMPT_ON)
	{
		l__term->prompt_line = l__term->line;
		l__term->prompt_col = l__term->column;
	}
	
	/* Okay, we got to be the reader, so we don't need the display mutex any more */
	mtx_unlock(&dbg_display_mutex);	
	
	/* Read until the end of the buffer */
	for (l__num = 0; l__num < num; l__num ++)
	{
		/* Synchronize to the keyboard thread */
		hymk_sync(dbg_keyboard_thread->thread_sid, 0xFFFFFFFF, 0);
	
		l__retval = l__term->keybuf;
		
		/* Repeat, if backspace */
		if (l__retval == '\b')
		{
			if (l__num > 0)
			{
				l__num --;
				
				/* Remove more bytes if we have a multibyte UTF-8 charracter */
				if ((buf[l__num] & 0xC0) == 0x80)
				{
					while ((l__num > 0) && ((buf[l__num] & 0xC0) == 0x80))
					{
						buf[l__num] = 0;
						l__num --;
					}
				}
				 else
				{
					buf[l__num] = 0;
				}
			}				
			
			l__num --;
			
			hymk_sync(dbg_keyboard_thread->thread_sid, 0xFFFFFFFF, 0);
			continue;
		}
		
		/*
		 * End of the buffer or CR? 
		 * Then exit, otherwise save the char to the output buffer and 
		 * restart the keyboard thread
		 *
		 */
		if ((l__retval == '\n') || (l__num == num - 1)) 
		{
			buf[l__num] = 0;
			break;
		}
		else
		{
			/* Is it a non-ASCII charracter? */
			if (l__retval < 0)
			{
				uint32_t l__tmpc = dbg_extascii_to_utf8(l__retval);
				
				if ((l__tmpc & 0xE0) == 0xC0)
				{
					/* UTF-8 two octets */
					buf[l__num] = l__tmpc & 0xFF;
					buf[l__num + 1] = (l__tmpc >> 8);
					
					l__num ++;
				}
				 else if ((l__tmpc & 0xF0) == 0xE0)
				{
					/* UTF-8 three octets */
					buf[l__num] = l__tmpc & 0xFF;
					buf[l__num + 1] = (l__tmpc >> 8) & 0xFF;
					buf[l__num + 2] = l__tmpc >> 16;
					
					l__num += 2;
				}
				 else
				{
					/* UTF-8 / ASCII: one octet */
					buf[l__num] = l__tmpc;
				}
			}
			 else
			{
				buf[l__num] = l__retval;
			}
						
			hymk_sync(dbg_keyboard_thread->thread_sid, 0xFFFFFFFF, 0);
		}
	}
	
	/* Reset the reader-status */
	l__term->reader = 0;
	
	/* Deactivate the prompt */
	l__term->prompt_line = 0;
	l__term->prompt_col = 0;
	
	mtx_unlock(&l__term->is_reading);
	
	/* Signalize the keyboard thread, that we got the wanted informations */
	hymk_sync(dbg_keyboard_thread->thread_sid, 0xFFFFFFFF, 0);
	
	return l__num + 1;
}

/*
 * dbg_get_termflags(termid)
 *
 * Returns the control flags of the terminal "termid"
 *
 */
unsigned dbg_get_termflags(int termid)
{
	unsigned l__flags;
	
	/* Search the terminal descriptor */
	mtx_lock(&dbg_display_mutex, -1);
	
	dbg_terminals_t *l__term = dbg_terminals;
	
	while(l__term != NULL)
	{
		if (l__term->keynum == termid) 
			break;
		else
			l__term = l__term->ls.n;
	}
	
	/* Did we found it? */
	if (l__term == NULL) {mtx_unlock(&dbg_display_mutex); return 0;}
			
	l__flags = l__term->flags;
			
	mtx_unlock(&dbg_display_mutex);
	
	return l__flags;
}

/*
 * dbg_set_termflags(termid, flags)
 *
 * Changes the control flags of the terminal "termid" to "flags"
 *
 */
void dbg_set_termflags(int termid, unsigned flags)
{
	/* Search the terminal descriptor */
	mtx_lock(&dbg_display_mutex, -1);
	
	dbg_terminals_t *l__term = dbg_terminals;
	
	while(l__term != NULL)
	{
		if (l__term->keynum == termid) 
			break;
		else
			l__term = l__term->ls.n;
	}
	
	/* Did we found it? */
	if (l__term == NULL) {mtx_unlock(&dbg_display_mutex); return;}
			
	l__term->flags = flags;
			
	mtx_unlock(&dbg_display_mutex);
	
	return;
}

/*
 * dbg_set_termcolor(termid, color)
 *
 * Changes the output color of a terminal "termid" to "color".
 *
 */
void dbg_set_termcolor(int termid, unsigned color)
{
	/* Search the terminal descriptor */
	mtx_lock(&dbg_display_mutex, -1);
	
	dbg_terminals_t *l__term = dbg_terminals;
	
	while(l__term != NULL)
	{
		if (l__term->keynum == termid) 
			break;
		else
			l__term = l__term->ls.n;
	}
	
	/* Did we found it? */
	if (l__term == NULL) {mtx_unlock(&dbg_display_mutex); return;}
			
	l__term->output_color = (uint8_t)color;
			
	mtx_unlock(&dbg_display_mutex);
	
	return;
}

/*
 * dbg_iprintf(termid, fmt, ...)
 *
 * Printf on a terminal (internally used by the debugger).
 *
 */
int dbg_iprintf(int termid, const utf8_t *fm, ...)
{
	utf8_t l__buf[1000];
	int l__i;
	va_list l__args;

	va_start(l__args, fm);
	l__i = vsnprintf(l__buf, 1000, fm, l__args);
    	va_end(l__args);

	if (l__i > 0) dbg_puts(termid, l__buf);
	
	return l__i;
}

/*
 * dbg_isprintf(fmt, ...)
 *
 * Printf on the system terminal (internally used by the debugger).
 *
 */
int dbg_isprintf(const utf8_t *fm, ...)
{
	utf8_t l__buf[1000];
	int l__i;
	va_list l__args;

	va_start(l__args, fm);
	l__i = vsnprintf(l__buf, 1000, fm, l__args);
    	va_end(l__args);

	if (l__i > 0) dbg_puts(1, l__buf);
	
	return l__i;
}

/*
 * dbg_pause(term)
 *
 * "Press any key to continue" on terminal "term".
 *
 */
void dbg_pause(int term)
{
		dbg_iprintf(term, "- Press any key to continue -");
		
		/* No Echo on keyboard input */
		unsigned l__oldflags = dbg_get_termflags(term);
		dbg_set_termflags(term, DBGTERM_ECHO_OFF);
		dbg_getc(term); 
				
		dbg_iprintf(term, "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
		
		/* Restore terminal flags */
		dbg_set_termflags(term, l__oldflags);
		
		return;
}

/*
 * dbg_lock_terminal(term, stop)
 *
 * Locks the terminal "term" for an exclusiv read- and write
 * access mode. This function will wait unlimited time or
 * exit, if "stop" is 1.
 *
 * Return value:
 *	0	Lock successful
 *	1	Error
 *
 */
int dbg_lock_terminal(int term, int stop)
{
	while(1)
	{
		/* Search the terminal descriptor */
		mtx_lock(&dbg_display_mutex, -1);
	
		dbg_terminals_t *l__term = dbg_terminals;
	
		while(l__term != NULL)
		{
			if (l__term->keynum == term) 
				break;
			else
				l__term = l__term->ls.n;
		}
	
		/* Did we found it? */
		if (l__term == NULL) {mtx_unlock(&dbg_display_mutex); return 1;}
	
		/* Try to lock the mutex */
		if (mtx_trylock(&l__term->is_writing))
		{
			/* Success. Leave */
			mtx_unlock(&dbg_display_mutex);
			return 0;
		}
		 else
		{
			if (stop)
			{
				/* We won't wait */
				mtx_unlock(&dbg_display_mutex);
				return 1;			
			}
			
			/* Failed. Setup its latency bit and sleep for a while */
			l__term->is_writing.latency = 1;			
			
			mtx_unlock(&dbg_display_mutex);
			blthr_yield(0);			
		}
	}	
}

/*
 * dbg_unlock_terminal
 *
 * Unlocks the terminal
 *
 */
void dbg_unlock_terminal(int term)
{
	/* Search the terminal descriptor */
	mtx_lock(&dbg_display_mutex, -1);
	
	dbg_terminals_t *l__term = dbg_terminals;
	
	while(l__term != NULL)
	{
		if (l__term->keynum == term) 
			break;
		else
			l__term = l__term->ls.n;
	}
	
	/* Did we found it? */
	if (l__term == NULL) {mtx_unlock(&dbg_display_mutex); return;}
	
	mtx_unlock(&l__term->is_writing);
	
	mtx_unlock(&dbg_display_mutex);
}
