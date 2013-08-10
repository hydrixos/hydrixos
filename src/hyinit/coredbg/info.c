/*
 *
 * info.c
 *
 * (C)2006 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying').   
 *
 * System information
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

typedef struct
{
	const utf8_t 	*name;		/* Name of the table entry */
	unsigned	position;	/* Table position (Array entry number of uint32_t[]!) */
}dbg_info_nametable_t;

#define DBG_INFO_MKNAME(___p, ___t)	{#___p, ___t##___p}

#define DBG_INFO_MKMAIN(___p)		DBG_INFO_MKNAME(___p, MAININFO_)
#define DBG_INFO_MKPROC(___p)		DBG_INFO_MKNAME(___p, PRCTAB_)
#define DBG_INFO_MKTHRD(___p)		DBG_INFO_MKNAME(___p, THRTAB_)

/* Table of names for "sysinfo" */
#define DBG_SYSINFOTAB_SIZE		20

static const dbg_info_nametable_t   dbg_sysinfo_tab[DBG_SYSINFOTAB_SIZE] =
{
	DBG_INFO_MKMAIN(CURRENT_PROCESS),
	DBG_INFO_MKMAIN(CURRENT_THREAD),	
	DBG_INFO_MKMAIN(PROC_TABLE_ENTRY),
	DBG_INFO_MKMAIN(THRD_TABLE_ENTRY),	
	DBG_INFO_MKMAIN(KERNEL_MAJOR),
	DBG_INFO_MKMAIN(KERNEL_MINOR),	
	DBG_INFO_MKMAIN(KERNEL_REVISION),
	DBG_INFO_MKMAIN(RTC_COUNTER_LOW),
	DBG_INFO_MKMAIN(RTC_COUNTER_HIGH),	
	DBG_INFO_MKMAIN(CPU_ID_CODE),
	DBG_INFO_MKMAIN(PAGE_SIZE),	
	DBG_INFO_MKMAIN(MAX_PAGE_OPERATION),

	DBG_INFO_MKMAIN(X86_CPU_NAME_PART_1),	
	DBG_INFO_MKMAIN(X86_CPU_NAME_PART_2),
	DBG_INFO_MKMAIN(X86_CPU_NAME_PART_3),
	DBG_INFO_MKMAIN(X86_CPU_TYPE),
	DBG_INFO_MKMAIN(X86_CPU_FAMILY),		
	DBG_INFO_MKMAIN(X86_CPU_MODEL),	
	DBG_INFO_MKMAIN(X86_CPU_STEPPING),
	DBG_INFO_MKMAIN(X86_RAM_SIZE)
};

/* Table of names for "proc" */
#define DBG_PROCINFOTAB_SIZE		14

static const dbg_info_nametable_t   dbg_procinfo_tab[DBG_PROCINFOTAB_SIZE] =
{
	DBG_INFO_MKPROC(IS_USED),
	DBG_INFO_MKPROC(IS_DEFUNCT),
	DBG_INFO_MKPROC(SID),
	DBG_INFO_MKPROC(CONTROLLER_THREAD_SID),
	DBG_INFO_MKPROC(CONTROLLER_THREAD_DESCR),
	DBG_INFO_MKPROC(PAGE_COUNT),
	DBG_INFO_MKPROC(IO_ACCESS_RIGHTS),
	DBG_INFO_MKPROC(PAGEDIR_PHYSICAL_ADDR),
	DBG_INFO_MKPROC(IS_ROOT),
	DBG_INFO_MKPROC(IS_PAGED),
	DBG_INFO_MKPROC(THREAD_COUNT),
	DBG_INFO_MKPROC(THREAD_LIST_BEGIN),
	DBG_INFO_MKPROC(UNIQUE_ID),

	DBG_INFO_MKPROC(X86_MMTABLE)
};

/* Table of names for "thrd" */
#define DBG_THRDINFOTAB_SIZE		41

