/*
 *
 * variable.c
 *
 * (C)2006 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying').   
 *
 * Shell variables
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

#include <coredbg/cdebug.h>

#include "../hyinit.h"
#include "coredbg.h"

dbg_variable_t *dbg_variables = NULL;
mtx_t dbg_variables_mtx = MTX_DEFINE(); 

/*
 * dbg_get_variable(name)
 *
 * Returns the variable structure of a variable with name "name".
 *
 * REMEMBER: This function should be run with locked mutex dbg_variables_mtx!
 *
 * Return value:
 *	!= NULL		Pointer to that variable
 *	== NULL		Variable not found
 *
 */
dbg_variable_t* dbg_get_variable(const utf8_t *name)
{
	dbg_variable_t *l__vars = dbg_variables;
	
	if (name == NULL) return NULL;
	
	/* Search it */
	while (l__vars != NULL)
	{
		if (!str_compare(l__vars->name, name, 32))
			return l__vars;
			
		l__vars = l__vars->ls.n;
	}
	
	return NULL;
}

/*
 * dbg_export(name, value)
 *
 * Creates a variable "name" and sets its value to "value"
 * or changes the value of an existing variable "name" to "value".
 *
 * Return value:
 *	0	If successful
 *	1	If error
 *
 */
int dbg_export(const utf8_t *name, const utf8_t *value)
{
	dbg_variable_t *l__vars;
	
	if ((name == NULL) || (value == NULL)) return 1;
	
	mtx_lock(&dbg_variables_mtx, -1);
	l__vars = dbg_get_variable(name);
	
	/* Is it a new one? */
	if (l__vars == NULL)
	{
		/* Create it */
		l__vars = mem_alloc(sizeof(dbg_variable_t));
		if (l__vars == NULL)
		{
			mtx_unlock(&dbg_variables_mtx);
			
			dbg_isprintf("Can't allocate variable because of %i\n", *tls_errno);
			*tls_errno = 0;
			
			return 1;
		}
		
		/* Set its name / value */
		str_copy(l__vars->name, name, 32);
		str_copy(l__vars->value, value, 32);
		
		/* Add it to the list */
		lst_add(dbg_variables, l__vars);
		
		dbg_isprintf("Store new var to 0x%X, (n: 0x%X, v: 0x%X)\n", l__vars, l__vars->name, l__vars->value);
		
		mtx_unlock(&dbg_variables_mtx);
		return 0;
	}
	 else	/* It is an existing one */
	{
		/* Change its value */
		str_copy(l__vars->value, value, 32);
		
		dbg_isprintf("Store old var to 0x%X, (n: 0x%X, v: 0x%X)\n", l__vars, l__vars->name, l__vars->value);
		
		mtx_unlock(&dbg_variables_mtx);
		return 0;
	}
}

/*
 * dbg_get_value(name, buf, sz)
 *
 * Copies the content of the variable "name" to the buffer "buf" which has
 * a size of "sz" bytes.
 *
 * Return value:
 *	0	If successful (value -> buf)
 *	1	If error
 *
 */ 
int dbg_get_value(const utf8_t *name, utf8_t *buf, size_t sz)
{
	dbg_variable_t *l__vars;
	
	if ((buf == NULL) || (name == NULL)) return 1;
	
	mtx_lock(&dbg_variables_mtx, -1);
	l__vars = dbg_get_variable(name);
		
	/* Is it existing? */
	if (l__vars == NULL)
	{
		/* No. */
		
		mtx_unlock(&dbg_variables_mtx);
		return 1;	
	}
	 else
	{
		/* Yes. Copy it */
		if (sz > 32) sz = 32;
		
		str_copy(buf, l__vars->value, sz);
		
		mtx_unlock(&dbg_variables_mtx);
		return 0;
	}
}
