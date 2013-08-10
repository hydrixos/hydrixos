/*
 *
 * trace.c
 *
 * (C)2006 by Friedrich Gr�ter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying').   
 *
 * Trace functions
 *
 */
#include <hydrixos/types.h>
#include <hydrixos/tls.h>
#include <hydrixos/errno.h>
#include <hydrixos/blthr.h>
#include <hydrixos/mem.h>
#include <hydrixos/stdfun.h>
#include <hydrixos/system.h>

#include <hydrixos/pmap.h>

#include "../hyinit.h"
#include "coredbg.h"

/*
 * dbg_get_registers
 *
 * Returns the register state of another thread.
 * The other thread doesn't need to be a client
 * of the debugger. But it is strongly recommended
 * to read the registers from a freezed thread only.
 *
 */
dbg_registers_t dbg_get_registers(sid_t client)
{
	reg_t l__generic = hymk_read_regs(client, REGS_X86_GENERIC);
	reg_t l__index = hymk_read_regs(client, REGS_X86_INDEX);
	reg_t l__pointers = hymk_read_regs(client, REGS_X86_POINTERS);
	reg_t l__eflags = hymk_read_regs(client, REGS_X86_EFLAGS);
	dbg_registers_t l__retval = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	
	/* Error */
	if (*tls_errno)
	{
		dbg_shell_t *l__shell = *dbg_tls_shellptr;
		
		dbg_iprintf(l__shell->terminal, "Can't read registers from 0x%X, because of %i.\n", *tls_errno);
		return l__retval;
	}
	
	/* Store registers */
	l__retval.eax = l__generic.regs[0];
	l__retval.ebx = l__generic.regs[1];
	l__retval.ecx = l__generic.regs[2];
	l__retval.edx = l__generic.regs[3];
	l__retval.esi = l__index.regs[0];
	l__retval.edi = l__index.regs[1];
	l__retval.ebp = l__index.regs[2];
	l__retval.esp = l__pointers.regs[0];
	l__retval.eip = l__pointers.regs[1];
	l__retval.eflags = l__eflags.regs[0];
	
	return l__retval;
}

/*
 * dbg_set_registers
 *
 * Changes the register state of another thread.
 * The other thread doesn't need to be a client
 * of the debugger. But it is strongly recommended
 * to change the registers of a freezed thread only. 
 *
 */
void dbg_set_registers(sid_t client, dbg_registers_t reg)
{
	reg_t l__generic;
	reg_t l__index;
	reg_t l__pointers;
	reg_t l__eflags;
	
	/* Load registers */
	l__generic.regs[0] = reg.eax;
	l__generic.regs[1] = reg.ebx;
	l__generic.regs[2] = reg.ecx;
	l__generic.regs[3] = reg.edx;
	l__index.regs[0] = reg.esi;
	l__index.regs[1] = reg.edi;
	l__index.regs[2] = reg.ebp;
	l__pointers.regs[0] = reg.esp;
	l__pointers.regs[1] = reg.eip;
	l__eflags.regs[0] = reg.eflags;
	
	/* Store them */
	hymk_write_regs(client, REGS_X86_GENERIC, l__generic);
	hymk_write_regs(client, REGS_X86_INDEX, l__index);
	hymk_write_regs(client, REGS_X86_POINTERS, l__pointers);
	hymk_write_regs(client, REGS_X86_EFLAGS, l__eflags);
	
	/* Error */
	if (*tls_errno)
	{
		dbg_shell_t *l__shell = *dbg_tls_shellptr;
		
		dbg_iprintf(l__shell->terminal, "Can't change registers of 0x%X, because of %i.\n", *tls_errno);
		return;
	}
	
	
	return;
}

/*
 * dbg_get_eip(client)
 *
 * Get the content of the EIP register of the client "client".
 *
 */
uintptr_t dbg_get_eip(sid_t client)
{
	dbg_registers_t l__reg = dbg_get_registers(client);
	if (*tls_errno) return 0;
	
	return l__reg.eip;
}

/*
 * dbg_set_eip(client)
 *
 * Set the content of the EIP register of the client "client".
 *
 */
void dbg_set_eip(sid_t client, uintptr_t adr)
{
	dbg_registers_t l__reg = dbg_get_registers(client);
	if (*tls_errno) return;
	
	l__reg.eip = adr;
	dbg_set_registers(client, l__reg);
	
	return;
}

/*
 * dbg_get_esp(client)
 *
 * Get the content of the ESP register of the client "client".
 *
 */
uintptr_t dbg_get_esp(sid_t client)
{
	dbg_registers_t l__reg = dbg_get_registers(client);
	if (*tls_errno) return 0;
	
	return l__reg.esp;
}