static const dbg_info_nametable_t   dbg_thrdinfo_tab[DBG_THRDINFOTAB_SIZE] =
{
	DBG_INFO_MKTHRD(IS_USED),
	DBG_INFO_MKTHRD(SID),
	DBG_INFO_MKTHRD(PROCESS_SID),
	DBG_INFO_MKTHRD(SYNC_SID),
	DBG_INFO_MKTHRD(MEMORY_OP_SID),
	DBG_INFO_MKTHRD(MEMORY_OP_DESTADR),
	DBG_INFO_MKTHRD(MEMORY_OP_MAXSIZE),
	DBG_INFO_MKTHRD(MEMORY_OP_ALLOWED),
	DBG_INFO_MKTHRD(KERNEL_STACK_ADDRESS),
	DBG_INFO_MKTHRD(PROCESS_DESCR),
	DBG_INFO_MKTHRD(THRSTAT_FLAGS),
	DBG_INFO_MKTHRD(IRQ_RECV_NUMBER),
	DBG_INFO_MKTHRD(FREEZE_COUNTER),
	DBG_INFO_MKTHRD(RUNQUEUE_PREV),
	DBG_INFO_MKTHRD(RUNQUEUE_NEXT),
	DBG_INFO_MKTHRD(SOFTINT_LISTENER_SID),
	DBG_INFO_MKTHRD(EFFECTIVE_PRIORITY),
	DBG_INFO_MKTHRD(STATIC_PRIORITY),
	DBG_INFO_MKTHRD(SCHEDULING_CLASS),
	DBG_INFO_MKTHRD(TIMEOUT_LOW),
	DBG_INFO_MKTHRD(TIMEOUT_HIGH),

	DBG_INFO_MKTHRD(LAST_EXCPT_NUMBER),
	DBG_INFO_MKTHRD(LAST_EXCPT_NR_PLATTFORM),
	DBG_INFO_MKTHRD(LAST_EXCPT_ADDRESS),
	DBG_INFO_MKTHRD(LAST_EXCPT_ERROR_CODE),
	DBG_INFO_MKTHRD(PAGEFAULT_DESCRIPTOR),
	DBG_INFO_MKTHRD(PAGEFAULT_LINEAR_ADDRESS),

	DBG_INFO_MKTHRD(PREV_THREAD_OF_PROC),
	DBG_INFO_MKTHRD(NEXT_THREAD_OF_PROC),

	DBG_INFO_MKTHRD(RECEIVED_SOFTINT),
	DBG_INFO_MKTHRD(RECV_LISTEN_TO),

	DBG_INFO_MKTHRD(UNIQUE_ID),

	DBG_INFO_MKTHRD(CUR_SYNC_QUEUE_PREV),
	DBG_INFO_MKTHRD(CUR_SYNC_QUEUE_NEXT),
	DBG_INFO_MKTHRD(OWN_SYNC_QUEUE_BEGIN),
	DBG_INFO_MKTHRD(TIMEOUT_QUEUE_PREV),
	DBG_INFO_MKTHRD(TIMEOUT_QUEUE_NEXT),

	DBG_INFO_MKTHRD(X86_KERNEL_POINTER),

	DBG_INFO_MKTHRD(X86_TLS_PHYS_ADDRESS),

	DBG_INFO_MKTHRD(X86_FPU_STACK),

	DBG_INFO_MKTHRD(LOCAL_STORAGE_BEGIN)
};

/*
 * dbg_get_info_num(cmd, table, tab_len)
 *
 * Get the number of the wanted table entry of a system information table
 * for the comand "cmd". The names for the selected table can be resolved
 * using the name table "table" which has "len" entries.
 *
 * Return value:
 *	>=0	The Number
 *	< 0	Error
 *
 */
