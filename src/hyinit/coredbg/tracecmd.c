/*
 *
 * tracecmd.c
 *
 * (C)2006 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying').   
 *
 * Trace command line functions
 *
 */
#include <hydrixos/types.h>
#include <hydrixos/tls.h>
#include <hydrixos/errno.h>
#include <hydrixos/blthr.h>
#include <hydrixos/mem.h>
#include <hydrixos/stdfun.h>
#include <hydrixos/system.h>

#include <coredbg/cdebug.h>

#include "../hyinit.h"
#include "coredbg.h"


/*
 * dbg_get_sidpar(name, type)
 *
 * Returns the value of the SID parameter "-c <sid>" of a tracer command "name".
 * If SID completion should be used, use a SID of the type "type".
 *
 * Return value:
 *	>  -1		SID of the operation
 *	== SID_INVALID	Error
 *
 */
sid_t dbg_get_sidpar(const utf8_t *name, sid_t type)
{
	sid_t l__sid;
	dbg_shell_t *l__shell = *dbg_tls_shellptr;
	
	int l__infopar = dbg_test_par(0, "-c");
	if (l__infopar > -1)
	{
		/* The next parameter should contain our SID */
		if (l__shell->n_pars < (unsigned)l__infopar)
		{
			dbg_iprintf(l__shell->terminal, "Missing SID parameter. Try \"help %s\" for more informations.\n", name);
			return SID_INVALID;
		}
		
		/* Get the SID */
		if (dbglib_atoul(l__shell->pars[l__infopar + 1], &l__sid, 16))
		{
			dbg_iprintf(l__shell->terminal, "Can't convert parameter - \"%s\".\n", l__shell->pars[l__infopar + 1]);
			return SID_INVALID;
		}
	}
	 else
	{
		/* No special SID. Just the current one. */
		l__sid = l__shell->client;
	}
	
	/* SID completion */
	if (l__sid < 0xFFFF) l__sid += (type & SID_TYPE_MASK);
	
	return l__sid;
}

/*
 * dbg_sh_start
 *
 * Starts the current client thread
 *
 * Usage
 *	start
 *
 */
int dbg_sh_start(void)
{
	dbg_shell_t *l__shell = *dbg_tls_shellptr;
	
	dbg_iprintf(l__shell->terminal, "Starting now 0x%X. Leaving interactive mode.\n", l__shell->client);
	
	/* Get the client structure */
	dbg_client_t* l__client = dbg_find_client(l__shell->client);
	if (l__client == NULL) 
	{
		mtx_unlock(&dbg_clients_mtx);
		dbg_isprintf("Can't find client 0x%X.\n", l__shell->client);
		return -1;
	}
		
	mtx_unlock(&dbg_clients_mtx);	
	
	dbg_execute_client(l__client);
	
	dbg_iprintf(l__shell->terminal, "Entering interactive mode for 0x%X.\n", l__shell->client);
	
	return 0;
}

/*
 * dbg_sh_hook
 *
 * Get the control over an uncontrolled thread.
 *
 * Usage
 *	hook <sid> <term>
 *
 *	<sid>		The SID of the thread which should be controled
 *	<term>		Put its shell on terminal <term>
 *
 */
int dbg_sh_hook(void)
{
	dbg_shell_t *l__shell = *dbg_tls_shellptr;
	sid_t l__sid;
	int l__term;
	
	if (l__shell->n_pars < 3)
	{
		dbg_iprintf(l__shell->terminal, "Missing parameter. Try \"help hook\" for more informations.\n");
		return -1;
	}
	
	/* Get the SID */
	if (dbglib_atoul(l__shell->pars[1], &l__sid, 16))
	{
		dbg_iprintf(l__shell->terminal, "Missing SID parameter. Try \"help hook\" for more informations.\n");
		return -1;
	}
	
	/* SID completion */
	if (l__sid < 0xFFFF) l__sid += 0x1000000;
		
	/* Get the terminal number */
	if (dbglib_atosl(l__shell->pars[2], &l__term, 10))
	{
		dbg_iprintf(l__shell->terminal, "Missing terminal parameter. Try \"help hook\" for more informations.\n");
		return -1;
	}	
	
	if ((l__term < 0) || (l__term > 12))
	{
		dbg_iprintf(l__shell->terminal, "Invalid terminal - %i.\n", l__term);
		return -1;
	}
		
	int l__a = 0;
		
	/* Try to hook it */
	if ((l__a = dbg_hook_client(l__sid, l__term)))
	{
		if (l__a > 0)
		{
			dbg_iprintf(l__shell->terminal, "Client 0x%X already exists. Can't hook an existing client.\n", l__sid);
		}
		 else
		{
			dbg_iprintf(l__shell->terminal, "Can't hook client 0x%X, because of %i.\n", l__sid, *tls_errno);
		}
		
		return -1;
	}
	
	return 0;
}

/*
 * dbg_sh_exit
 *
 * Exit the current debugging session
 *
 * Usage
 *	exit [kill]
 *
 *	[kill]		Kill the client thread, if used
 *
 */
int dbg_sh_exit(void)
{
	dbg_shell_t *l__shell = *dbg_tls_shellptr;
	
	/* Kill the client */
	if (dbg_test_par(0, "kill") > -1)
	{
		hymk_destroy_subject(l__shell->client);
	}
	 else
	{
		/* Awake it */
		hymk_awake_subject(l__shell->client);
	}
	
	/* Reset terminal flags */
	dbg_set_termflags(l__shell->terminal, 0);
	
	/* Kill us */
	dbg_destroy_client(l__shell->client);
	
	/* Still alive? Error on system terminal. */
	dbg_isprintf("Can't exit client session 0x%X, because of %i.\n", l__shell->client, *tls_errno);

	return 0;
}

/*
 * dbg_print_eflags(term, flags)
 *
 * Prints the Flags of an EFLAG register "eflags" on terminal "term".
 *
 */