/*
 * dbg_set_esp(client)
 *
 * Set the content of the ESP register of the client "client".
 *
 */
void dbg_set_esp(sid_t client, uintptr_t adr)
{
	dbg_registers_t l__reg = dbg_get_registers(client);
	if (*tls_errno) return;
	
	l__reg.esp = adr;
	dbg_set_registers(client, l__reg);
	
	return;
}

/*
 * dbg_get_eflags(client)
 *
 * Get the content of the EFLAGS register of the client "client".
 *
 */
uint32_t dbg_get_eflags(sid_t client)
{
	dbg_registers_t l__reg = dbg_get_registers(client);
	if (*tls_errno) return 0;
	
	return l__reg.eflags;
}

/*
 * dbg_set_eflags(client)
 *
 * Set the content of the EFLAGS register of the client "client".
 *
 */
void dbg_set_eflags(sid_t client, uint32_t eflags)
{
	dbg_registers_t l__reg = dbg_get_registers(client);
	if (*tls_errno) return;
	
	l__reg.eflags = eflags;
	dbg_set_registers(client, l__reg);
	
	return;
}

/*
 * dbg_prep_access(client, adr, len, op)
 *
 * Prepares the memory access to a client "client" at address "adr"
 * and for a length of "len" bytes. The operation will bei "op" (PGA_READ 
 * or PGA_WRITE).
 *
 * Return value:
 *	!= NULL		VFS memory buffer
 *	== NULL		Error
 */
static void* dbg_prep_access(sid_t client, uintptr_t adr, size_t len, unsigned op)
{
	uintptr_t l__offset = adr & 0xfff;
	unsigned l__flags;
	
	/* Get the count of pages to read */
	int l__pages = len / 4096;
	if (l__offset) l__pages ++;
	if ((adr + len) % 4096) l__pages ++;

	adr &= (~0xfff);

	/* Read the operation flags */
	if (op == PGA_READ)
	{ 
		l__flags = MAP_READ;
	}
	else if (op == PGA_WRITE)
	{
		l__flags = MAP_WRITE;
	}
	else
	{
		return NULL;	
	}

	/* Keep it stopped */
	hymk_freeze_subject(client);
	if (*tls_errno) {dbg_isprintf("Can't freeze 0x%X for accessing to its memory, because of %i.\n", client, adr); *tls_errno = 0; return NULL;}
	
	/* Save its allow status */
	uint32_t l__old_sid = hysys_thrtab_read(client, THRTAB_MEMORY_OP_SID);
	if (*tls_errno) {dbg_isprintf("Can't read from the settings of 0x%X, because of %i.\n", client, adr); *tls_errno = 0; return NULL;}
	
	uintptr_t l__old_destadr = hysys_thrtab_read(client, THRTAB_MEMORY_OP_DESTADR);
	if (*tls_errno) {dbg_isprintf("Can't read from the settings of 0x%X, because of %i.\n", client, adr); *tls_errno = 0; return NULL;}
	
	uint32_t l__old_maxsize = hysys_thrtab_read(client, THRTAB_MEMORY_OP_MAXSIZE);
	if (*tls_errno) {dbg_isprintf("Can't read from the settings of 0x%X, because of %i.\n", client, adr); *tls_errno = 0; return NULL;}
	
	uint32_t l__old_allowed = hysys_thrtab_read(client, THRTAB_MEMORY_OP_ALLOWED);
	if (*tls_errno) {dbg_isprintf("Can't read from the settings of 0x%X, because of %i.\n", client, adr); *tls_errno = 0; return NULL;}
	
	/* Allocate an access buffer */
	void* l__tmp = pmap_alloc(l__pages * 4096);
	if (l__tmp == NULL) 
	{
		dbg_isprintf("Can't allocate %i bytes of VFS memory for sharing, because of %i.\n", l__pages, *tls_errno);
		*tls_errno = 0;
		return NULL;
	}
	
	/* Allow our new operation */
	hymk_allow((*tls_my_thread)->thread_sid, client, (void*)adr, l__pages, ALLOW_MAP|ALLOW_REVERSE);
	if (*tls_errno)
	{
		dbg_isprintf("Can't allow access to 0x%X at 0x%X for %i pages, because of %i.\n", client, adr, l__pages, *tls_errno);
		hymk_awake_subject(client);
		*tls_errno = 0;
		
		pmap_free(l__tmp);
		
		return NULL;
	}
	
	/* Map the page */
	hysys_map(client, l__tmp, l__pages, l__flags|MAP_REVERSE, 0);
	if (*tls_errno)
	{
		dbg_isprintf("Can't map data from client 0x%X, because of %i.\n", client, *tls_errno);
		*tls_errno = 0;
		
		/* Reset allow state */
		hymk_allow(l__old_sid, client, (void*)l__old_destadr, l__old_maxsize, l__old_allowed);
		if (*tls_errno)
		{
			dbg_isprintf("(1) Unable to reset allow state of client thread 0x%X, because of %i.\n", client, *tls_errno);
			*tls_errno = 0;
		}
			
		hymk_awake_subject(client);
			
		pmap_free(l__tmp);
		return NULL;
	}

	/* Reset allow state */
	hymk_allow(l__old_sid, client, (void*)l__old_destadr, l__old_maxsize, l__old_allowed);
	if (*tls_errno)
	{
		dbg_isprintf("(2) Unable to reset allow state of client thread 0x%X, because of %i", client, *tls_errno);
		*tls_errno = 0;
		
		hymk_awake_subject(client);
		
		pmap_free(l__tmp);
		return NULL;
	}
			
	/* Find inactive pages in our mapping by filling it with new page frames */
	uintptr_t l__tmpadr = adr;
	
	while (l__pages --)
	{
		/* There is a page which is not readable... */
		if (!(hymk_test_page(l__tmpadr, client) & op))
		{
			dbg_isprintf("Can't access to 0x%X at 0x%X to page 0x%X. Access denied (PGA-flags 0x%X for 0x%X).\n", client, adr, l__tmpadr, hymk_test_page(l__tmpadr, client), op);
			if (*tls_errno) 
			{
				dbg_isprintf("Can't test rights, because of %i.\n", *tls_errno);
				*tls_errno = 0;
			}
			
			pmap_free(l__tmp);
			return NULL;
		}
		
		l__tmpadr += 4096;
	}
	
	/* Make sure, that we are really access synchronized memory */
	HYSYS_MSYNC();

	return l__tmp;
}