static int dbg_get_info_num(const utf8_t *cmd, const dbg_info_nametable_t *table, int len)
{
	dbg_shell_t *l__shell = *dbg_tls_shellptr;
	int l__infopar;
	uint32_t l__num;
	
	/* Less than 2 parameters? */
	if (l__shell->n_pars < 3)
	{
		dbg_iprintf(l__shell->terminal, "Missing parameter. Try \"help %s\" for more information.\n", cmd);
		return -1;
	}	
	
	/* Is the parameter a name (-n)? */
	l__infopar = dbg_test_par(0, "-n");
	if (l__infopar > -1)	
	{
		/* Is there a name parameter? */
		if (((unsigned)l__infopar + 1) >= l__shell->n_pars)
		{
			dbg_iprintf(l__shell->terminal, "Missing name parameter. Try \"help %s\" for more information.\n", cmd);
			return -1;
		}
		
		/* Load it and try to find it within the name table */
		int l__n = len;
		
		while (l__n --)
		{
			/* Found it? */
			if (!str_compare(table[l__n].name, l__shell->pars[l__infopar + 1], 128))
			{
				l__num = table[l__n].position;
				break;
			}
		}
	}

	/* Is the parameter a number (-a)? */
	if (l__infopar <= -1)
	{
		l__infopar = dbg_test_par(0, "-a");
		if (l__infopar > -1)
		{
			/* Is there a name parameter? */
			if (((unsigned)l__infopar + 1) >= l__shell->n_pars)
			{
				dbg_iprintf(l__shell->terminal, "Missing number parameter. Try \"help %s\" for more information.\n", cmd);
				return -1;
			}

			/* Try to load the number */
			if (dbglib_atoul(l__shell->pars[l__infopar +1], &l__num, 10))
			{
				dbg_iprintf(l__shell->terminal, "Invalid number - %s.\n", l__shell->pars[l__infopar + 1]);
				return -1;
			}
		}
	 	 else
		{
			dbg_iprintf(l__shell->terminal, "Missing name or number parameter. Try \"help %s\" for more information.\n", cmd);
			return -1;
		}
	}
	
	return l__num;
}

/*
 * dbg_print_infotab(table, len)
 *
 * Prints all entries of the information page name table "table"
 * which has a length of "len" bytes, if the parameter "-s" was set.
 *
 * Return value:
 *	1	List printed
 *	0	No -s command
 *
 */
static int dbg_print_infotab(const dbg_info_nametable_t *table, int len)
{
	dbg_shell_t *l__shell = *dbg_tls_shellptr;
	
	if (dbg_test_par(0, "-s") > -1)	
	{	
		dbg_iprintf(l__shell->terminal, "Valid names and numbers of the main info page:\n\n");
		
		for (int l__n = 0; l__n < len; l__n ++)
		{
			dbg_iprintf(l__shell->terminal, "\t%.30s\t\t\t%i\n", table[l__n].name, table[l__n].position);
			if (((l__n % 12) == 0) && (l__n > 0)) dbg_pause(l__shell->terminal);
		}	
		
		return 1;
	}	
	
	return 0;
}

/*
 * dbg_sh_sysinfo
 *
 * Ouput informations from the main info page.
 *
 * Usage:
 *      sysinfo {-n <name>|-a <number>} [-s]
 *
 *		-s		Write a list of all valid names and numbers
 *		-n <name>	The name of the sysinfo table entry
 *		-a <number>	The entry number within the sysinfo table (Dec).
 *				You can specify either -n or -a.
 *
 */
int dbg_sh_sysinfo(void)
{
	dbg_shell_t *l__shell = *dbg_tls_shellptr;
	int l__num = -1;
		
	/* Create a list of all valid names and numbers (-s)? */
	if (dbg_print_infotab(dbg_sysinfo_tab, DBG_SYSINFOTAB_SIZE))
		return 0;
		
	/* Get the entry number */
	l__num = dbg_get_info_num("sysinfo", dbg_sysinfo_tab, DBG_SYSINFOTAB_SIZE);
		
	/* Is it a valid entry? */
	if ((l__num < 0) || (l__num >= 1024))
	{
		dbg_iprintf(l__shell->terminal, "Invalid name or number parameter.\n");
		return -1;
	}
	
	/* Write the value */
	dbg_iprintf(l__shell->terminal, "SYSINFO(0x%X) => Value: 0x%X\n", l__num, hysys_info_read(l__num));
	
	return 0;
}