static void dbg_print_eflags(int term, uint32_t eflags)
{
	dbg_iprintf(term, "EFLAGS: ");
	if (eflags & EFLAG_CF)	dbg_iprintf(term, " CF");
	if (eflags & EFLAG_PF)	dbg_iprintf(term, " PF");
	if (eflags & EFLAG_AF)	dbg_iprintf(term, " AF");
	if (eflags & EFLAG_ZF)	dbg_iprintf(term, " ZF");
	if (eflags & EFLAG_SF)	dbg_iprintf(term, " SF");
	if (eflags & EFLAG_TF)	dbg_iprintf(term, " TF");
	if (eflags & EFLAG_IF)	dbg_iprintf(term, " IF");
	if (eflags & EFLAG_DF)	dbg_iprintf(term, " DF");
	if (eflags & EFLAG_OF)	dbg_iprintf(term, " OF");
	if (eflags & EFLAG_NT)	dbg_iprintf(term, " NT");
	if (eflags & EFLAG_RF)	dbg_iprintf(term, " RF");
	if (eflags & EFLAG_VM)	dbg_iprintf(term, " VM");
	if (eflags & EFLAG_AC)	dbg_iprintf(term, " AC");
	if (eflags & EFLAG_VIF)	dbg_iprintf(term, " VIF");
	if (eflags & EFLAG_VIP)	dbg_iprintf(term, " VIP");
	dbg_iprintf(term, "\n");
}

/*
 * dbg_sh_getreg
 *
 * Retrieves the content of one (or more)
 *
 * Usage
 *	dbg_sh_getreg [-c sid] [reg1] [reg2]
 *
 *	-c sid		Use a special sid (if not defined the current client will be used)
 *
 *	If no register is defined the content all registers will be displayed
 *
 */