/*
 * dbg_read(client, adr, buf, len)
 *
 * Reads "len" bytes of the memory of client "client" at its address "adr"
 * to "buf". 
 *
 * Return value:
 *	0	Successful
 *	1	Error
 *
 */
int dbg_read(sid_t client, uintptr_t adr, void* buf, size_t len)
{	
	uintptr_t l__offset = adr & 0xfff;
	uint8_t* l__tmp = dbg_prep_access(client, adr, len, PGA_READ);
		
	if (l__tmp == NULL) 
		return 1;
	
	/* Read data to buf */
	buf_copy(buf, &l__tmp[l__offset], len);
	if (*tls_errno)
	{
		dbg_isprintf("Can't copy data in dbg_read, because of %i.\n", *tls_errno);
		*tls_errno = 0;
		
		pmap_free(l__tmp);
		return 1;
	}
	
	/* Awake the client */
	hymk_awake_subject(client);	
	
	/* Free the buffer */
	pmap_free(l__tmp);
	return 0;
}

/*
 * dbg_write(client, adr, buf, len)
 *
 * Writes "len" bytes from "buf" to the memory of client "client" at its address "adr".
 *
 * Return value:
 *	0	Successful
 *	1	Error
 *
 */
int dbg_write(sid_t client, uintptr_t adr, const void* buf, size_t len)
{	
	uintptr_t l__offset = adr & 0xfff;
	uint8_t* l__tmp = dbg_prep_access(client, adr, len, PGA_WRITE);
		
	if (l__tmp == NULL) 
		return 1;
		
	/* Write data to buf */
	buf_copy(&l__tmp[l__offset], buf, len);
	if (*tls_errno)
	{
		dbg_isprintf("Can't copy data in dbg_write, because of %i.\n", *tls_errno);
		*tls_errno = 0;
		
		pmap_free(l__tmp);
		return 1;
	}

	/* Awake the client */
	hymk_awake_subject(client);	
	
	/* Free the buffer */
	pmap_free(l__tmp);
	return 0;
}

/*
 * dbg_read_stack(client, level)
 *
 * Reads a DWORD from the stack of the client "client". The
 * DWORD will be located at ESP - (level * sizeof(uint32_t)).
 *
 */
uint32_t dbg_read_stack(sid_t client, int level)
{
	uint32_t l__buf;
	uintptr_t l__adr;
	
	/* Get its ESP */
	uintptr_t l__esp = dbg_get_esp(client);
	if (*tls_errno) return 0;
	
	l__adr = l__esp - (level * sizeof(uint32_t));
		
	if (dbg_read(client, l__adr, &l__buf, sizeof(uint32_t)))
	{
		dbg_isprintf("Can't read datas from the stack of 0x%X on level 0x%X at ESP 0x%X + 0x%X = 0x%X, because of %i.\n", client, level, l__esp, level * sizeof(uint32_t), l__adr, *tls_errno);
		return 0;
	}
	
	return l__buf;
}