/*
 * dbg_sh_proc
 *
 * Write informations about a process to the terminal
 *
 * Usage:
 *     proc {-n <name>|-a <number>} [-s] [-c <sid>]
 *
 *		-s		Write a list of all valid names and numbers
 *		-n <name>	The name of the proc table entry
 *		-a <number>	The entry number within the proc table (Dec).
 *				You can specify either -n or -a.
 *		-c <sid>	The SID of a process or thread which informations
 *				should be displayed. If the SID of a thread is given
 *				the function will display the informations of its
 *				process. If no SID is given the informations about
 *				the client's process will be displayed.
 *
 */
int dbg_sh_proc(void)
{
	dbg_shell_t *l__shell = *dbg_tls_shellptr;
	int l__num = -1;
	sid_t l__sid;
		
	/* Create a list of all valid names and numbers (-s)? */
	if (dbg_print_infotab(dbg_procinfo_tab, DBG_PROCINFOTAB_SIZE))
		return 0;
		
	/* Get a valid SID */
	l__sid = dbg_get_sidpar("proc", SIDTYPE_PROCESS);
	if (l__sid == SID_INVALID) 
		return -1;
		
	/* Is it a thread SID? */
	if (l__sid & SIDTYPE_THREAD)
	{
		sid_t l__tmpsid = hysys_thrtab_read(l__sid, THRTAB_PROCESS_SID);
		
		/* Invalid SID? */
		if ((l__tmpsid & SIDTYPE_PROCESS) != SIDTYPE_PROCESS)
		{
			dbg_iprintf(l__shell->terminal, "Can't get a valid process sid from 0x%X. Retreiving 0x%X.\n", l__sid, l__tmpsid);
			return -1;
		}
		
		l__sid = l__tmpsid;
	}
		
	/* Get the entry number */
	l__num = dbg_get_info_num("proc", dbg_procinfo_tab, DBG_SYSINFOTAB_SIZE);
		
	/* Is it a valid entry? */
	if ((l__num < 0) || (l__num >= 2048))
	{
		dbg_iprintf(l__shell->terminal, "Invalid name or number parameter.\n");
		return -1;
	}
	
	/* Write the value */
	dbg_iprintf(l__shell->terminal, "PROCESS(0x%X, SID 0x%X) => Value: 0x%X\n", l__num, l__sid, hysys_prctab_read(l__sid, l__num));
	
	return 0;
}

/*
 * dbg_sh_thrd
 *
 * Write informations about a thread to the terminal
 *
 * Usage:
 *     thrd {-n <name>|-a <number>} [-s] [-c <sid>]
 *
 *		-s		Write a list of all valid names and numbers
 *		-n <name>	The name of the thread table entry
 *		-a <number>	The entry number within the thread table (Dec).
 *				You can specify either -n or -a.
 *		-c <sid>	The SID of a thread which informations
 *				should be displayed. If no SID is given the
 *				SID of the current client will be used.
 *
 */
int dbg_sh_thrd(void)
{
	dbg_shell_t *l__shell = *dbg_tls_shellptr;
	int l__num = -1;
	sid_t l__sid;
		
	/* Create a list of all valid names and numbers (-s)? */
	if (dbg_print_infotab(dbg_thrdinfo_tab, DBG_THRDINFOTAB_SIZE))
		return 0;
		
	/* Get a valid SID */
	l__sid = dbg_get_sidpar("thrd", SIDTYPE_THREAD);
	if (l__sid == SID_INVALID) 
		return -1;
		
	/* Get the entry number */
	l__num = dbg_get_info_num("thrd", dbg_thrdinfo_tab, DBG_THRDINFOTAB_SIZE);
		
	/* Is it a valid entry? */
	if ((l__num < 0) || (l__num >= 2048))
	{
		dbg_iprintf(l__shell->terminal, "Invalid name or number parameter.\n");
		return -1;
	}
	
	/* Write the value */
	dbg_iprintf(l__shell->terminal, "THREAD(0x%X, SID 0x%X) => Value: 0x%X\n", l__num, l__sid, hysys_thrtab_read(l__sid, l__num));
	
	return 0;
}