#define DBG_GETREG_LOAD(___l, ___g) 	if ((dbg_test_par(0, #___l) > -1) || (dbg_test_par(0, #___g) > -1))\
					{\
						dbg_iprintf(l__shell->terminal, "%s = 0x%X\n", #___g, l__regs.___l);\
						l__parctr ++;\
					}\
 
int dbg_sh_getreg(void)
{
	dbg_shell_t *l__shell = *dbg_tls_shellptr;
	int l__parctr = 0;
	sid_t l__sid;
	
	/* Was a SID defined? */
	l__sid = dbg_get_sidpar("getreg", SIDTYPE_THREAD);
	if (l__sid == SID_INVALID) return -1;
	
	/* Retrieve the registers */
	dbg_registers_t l__regs = dbg_get_registers(l__sid);
	if (*tls_errno)
	{
		*tls_errno = 0;
		return -1;
	}
	
	/* Test the parameter list */
	DBG_GETREG_LOAD(eax, EAX);
	DBG_GETREG_LOAD(ebx, EBX);
	DBG_GETREG_LOAD(ecx, ECX);
	DBG_GETREG_LOAD(edx, EDX);
	DBG_GETREG_LOAD(esi, ESI);
	DBG_GETREG_LOAD(edi, EDI);
	DBG_GETREG_LOAD(ebp, EBP);
	DBG_GETREG_LOAD(esp, ESP);
	DBG_GETREG_LOAD(eip, EIP);
		
	if ((dbg_test_par(0, "eflags") > -1) || (dbg_test_par(0, "EFLAGS") > -1))
	{
		dbg_iprintf(l__shell->terminal, "EFLAGS = 0x%X\n", l__regs.eflags);
		dbg_print_eflags(l__shell->terminal, l__regs.eflags);
		l__parctr ++;
	}
	
	/* No register specified? So we print all */
	if (l__parctr == 0)
	{
		dbg_iprintf(l__shell->terminal, "EAX 0x%.8X\tEBX 0x%.8X\tECX 0x%.8X\tEDX 0x%.8X\n", l__regs.eax, l__regs.ebx, l__regs.ecx, l__regs.edx);
		dbg_iprintf(l__shell->terminal, "ESI 0x%.8X\tEDI 0x%.8X\tEBP 0x%.8X\tESP 0x%.8X\n", l__regs.esi, l__regs.edi, l__regs.ebp, l__regs.esp);
		dbg_iprintf(l__shell->terminal, "EIP 0x%.8X\n", l__regs.eip);
		dbg_iprintf(l__shell->terminal, "EFLAGS 0x%.8X\n", l__regs.eflags);
		dbg_print_eflags(l__shell->terminal, l__regs.eflags);
	}
					
	return 0;
}

/*
 * dbg_sh_setreg
 *
 * Changes the content of one or more registers
 *
 * Usage:
 *	setreg <reg1> <val1> ... <regn> <valn> [-c <sid>]
 *
 *	-c sid		Use a special sid (if not defined the current client will be used)
 * 
 */
#define DBG_SETREG_SET(___l, ___g)		if (((l__infopar = dbg_test_par(0, #___l)) > -1) || ((l__infopar = dbg_test_par(0, #___g)) > -1))\
						{\
							if (dbglib_atoul(l__shell->pars[l__infopar + 1], &l__regs.___l, 16))\
							{\
								dbg_iprintf(l__shell->terminal, "Can't convert %s to a register value for %s.\n", l__shell->pars[l__infopar + 1], l__shell->pars[l__infopar]);\
							}\
						\
							dbg_iprintf(l__shell->terminal, "Set %s to 0x%X.\n", #___g, l__regs.___l);\
						}
 
int dbg_sh_setreg(void)
{
	dbg_shell_t *l__shell = *dbg_tls_shellptr;
	int l__infopar = -1;
	sid_t l__sid;
	
	/* Right count of parameters? */
	if (l__shell->n_pars < 2)
	{
		dbg_iprintf(l__shell->terminal, "Missing parameters. Try \"help setreg\" for more informations.\n");
		return -1;
	}	
	
	/* Was a SID defined? */
	l__sid = dbg_get_sidpar("setreg", SIDTYPE_THREAD);
	if (l__sid == SID_INVALID) return -1;
		
	/* Load the content of the registers */
	dbg_registers_t l__regs = dbg_get_registers(l__sid);
	if (*tls_errno)
	{
		*tls_errno = 0;
		return -1;
	}
		
	/* Change the wanted register */
	DBG_SETREG_SET(eax, EAX);
	DBG_SETREG_SET(ebx, EBX);
	DBG_SETREG_SET(ecx, ECX);
	DBG_SETREG_SET(edx, EDX);
	DBG_SETREG_SET(esi, ESI);
	DBG_SETREG_SET(edi, EDI);
	DBG_SETREG_SET(esp, ESP);
	DBG_SETREG_SET(ebp, EBP);
	DBG_SETREG_SET(eip, EIP);
	DBG_SETREG_SET(eflags, EFLAGS);
	
	/* Save the changings */
	dbg_set_registers(l__sid, l__regs);
	if (*tls_errno)
	{
		*tls_errno = 0;
		return -1;
	}
	
	return 0;
}

/*
 * dbg_prepare_sh_read(name, sz, &numb)
 *
 * Prepares the read procedure for the command "name" for reading data in
 * junks of "sz". The number of bytes to read will be stored to the buffer,
 * where "numb" points to.
 *
 * Return value:
 *	== NULL		if failed
 *	!= NULL		pointer to the read datas (number of blocks => numb)
 *
 */
static void* dbg_prepare_sh_read(const utf8_t *name, size_t sz, unsigned *numb)
{
	dbg_shell_t *l__shell = *dbg_tls_shellptr;
	int l__infopar = -1;
	sid_t l__sid;
	unsigned l__num;
	uintptr_t l__adr;
	uint32_t *l__buf = NULL;
	
	/* Right count of parameters? */
	if (l__shell->n_pars < 2)
	{
		dbg_iprintf(l__shell->terminal, "Missing parameters. Try \"help %s\" for more informations.\n", name);
		return NULL;
	}
	
	/* Was a SID defined? */
	l__sid = dbg_get_sidpar(name, SIDTYPE_THREAD);	
	if (l__sid == SID_INVALID) return NULL;
	
	/* SID completion */
	if (l__sid < 0xFFFF) l__sid += 0x1000000;	
	
	/* Was a size defined? */
	l__infopar = dbg_test_par(0, "-n");
	if (l__infopar > -1)
	{
		/* The next parameter should contain our size parameter */
		if (l__shell->n_pars < (unsigned)l__infopar)
		{
			dbg_iprintf(l__shell->terminal, "Missing size parameter. Try \"help %s\" for more informations.\n", name);
			return NULL;
		}
		
		/* Get the number of DWORDs */
		if (dbglib_atoul(l__shell->pars[l__infopar + 1], &l__num, 10))
		{
			dbg_iprintf(l__shell->terminal, "Can't convert parameter - \"%s\".\n", l__shell->pars[l__infopar + 1]);
			return NULL;
		}
	}
	 else
	{
		/* No number. Just one DWORD. */
		l__num = 1;
	}
	
	/* Get the destination address */
	if (dbglib_atoul(l__shell->pars[1], &l__adr, 16))
	{
		dbg_iprintf(l__shell->terminal, "Illegal address - \"%s\".\n", l__shell->pars[1]);
		return NULL;
	}
	
	/* Allocate the memory buffer */
	l__buf = mem_alloc(l__num * sz);
	if (l__buf == NULL)
	{
		dbg_iprintf(l__shell->terminal, "Can't allocate memory for read buffer, because of %i.\n", *tls_errno);
		*tls_errno = 0;
		return NULL;
	}
	
	/* Try to read it */
	if (dbg_read(l__sid, l__adr, l__buf, l__num * sz))
	{
		dbg_iprintf(l__shell->terminal, "Can't read %i bytes from 0x%X at 0x%X (message on system terminal).\n", l__num * sizeof(uint8_t), l__sid, l__adr, *tls_errno);
		*tls_errno = 0;
		return NULL;
	}
	
	dbg_iprintf(l__shell->terminal, "Reading %i bytes from 0x%X at 0x%X:\n", l__num * sz, l__sid, l__adr);

	*numb = l__num;

	return l__buf;
}

/*
 * dbg_sh_readd
 *
 * Read one or more dwords from the clients' memory
 *
 * Usage:
 *	readd <address> [-c <sid>] [-n <num>]
 *	 	-c sid		Use a special sid (if not defined the current client will be used)
 *		-n num		Read "num" dwords, otherwise one.
 */
int dbg_sh_readd(void)
{
	dbg_shell_t *l__shell = *dbg_tls_shellptr;
	unsigned l__num = 0;
	
	/* Read data */
	uint32_t *l__buf = dbg_prepare_sh_read("readd", sizeof(uint32_t), &l__num);
	if (l__buf == NULL) return -1;
	
	/* Write it to the screen */
	for (unsigned l__i = 0; l__i < l__num; l__i ++)
	{
		dbg_iprintf(l__shell->terminal, "0x%.8X  ", l__buf[l__i]);
		if (!((l__i + 1) % 6)) dbg_iprintf(l__shell->terminal, "\n");
	}
		
	dbg_iprintf(l__shell->terminal, "\n\n");
	
	mem_free(l__buf);
	
	return -1;
}

/*
 * dbg_sh_readb
 *
 * Read one or more bytes from the clients' memory
 *
 * Usage:
 *	readd <address> [-c <sid>] [-n <num>]
 *	 	-c sid		Use a special sid (if not defined the current client will be used)
 *		-n num		Read "num" bytes, otherwise one.
 *		-a		Display as ASCII charracters
 *
 */
int dbg_sh_readb(void)
{
	dbg_shell_t *l__shell = *dbg_tls_shellptr;
	unsigned l__num = 0;
	
	/* Read data */
	uint8_t *l__buf = dbg_prepare_sh_read("readb", sizeof(uint8_t), &l__num);
	if (l__buf == NULL) return -1;
	
	if (dbg_test_par(0, "-a") == -1)
	{
		/* Write it to the screen */
		for (unsigned l__i = 0; l__i < l__num; l__i ++)
		{
			dbg_iprintf(l__shell->terminal, "0x%.2X  ", l__buf[l__i]);
			if (!((l__i + 1) % 12)) dbg_iprintf(l__shell->terminal, "\n");
		}
	}
	 else
	{
		dbg_set_termcolor(l__shell->terminal, DBGCOL_LIGHTEN_BLUE);
		dbg_iprintf(l__shell->terminal, "| ");
		dbg_set_termcolor(l__shell->terminal, DBGCOL_GREY);
		/* Write it to the screen */
		for (unsigned l__i = 0; l__i < l__num; l__i ++)
		{
			if (l__buf[l__i] == '\n') 
			{
				dbg_set_termcolor(l__shell->terminal, DBGCOL_LIGHTEN_RED);
				dbg_iprintf(l__shell->terminal, "N");
				dbg_set_termcolor(l__shell->terminal, DBGCOL_GREY);
			}
			 else if (l__buf[l__i] == '\t') 
			{
				dbg_set_termcolor(l__shell->terminal, DBGCOL_LIGHTEN_RED);
				dbg_iprintf(l__shell->terminal, "T");				
				dbg_set_termcolor(l__shell->terminal, DBGCOL_GREY);
			}
			 else if (l__buf[l__i] == '\b') 
			{
				dbg_set_termcolor(l__shell->terminal, DBGCOL_LIGHTEN_RED);
				dbg_iprintf(l__shell->terminal, "B");				
				dbg_set_termcolor(l__shell->terminal, DBGCOL_GREY);
			}	
			 else if (l__buf[l__i] == '\0') 
			{
				dbg_set_termcolor(l__shell->terminal, DBGCOL_LIGHTEN_RED);
				dbg_iprintf(l__shell->terminal, "0");				
				dbg_set_termcolor(l__shell->terminal, DBGCOL_GREY);
			}					
			 else
			{
				dbg_set_termcolor(l__shell->terminal, DBGCOL_WHITE);
				dbg_iprintf(l__shell->terminal, "%c", l__buf[l__i]);
				dbg_set_termcolor(l__shell->terminal, DBGCOL_GREY);
			}
			if (!((l__i + 1) % 76)) 
			{
				dbg_set_termcolor(l__shell->terminal, DBGCOL_LIGHTEN_BLUE);
				dbg_iprintf(l__shell->terminal, "\n| ");
				dbg_set_termcolor(l__shell->terminal, DBGCOL_GREY);
			}
		}	
	}
		
	dbg_iprintf(l__shell->terminal, "\n\n");
	
	mem_free(l__buf);
	
	return -1;
}

/*
 * dbg_sh_writed
 *
 * Write one or more dwords to the clients' memory
 *
 * Usage:
 *	writed <address> [-c <sid>] -d <data1> [<data-2> <data-3> ... <data-n>]
 *	 	-c sid		Use a special sid (if not defined the current client will be used)
 */
int dbg_sh_writed(void)
{
	dbg_shell_t *l__shell = *dbg_tls_shellptr;
	int l__infopar = -1;
	unsigned l__datas = 0;
	sid_t l__sid = 0;
	uintptr_t l__adr = 0;
	uint32_t *l__buf = NULL;
	
	/* Right count of parameters? */
	if (l__shell->n_pars < 2)
	{
		dbg_iprintf(l__shell->terminal, "Missing parameters. Try \"help writed\" for more informations.\n");
		return -1;
	}
	
	/* Get the destination address */
	if (dbglib_atoul(l__shell->pars[1], &l__adr, 16))
	{
		dbg_iprintf(l__shell->terminal, "Illegal address - \"%s\".\n", l__shell->pars[1]);
		return NULL;
	}	
	
	/* Was a SID defined? */
	l__sid = dbg_get_sidpar("writed", SIDTYPE_THREAD);
	if (l__sid == SID_INVALID) return -1;
	
	/* Get the data parameter */
	l__infopar = dbg_test_par(0, "-d");
	if (l__infopar == -1)
	{
		dbg_iprintf(l__shell->terminal, "Missing data parameter. Try \"help writed\" for more informations.\n");
		return -1;
	}

	/* Store the datas to a buffer */
	l__datas = l__shell->n_pars - (l__infopar + 1);
	if (l__datas < 1)
	{
		dbg_iprintf(l__shell->terminal, "Missing datas. Try \"help writed\" for more informations.\n");
		return -1;
	}
	
	l__buf = mem_alloc(l__datas * sizeof(uint32_t));
	if (l__buf == NULL)
	{
		dbg_iprintf(l__shell->terminal, "Can't allocate outbuf buffer, because of %i.\n", *tls_errno);
		return -1;
	}

	for (unsigned l__i = 0; l__i < l__datas; l__i ++)
	{
		if (dbglib_atoul(l__shell->pars[l__infopar + l__i + 1], &l__buf[l__i], 16))
		{
			dbg_iprintf(l__shell->terminal, "Invalid data paremter - %s.\n", l__shell->pars[l__infopar + l__i + 1]);
		}
		
		dbg_iprintf(l__shell->terminal, "buf[%i] = 0x%X\n", l__i, l__buf[l__i]);
	}

	/* Write it to the client memory */
	dbg_iprintf(l__shell->terminal, "Writing %i bytes of datas to client 0x%X at 0x%X...", l__datas * sizeof(uint32_t), l__sid, l__adr);
	
	if (dbg_write(l__sid, l__adr, l__buf, l__datas * sizeof(uint32_t)))
	{
		*tls_errno = 0;
		dbg_iprintf(l__shell->terminal, "( FAILED )\nCan't write datas to client. Please look at the system terminal for further informations.\n");
		return -1;
	}

	dbg_iprintf(l__shell->terminal, "( DONE )\n");
	
	mem_free(l__buf);
	
	return 0;
}

/*
 * dbg_sh_writeb
 *
 * Write one or more bytes to the clients' memory
 *
 * Usage:
 *	writeb <address> [-c <sid>] -d <data1> [<data-2> <data-3> ... <data-n>]
 *	 	-c sid		Use a special sid (if not defined the current client will be used)
 */
int dbg_sh_writeb(void)
{
	dbg_shell_t *l__shell = *dbg_tls_shellptr;
	int l__infopar = -1;
	unsigned l__datas = 0;
	sid_t l__sid = 0;
	uintptr_t l__adr = 0;
	uint8_t *l__buf = NULL;
	
	/* Right count of parameters? */
	if (l__shell->n_pars < 2)
	{
		dbg_iprintf(l__shell->terminal, "Missing parameters. Try \"help writeb\" for more informations.\n");
		return -1;
	}
	
	/* Get the destination address */
	if (dbglib_atoul(l__shell->pars[1], &l__adr, 16))
	{
		dbg_iprintf(l__shell->terminal, "Illegal address - \"%s\".\n", l__shell->pars[1]);
		return NULL;
	}	
	
	/* Was a SID defined? */
	l__sid = dbg_get_sidpar("writeb", SIDTYPE_THREAD);
	if (l__sid == SID_INVALID) return -1;
	
	/* Get the data parameter */
	l__infopar = dbg_test_par(0, "-d");
	if (l__infopar == -1)
	{
		dbg_iprintf(l__shell->terminal, "Missing data parameter. Try \"help writeb\" for more informations.\n");
		return -1;
	}

	/* Store the datas to a buffer */
	l__datas = l__shell->n_pars - (l__infopar + 1);
	if (l__datas < 1)
	{
		dbg_iprintf(l__shell->terminal, "Missing datas. Try \"help writeb\" for more informations.\n");
		return -1;
	}
	
	l__buf = mem_alloc(l__datas * sizeof(uint8_t));
	if (l__buf == NULL)
	{
		dbg_iprintf(l__shell->terminal, "Can't allocate outbuf buffer, because of %i.\n", *tls_errno);
		return -1;
	}

	for (unsigned l__i = 0; l__i < l__datas; l__i ++)
	{
		uint32_t l__din;
		
		if (dbglib_atoul(l__shell->pars[l__infopar + l__i + 1], &l__din, 16))
		{
			dbg_iprintf(l__shell->terminal, "Invalid data paremter - %s. Value ignored.\n", l__shell->pars[l__infopar + l__i + 1]);
		}
		
		if (l__din > 255)
		{
			dbg_iprintf(l__shell->terminal, "Parameter 0x%X is to big for 8-bit output. Value shortened\n", l__din);
		}
		
		l__buf[l__i] = l__din;		
	}

	/* Write it to the client memory */
	dbg_iprintf(l__shell->terminal, "Writing %i bytes of datas to client 0x%X at 0x%X...", l__datas * sizeof(uint8_t), l__sid, l__adr);
	
	if (dbg_write(l__sid, l__adr, l__buf, l__datas * sizeof(uint8_t)))
	{
		*tls_errno = 0;
		dbg_iprintf(l__shell->terminal, "( FAILED )\nCan't write datas to client. Please look at the system terminal for further informations.\n");
		return -1;
	}

	dbg_iprintf(l__shell->terminal, "( DONE )\n");
	
	mem_free(l__buf);
	
	return 0;
}

/*
 * dbg_sh_writec
 *
 * Write one or more charracters to the clients' memory
 *
 * Usage:
 *	writeb <address> [-c <sid>] -d string
 *	 	-c sid		Use a special sid (if not defined the current client will be used)
 */
int dbg_sh_writec(void)
{
	dbg_shell_t *l__shell = *dbg_tls_shellptr;
	int l__infopar = -1;
	size_t l__datas = 0;
	sid_t l__sid = 0;
	uintptr_t l__adr = 0;
	
	/* Right count of parameters? */
	if (l__shell->n_pars < 2)
	{
		dbg_iprintf(l__shell->terminal, "Missing parameters. Try \"help writec\" for more informations.\n");
		return -1;
	}
	
	/* Get the destination address */
	if (dbglib_atoul(l__shell->pars[1], &l__adr, 16))
	{
		dbg_iprintf(l__shell->terminal, "Illegal address - \"%s\".\n", l__shell->pars[1]);
		return NULL;
	}	
	
	/* Was a SID defined? */
	l__sid = dbg_get_sidpar("writea", SIDTYPE_THREAD);
	if (l__sid == SID_INVALID) return -1;

	/* Get the data parameter */
	l__infopar = dbg_test_par(0, "-d");
	if (l__infopar == -1)
	{
		dbg_iprintf(l__shell->terminal, "Missing data parameter. Try \"help writed\" for more informations.\n");
		return -1;
	}

	/* Store the datas to a buffer */
	if (((unsigned)l__infopar + 1) >= l__shell->n_pars)
	{
		dbg_iprintf(l__shell->terminal, "Missing datas. Try \"help writed\" for more informations.\n");
		return -1;
	}

	/* Copy without zero termination */
	l__datas = str_len(l__shell->pars[l__infopar + 1], DBGSHELL_CMDBUFFER_SIZE);
	if (*tls_errno)
	{
		return -1;
	}
	
	/* Write it to the client memory */
	dbg_iprintf(l__shell->terminal, "Writing %i bytes of datas to client 0x%X at 0x%X...", l__datas * sizeof(uint8_t), l__sid, l__adr);
	
	if (dbg_write(l__sid, l__adr, l__shell->pars[l__infopar + 1], l__datas))
	{
		*tls_errno = 0;
		dbg_iprintf(l__shell->terminal, "( FAILED )\nCan't write datas to client. Please look at the system terminal for further informations.\n");
		return -1;
	}

	dbg_iprintf(l__shell->terminal, "( DONE )\n");
	
	return 0;
}

/*
 * dbg_sh_dump
 *
 * Creates a stack dump.
 *
 * Usage:
 *	dump -n <levels> -c <client>
 *	 	-n <levels>	Number of stack entries to dump
 *	 	-c sid		Use a special sid (if not defined the current client will be used)
 *
 */
int dbg_sh_dump(void)
{
	dbg_shell_t *l__shell = *dbg_tls_shellptr;
	sid_t l__sid = 0;
	unsigned l__num;
		
	/* Get the number of levels */
	int l__infopar = dbg_test_par(0, "-n");
	if (l__infopar > -1)
	{
		/* The next parameter should contain our number */
		if (l__shell->n_pars < (unsigned)l__infopar)
		{
			dbg_iprintf(l__shell->terminal, "Missing count parameter. Try \"help dump\" for more informations.\n");
			return -1;
		}
		
		/* Get the number */
		if (dbglib_atoul(l__shell->pars[l__infopar + 1], &l__num, 10))
		{
			dbg_iprintf(l__shell->terminal, "Can't convert parameter - \"%s\".\n", l__shell->pars[l__infopar + 1]);
			return -1;
		}
	}
	 else
	{
		/* Just eight. */
		l__num = 8;
	}	
	
	/* No entries to load */
	if (l__num == 0) return 0;
	
	/* Was a SID defined? */
	l__sid = dbg_get_sidpar("dump", SIDTYPE_THREAD);
	if (l__sid == SID_INVALID) return -1;
	
	/* Load entries */
	dbg_iprintf(l__shell->terminal, "Stack of 0x%X at ESP 0x%X (%i levels)\n", l__sid, dbg_get_esp(l__sid), l__num);
	if (*tls_errno) {dbg_iprintf(l__shell->terminal, "Can't retreive ESP.\n"); return -1;}
	
	int l__pactr = 0;
	
	while (l__num --)
	{
		uint32_t l__val = dbg_read_stack(l__sid, l__num);
		if (*tls_errno) {dbg_iprintf(l__shell->terminal, "Can't read from level %i, because of %i.\n", l__num, *tls_errno); return -1;}
		
		dbg_iprintf(l__shell->terminal, "0x%.8X\t(adr 0x%.8X\tlevel %i\tesp + %i)\n", l__val, dbg_get_esp(l__sid) - (l__num * sizeof(uint32_t)), l__num, l__num * sizeof(uint32_t));
		
		l__pactr ++;
		
		if (((l__pactr % 12) == 0) && (l__pactr > 0)) dbg_pause(l__shell->terminal);		
	}
	
	return 0;
}

#define DBG_SETUNSET_FLAGS(___fvar, ___flname, ___flmakro)\
{\
	if (dbg_test_par(0, "+"#___flname) > -1)\
	{\
		l__client->___fvar |= ___flmakro;\
	}\
	\
	if (dbg_test_par(0, "-"#___flname) > -1)\
	{\
		l__client->___fvar &= ~___flmakro;\
	}\
}

/*
 * dbg_sh_trace
 *
 * Returns the current trace status or changes it.
 *
 * Usage:
 *	trace [-c <sid>] [+B|-B] [+I|-I] [+D|-D] [+S|-S] [+M|-M]
 *	 	-c sid		Use a special sid (if not defined the current client will be used)
 *		+B | -B		Set (+B) or delete (-B) breakpoint counting and tracing
 *		+I | -I		Set (+I) or delete (-I) tracing of other software interrupts
 *		+D | -D		Set (+D) or delete (-D) tracing of debugger calls
 *		+S | -S		Set (+S) or delete (-S) tracing of system calls
 *		+M | -M		Set (+M) or delete (-M) tracing of executed instructions
 *
 */
#define DBG_SETUNSET_TRACE(___flname, ___flmakro) DBG_SETUNSET_FLAGS(trace_flags, ___flname, ___flmakro)
 
int dbg_sh_trace(void)
{
	dbg_shell_t *l__shell = *dbg_tls_shellptr;
	sid_t l__sid = 0;
	uint32_t l__flags;
	
	/* Get the SID */
	l__sid = dbg_get_sidpar("trace", SIDTYPE_THREAD);
	if (l__sid == SID_INVALID) return -1;

	/* Get the client structure */
	mtx_lock(&dbg_clients_mtx, -1);
	
	/* Get the client structure */
	dbg_client_t* l__client = dbg_find_client(l__sid);
	if (l__client == NULL) 
	{
		mtx_unlock(&dbg_clients_mtx);
		dbg_iprintf(l__shell->terminal, "Can't find client 0x%X for \"trace\".\n", l__sid);
		return 0xFFFFFFFF;
	}

	/* Test and change the flags */
	DBG_SETUNSET_TRACE(B, DBG_TRACE_BREAKPOINT_COUNTER);
	DBG_SETUNSET_TRACE(I, DBG_TRACE_OTHER_SOFTINTS);
	DBG_SETUNSET_TRACE(D, DBG_TRACE_DEBUGGER_CALLS);
	DBG_SETUNSET_TRACE(S, DBG_TRACE_SYSCALLS);
	DBG_SETUNSET_TRACE(M, DBG_TRACE_MACHINE_EXECUTION);
	DBG_SETUNSET_TRACE(b, DBG_TRACE_BREAKPOINT_COUNTER);
	DBG_SETUNSET_TRACE(i, DBG_TRACE_OTHER_SOFTINTS);
	DBG_SETUNSET_TRACE(d, DBG_TRACE_DEBUGGER_CALLS);
	DBG_SETUNSET_TRACE(s, DBG_TRACE_SYSCALLS);
	DBG_SETUNSET_TRACE(m, DBG_TRACE_MACHINE_EXECUTION);
		
	/* Save the flag state for output */
	l__flags = l__client->trace_flags;
	
	mtx_unlock(&dbg_clients_mtx);
	
	dbg_iprintf(l__shell->terminal, "Trace flags for 0x%X (0x%X): \n", l__sid, l__flags);
	
	if (l__flags & DBG_TRACE_BREAKPOINT_COUNTER)
	{
		dbg_iprintf(l__shell->terminal, "- Trace breakpoint counting\n");
	}
	if (l__flags & DBG_TRACE_OTHER_SOFTINTS)
	{
		dbg_iprintf(l__shell->terminal, "- Trace other software interrupts\n");
	}	
	if (l__flags & DBG_TRACE_DEBUGGER_CALLS)
	{
		dbg_iprintf(l__shell->terminal, "- Trace calls to the debugger\n");
	}	
	if (l__flags & DBG_TRACE_SYSCALLS)
	{
		dbg_iprintf(l__shell->terminal, "- Trace system calls\n");
	}	
	if (l__flags & DBG_TRACE_MACHINE_EXECUTION)
	{
		dbg_iprintf(l__shell->terminal, "- Trace executed machine code\n");
	}	
	
	if (l__flags == 0)
	{
		dbg_iprintf(l__shell->terminal, "(Tracing deactivated)\n");
	}
	
	dbg_iprintf(l__shell->terminal, "\n");
	
	return 0;
}

/*
 * dbg_sh_halton
 *
 * Returns the current halt conditions or changes it.
 *
 * Usage:
 *	halton [-c <sid>] [+B|-B] [+I|-I] [+D|-D] [+S|-S] [+M|-M]
 *	 	-c sid		Use a special sid (if not defined the current client will be used)
 *		+B | -B		Set (+B) or delete (-B) stopping on breakpoints
 *		+I | -I		Set (+I) or delete (-I) stopping on other software interrupts
 *		+D | -D		Set (+D) or delete (-D) stopping on debugger calls
 *		+S | -S		Set (+S) or delete (-S) stopping on system calls
 *		+M | -M		Set (+M) or delete (-M) stopping on every executed instruction
 *
 */
#define DBG_SETUNSET_HALT(___flname, ___flmakro) DBG_SETUNSET_FLAGS(halt_flags, ___flname, ___flmakro)
 
int dbg_sh_halton(void)
{
	dbg_shell_t *l__shell = *dbg_tls_shellptr;
	sid_t l__sid = 0;
	uint32_t l__flags;
	
	/* Get the SID */
	l__sid = dbg_get_sidpar("halton", SIDTYPE_THREAD);
	if (l__sid == SID_INVALID) return -1;

	/* Get the client structure */
	mtx_lock(&dbg_clients_mtx, -1);
	
	/* Get the client structure */
	dbg_client_t* l__client = dbg_find_client(l__sid);
	if (l__client == NULL) 
	{
		mtx_unlock(&dbg_clients_mtx);
		dbg_iprintf(l__shell->terminal, "Can't find client 0x%X for \"halton\".\n", l__sid);
		return 0xFFFFFFFF;
	}

	/* Test and change the flags */
	DBG_SETUNSET_HALT(B, DBG_HALT_BREAKPOINTS);
	DBG_SETUNSET_HALT(I, DBG_HALT_OTHER_SOFTINTS);
	DBG_SETUNSET_HALT(D, DBG_HALT_DEBUGGER_CALLS);
	DBG_SETUNSET_HALT(S, DBG_HALT_SYSCALLS);
	DBG_SETUNSET_HALT(M, DBG_HALT_SINGLE_STEP);
	DBG_SETUNSET_HALT(b, DBG_HALT_BREAKPOINTS);
	DBG_SETUNSET_HALT(i, DBG_HALT_OTHER_SOFTINTS);
	DBG_SETUNSET_HALT(d, DBG_HALT_DEBUGGER_CALLS);
	DBG_SETUNSET_HALT(s, DBG_HALT_SYSCALLS);
	DBG_SETUNSET_HALT(m, DBG_HALT_SINGLE_STEP);
	
	/* Save the flag state for output */
	l__flags = l__client->halt_flags;
	
	mtx_unlock(&dbg_clients_mtx);
	
	dbg_iprintf(l__shell->terminal, "Halt conditions for 0x%X (0x%X): \n", l__sid, l__flags);
	
	if (l__flags & DBG_HALT_BREAKPOINTS)
	{
		dbg_iprintf(l__shell->terminal, "- Halt on every breaking point\n");
	}
	if (l__flags & DBG_HALT_OTHER_SOFTINTS)
	{
		dbg_iprintf(l__shell->terminal, "- Halt on other software interrupts\n");
	}	
	if (l__flags & DBG_HALT_DEBUGGER_CALLS)
	{
		dbg_iprintf(l__shell->terminal, "- Halt on debugger calls\n");
	}	
	if (l__flags & DBG_HALT_SYSCALLS)
	{
		dbg_iprintf(l__shell->terminal, "- Halt on system calls\n");
	}	
	if (l__flags & DBG_HALT_SINGLE_STEP)
	{
		dbg_iprintf(l__shell->terminal, "- Halt on every executed instruction\n");
	}	
	
	if (l__flags == 0)
	{
		dbg_iprintf(l__shell->terminal, "(No halting conditions)\n");
	}
	
	dbg_iprintf(l__shell->terminal, "\n");
	
	return 0;
}

/*
 * dbg_sh_break
 *
 * Installs, deletes or configures a breakpoint
 *
 * Usage:
 *      break {<address>|-n <name>|-l} -o {add|delete|get|inc|reset} [-c client]
 *
 *		<address>	Address to watch
 *		-n name		Name of the breakpoint (default 0xADDRESS)
 *				At least one of both the address or the name has to be defined.
 *				You can define both at one time for the add parameter.
 *		-l		Just list all break points (-o will be ignored)
 * 
 *	 	-c sid		Use a special sid (if not defined the current client will be used) 
 *
 *		-o <op>		Operation for breakpoint:
 *					add	Add a new breakpoint
 *					delete	Deletes an existing breakpoint
 *					inc	Increments its usage counter
 *					get	Reads its usage counter
 *					reset	Resets its usage counter 
 *
 */
int dbg_sh_break(void)
{
	dbg_shell_t *l__shell = *dbg_tls_shellptr;
	sid_t l__sid = 0;
	uintptr_t l__adr;
	utf8_t *l__name = NULL;
	int l__infopar = 0;
		
	/* We need at least the address parameter */
	if (l__shell->n_pars < 2) 
	{
		dbg_iprintf(l__shell->terminal, "Missing parameter. Try \"help break\" for more informations.\n");
		return -1;
	}
	
	/* Get the SID */
	l__sid = dbg_get_sidpar("break", SIDTYPE_THREAD);
	if (l__sid == SID_INVALID) return -1;	
	
	/* Get the client structure */
	mtx_lock(&dbg_clients_mtx, -1);
	dbg_client_t* l__client = dbg_find_client(l__sid);
	if (l__client == NULL) 
	{
		mtx_unlock(&dbg_clients_mtx);
		dbg_iprintf(l__shell->terminal, "Can't find client 0x%X for \"break\".\n", l__sid);
		return 0xFFFFFFFF;
	}		

	/* Address or name? */
	if ((l__infopar = dbg_test_par(0, "-n")) > -1)
	{
		if (((unsigned)l__infopar + 1) >= l__shell->n_pars)
		{
			dbg_iprintf(l__shell->terminal, "Missing name parameter. Try \"help break\" for more informations.\n");
			mtx_unlock(&dbg_clients_mtx);
			return -1;
		}
		
		l__name = l__shell->pars[l__infopar + 1];
		
		l__adr = dbg_get_breakpoint_adr(l__client, l__shell->pars[l__infopar + 1]);
		if ((l__adr == 0) && (dbg_test_par(0, "add") == -1))
		{
			dbg_iprintf(l__shell->terminal, "Can't find breakpoint %s.\n", l__shell->pars[l__infopar + 1]);
			mtx_unlock(&dbg_clients_mtx);
			return -1;
		}
		
		/* Read address although, if add operation */
		if (dbg_test_par(0, "add") > -1)
		{
			/* Read address */
			if (dbglib_atoul(l__shell->pars[1], &l__adr, 16))
			{
				dbg_iprintf(l__shell->terminal, "Invalid address - %s.\n", l__shell->pars[1]);
				mtx_unlock(&dbg_clients_mtx);
				return -1;
			}		
		}
	}
	 else if (dbg_test_par(0, "-l") > -1)
	{
		/* Just list all breakpoints */
		dbg_breaks_t *l__brk = l__client->breakpoints;
		int l__i = 0;
				
		while (l__brk != NULL)
		{
			dbg_iprintf(l__shell->terminal, "%.32s\t0x%.8X\t\t%i\n", l__brk->name, l__brk->address, l__brk->count);
			l__brk = l__brk->ls.n;
			l__i ++;
			if (l__i > 20) dbg_pause(l__shell->terminal);
		}
		
		mtx_unlock(&dbg_clients_mtx);
		return 0;
	}
	 else
	{	
		/* Read address */
		if (dbglib_atoul(l__shell->pars[1], &l__adr, 16))
		{
			dbg_iprintf(l__shell->terminal, "Invalid address - %s.\n", l__shell->pars[1]);
			mtx_unlock(&dbg_clients_mtx);
			return -1;
		}
	}
	
	/* Address 0 is invalid */
	if (l__adr == 0)
	{
		dbg_iprintf(l__shell->terminal, "Invalid breakpoint address 0x%X.\n", l__adr);
		mtx_unlock(&dbg_clients_mtx);
		return -1;
	}
		
	/* Find the operation parameter */
	if ((l__infopar = dbg_test_par(0, "-o")) > -1)
	{
		/* Is there a following parameter to -o? */
		if (((unsigned)l__infopar + 1) >= l__shell->n_pars)
		{
			dbg_iprintf(l__shell->terminal, "Missing operation parameter. Try \"help break\" for more informations.\n");
			mtx_unlock(&dbg_clients_mtx);
			return -1;		
		}
		
		/* Test the operations */
		if (!str_compare(l__shell->pars[l__infopar +1], "add", 3))
		{
			int l__do_free_name = 0;
			
			/* Operation add */
			if (l__name == NULL)
			{
				l__name = mem_alloc(12);
				if (l__name == NULL)
				{
					dbg_iprintf(l__shell->terminal, "Can't allocate name buffer, because of %i.\n", *tls_errno);
					mtx_unlock(&dbg_clients_mtx);
					return -1;			
				}
				
				snprintf(l__name, 11, "0x%X", l__adr);
				l__do_free_name = 1;
			}

			if (dbg_add_breakpoint(l__client, l__name, l__adr))
			{
				if (l__do_free_name) mem_free(l__name);
				dbg_iprintf(l__shell->terminal, "Can't create break point 0x%X of client 0x%X. Look at the system terminal for further informations.\n", l__adr, l__sid);
				mtx_unlock(&dbg_clients_mtx);
				return -1;
			}
			
			if (l__do_free_name) mem_free(l__name);
			
			mtx_unlock(&dbg_clients_mtx);
			return 0;
			
		}
		 else if (!str_compare(l__shell->pars[l__infopar +1], "del", 3))
		{
			/* Operation del */
			if (dbg_del_breakpoint(l__client, l__adr))
			{
				dbg_iprintf(l__shell->terminal, "Can't delete break point 0x%X of client 0x%X. Look at the system terminal for further informations.\n", l__adr, l__sid);
				
				mtx_unlock(&dbg_clients_mtx);
				return -1;
			}
			
			mtx_unlock(&dbg_clients_mtx);
			return 0;
		}
		 else if (!str_compare(l__shell->pars[l__infopar +1], "get", 3))
		{
			dbg_breaks_t l__buf;
			
			/* Operation get */
			if (dbg_get_breakpoint_data(l__client, l__adr, &l__buf))
			{
				dbg_iprintf(l__shell->terminal, "Can't get break point 0x%X of client 0x%X. Look at the system terminal for further informations.\n", l__adr, l__sid);
				
				mtx_unlock(&dbg_clients_mtx);
				return -1;
			}
			
			dbg_iprintf(l__shell->terminal, "Breakpoint:\t0x%X\nName:\t%s\nCounter:\t%i\n\n", l__buf.address, l__buf.name, l__buf.count);
			
			mtx_unlock(&dbg_clients_mtx);
			return 0;
		}
		 else if (!str_compare(l__shell->pars[l__infopar +1], "inc", 3))
		{
			dbg_breaks_t l__buf;
			
			/* Operation get */
			if (dbg_inc_breakpoint_ctr(l__client, l__adr, &l__buf))
			{
				dbg_iprintf(l__shell->terminal, "Can't get break point 0x%X of client 0x%X. Look at the system terminal for further informations.\n", l__adr, l__sid);
				mtx_unlock(&dbg_clients_mtx);
				return -1;
			}
			
			dbg_iprintf(l__shell->terminal, "Incremented counter.\nBreakpoint:\t0x%X\nName:\t%s\nCounter:\t%i\n\n", l__buf.address, l__buf.name, l__buf.count);
			
			mtx_unlock(&dbg_clients_mtx);
			return 0;
		}
 		 else if (!str_compare(l__shell->pars[l__infopar +1], "reset", 3))
		{
			dbg_breaks_t l__buf;
			
			/* Operation get */
			if (dbg_reset_breakpoint_ctr(l__client, l__adr, &l__buf))
			{
				dbg_iprintf(l__shell->terminal, "Can't get break point 0x%X of client 0x%X. Look at the system terminal for further informations.\n", l__adr, l__sid);
				
				mtx_unlock(&dbg_clients_mtx);
				return -1;
			}
			
			dbg_iprintf(l__shell->terminal, "Reset counter.\nBreakpoint:\t0x%X\nName:\t%s\nCounter:\t%i\n\n", l__buf.address, l__buf.name, l__buf.count);
			
			mtx_unlock(&dbg_clients_mtx);
			return 0;
		}
		 else
		{
			dbg_iprintf(l__shell->terminal, "Invalid operation - %s.\n", l__shell->pars[l__infopar +1]);
			
			mtx_unlock(&dbg_clients_mtx);
			return -1;		
		}
	}
	 else
	{
		dbg_iprintf(l__shell->terminal, "Missing operation parameter. Try \"help break\" for more informations.\n");
		
		mtx_unlock(&dbg_clients_mtx);
		return -1;
	}
}