/*
 * dbg_set_traceflags(client, flags)
 *
 * Changes the trace flags of a client "client" to "flags".
 *
 * This function expects dbg_client_mtx to be locked and leaves it locked!
 *
 * Return value:
 *	== 0	Successful
 *	!= 0	Error
 *
 */
int dbg_set_traceflags(dbg_client_t *client, uint32_t flags)
{
	/* Change its flags */
	client->trace_flags = flags;
	
	return 0;
}	

/*
 * dbg_get_traceflags(client)
 *
 * Returns the trace flags of a client "client".
 *
 * This function expects dbg_client_mtx to be locked and leaves it locked!
 *
 * Return value:
 *	!= 0xFFFFFFFF  Trace flags of client "client".
 *	== 0xFFFFFFFF  Error
 *
 */
uint32_t dbg_get_traceflags(dbg_client_t *client)
{
	return client->trace_flags;
}

/*
 * dbg_set_haltflags(client, flags)
 *
 * Changes the halt flags of a client "client" to "flags".
 *
 * This function expects dbg_client_mtx to be locked and leaves it locked!
 *
 * Return value:
 *	== 0	Successful
 *	!= 0	Error
 *
 */
int dbg_set_haltflags(dbg_client_t *client, uint32_t flags)
{
	/* Change its flags */
	client->halt_flags = flags;
	
	return 0;
}	

/*
 * dbg_get_traceflags(client)
 *
 * Returns the halt flags of a client "client".
 *
 * This function expects dbg_client_mtx to be locked and leaves it locked!
 *
 * Return value:
 *	!= 0xFFFFFFFF  Trace flags of client "client".
 *	== 0xFFFFFFFF  Error
 *
 */
uint32_t dbg_get_haltflags(dbg_client_t *client)
{
	return client->halt_flags;
}

/*
 * dbg_get_breakpoint_data(client, adr)
 *
 * Returns a pointer to a breakpoint structure. This
 * function expects the dbg_clients_mtx to be locked
 * and returns a pointer to the breaking point structure.
 * The function won't modify the state of dbg_client_mutex.
 *
 * Return value:
 *	!= NULL		Pointer to the breaking point structure
 *	== NULL		No structure found
 *
 */
static dbg_breaks_t* dbg_get_breakpoint(dbg_client_t *client, uintptr_t adr)
{
	dbg_breaks_t *l__brk = client->breakpoints;
	
	/* Search the breaking point */
	while (l__brk != NULL)
	{
		if (l__brk->address == adr)
			return l__brk;
		
		l__brk = l__brk->ls.n;
	}
	
	return NULL;
}

/*
 * dbg_add_breakpoints(client, name, adr)
 *
 * Adds a breakpoint for client "client" with name "name" at address
 * "adr". Fails if a breakpoint already exists on the same address
 *
 * This function expects dbg_client_mtx to be locked and leaves it locked!
 *
 * Return value:
 *	0	Successful
 *	!= 0	Error
 *
 */
int dbg_add_breakpoint(dbg_client_t *client, const utf8_t *name, uintptr_t adr)
{
	dbg_breaks_t *l__brk;

	/* Is there any existing breaking point for the same address? */
	if (dbg_get_breakpoint(client, adr) != NULL)
	{
		dbg_isprintf("Breakpoint already exists for 0x%X at 0x%X.\n", client->client, adr);
		return 1;
	}
	
	/* Okay, just create one */
	l__brk = mem_alloc(sizeof(dbg_breaks_t));
	if (l__brk == NULL)
	{
		dbg_isprintf("Can't create breaking point 0x%X for client 0x%X, because of %i.\n", adr, client->client, *tls_errno);
		return 1;
	}
	
	/* Configure it */
	str_copy(l__brk->name, name, 32);
	l__brk->address = adr;
	l__brk->count = 0;
	
	lst_add(client->breakpoints, l__brk);
	
	client->breakpoints_n ++;
	
	return 0;
}

/*
 * dbg_del_breakpoints(client, adr)
 *
 * Deletes a breaking point of the client "client" which is at address "adr".
 *
 * This function expects dbg_client_mtx to be locked and leaves it locked!
 *
 * Return value:
 *	0	Successful
 *	!= 0	Error
 *
 */
int dbg_del_breakpoint(dbg_client_t *client, uintptr_t adr)
{
	dbg_breaks_t *l__brk;
	
	/* Is there any existing breaking point for the same address? */
	if ((l__brk = dbg_get_breakpoint(client, adr)) == NULL)
	{
		dbg_isprintf("Breakpoint doesn't exists for 0x%X at 0x%X.\n", client->client, adr);
		return 1;
	}
	
	/* Okay, delete it */
	lst_dellst(client->breakpoints, l__brk);
	
	mem_free(l__brk);
	
	client->breakpoints_n --;
	
	return 0;
}

/*
 * dbg_free_breakpoint_list(client)
 *
 * Destroys all breaking point entries of the client structure "client".
 * This function expects dbg_clients_mtx to be locked and leaves it
 * also in this state.
 *
 */
void dbg_free_breakpoint_list(dbg_client_t* client)
{
	dbg_breaks_t *l__brk = client->breakpoints;
	
	while (l__brk != NULL)
	{
		dbg_breaks_t *l__n = l__brk->ls.n;
			
		mem_free(l__brk);
		l__brk = l__n;
	}
	
	client->breakpoints = NULL;
	
	client->breakpoints_n = 0;
}

/*
 * dbg_inc_breakpoint_ctr(client, adr, buf)
 *
 * Increments the breakpoint access counter for the
 * breakpoint of the client "client" at address "adr".
 * It returns a copy of the break point structure to "buf"
 * after that.
 *
 * This function expects dbg_client_mtx to be locked and leaves it locked!
 *
 * Return value:
 *	0	Successful (breakpoint structure -> *buf)
 *	!= 0	Error
 *
 */
int dbg_inc_breakpoint_ctr(dbg_client_t *client, uintptr_t adr, dbg_breaks_t *buf)
{
	dbg_breaks_t *l__brk;
		
	/* Is there any existing breaking point for the same address? */
	if ((l__brk = dbg_get_breakpoint(client, adr)) == NULL)
	{
		return 1;
	}

	l__brk->count ++;
	if (buf != NULL) *buf = *l__brk;
	
	return 0;
}

/*
 * dbg_reset_breakpoint_ctr(client, adr, buf)
 *
 * Resets the breakpoint access counter for the
 * breakpoint of the client "client" at address "adr" to 0.
 * It returns a copy of the break point structure to "buf"
 * after that.
 *
 * This function expects dbg_client_mtx to be locked and leaves it locked!
 *
 * Return value:
 *	0	Successful (breakpoint structure -> *buf)
 *	!= 0	Error
 *
 */
int dbg_reset_breakpoint_ctr(dbg_client_t *client, uintptr_t adr, dbg_breaks_t *buf)
{
	dbg_breaks_t *l__brk;
	
	/* Is there any existing breaking point for the same address? */
	if ((l__brk = dbg_get_breakpoint(client, adr)) == NULL)
	{
		dbg_isprintf("Breakpoint doesn't exists for 0x%X at 0x%X.\n", client->client, adr);
		return 1;
	}

	l__brk->count = 0;
	if (buf != NULL) *buf = *l__brk;
	
	return 0;
}

/*
 * dbg_get_breakpoint_data(client, adr, buf)
 *
 * It returns a copy of the break point structure to "buf".
 *
 * This function expects dbg_client_mtx to be locked and leaves it locked!
 *
 * Return value:
 *	0	Successful (breakpoint structure -> *buf)
 *	!= 0	Error
 *
 */
int dbg_get_breakpoint_data(dbg_client_t *client, uintptr_t adr, dbg_breaks_t *buf)
{
	dbg_breaks_t *l__brk;
	
	/* Is there any existing breaking point for the same address? */
	if ((l__brk = dbg_get_breakpoint(client, adr)) == NULL)
	{
		return 1;
	}

	if (buf != NULL) *buf = *l__brk;
	
	return 0;
}

/*
 * dbg_get_breakpoint_adr(client, name)
 *
 * Returns the address of a breakpoint of the client "client" which has the
 * name "name".
 *
 * This function expects dbg_client_mtx to be locked and leaves it locked!
 *
 * Return value:
 *	0 	Error
 *	!= 0	Address of the break point
 *
 */
uintptr_t dbg_get_breakpoint_adr(dbg_client_t *client, const utf8_t *name)
{
	uintptr_t l__retval = 0;
	
	dbg_breaks_t *l__brk = client->breakpoints;
	
	/* Search the breaking point */
	while (l__brk != NULL)
	{
		if (!str_compare(l__brk->name, name, 32))
		{
			l__retval = l__brk->address;
			
			return l__retval;
		}
		
		l__brk = l__brk->ls.n;
	}	
	
	return 0;
}
