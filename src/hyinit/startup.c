/*
 *
 * startup.c
 *
 * (C)2005 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying'). 
 *
 * Startup code of hyInit
 *
 */
#include <hydrixos/types.h> 
#include <hydrixos/errno.h> 
#include <hydrixos/hymk.h> 
#include <hydrixos/tls.h>
#include <hydrixos/stdfun.h>
#include <hydrixos/blthr.h>

#include <hydrixos/system.h>
#include <hydrixos/mem.h>

#include <hydrixos/spxml.h>

#include <coredbg/cdebug.h>
  
#include "hyinit.h"  

sid_t initproc_debugger_sid = 0;


/*
 * spxml_display_tree(node, nested)
 *
 * Displays all elements of the SPXML tree "node". Sub-elements
 * will be shown nested by "nested" tabs.
 *
 *
 */
static void spxml_display_tree(spxml_node_t *node, unsigned nested)
{
	spxml_node_t *l__children = node->children;
	utf8_t *l__neststr = mem_alloc(nested + 1);
	int l__i = nested;
	
	while (l__i --)
	{
		l__neststr[l__i] = '\t';
	}
	
	l__neststr[nested] = 0;
	
	utf8_t *l__buf = mem_alloc(node->tag_len + 1);
	str_copy(l__buf, node->tag, node->tag_len);
	l__buf[node->tag_len] = 0;
			
	if (node->type == SPXMLEVENT_BEGIN_TAG)
	{
		dc_printf("%sBEGIN: %i %s\n", l__neststr, node->type, l__buf);	
		
		while (l__children != NULL)
		{
			spxml_display_tree(l__children, nested + 1);
		
			l__children = l__children->ls.n;
		}
		dc_printf("%sEND: %i %s\n", l__neststr, node->type, l__buf);	
	}
	 else
	{
		dc_printf("%sSINGLE: %i %s\n", l__neststr, node->type, l__buf);	
	}	
	
	mem_free(l__buf);
	mem_free(l__neststr);
	
	return;
}

/***********************************/
static int testparser()
{
	const utf8_t *l__xml = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><enzyklopaedie>  <titel>Wikipedia Städteverzeichnis</titel>     <eintrag>          <stichwort>Genf</stichwort>          <eintragstext>Genf ist der Sitz von...</eintragstext>     </eintrag>     <eintrag>          <stichwort>Köln</stichwort>          <eintragstext>Köln ist eine <Stadt/>, die ...</eintragstext>     </eintrag></enzyklopaedie>";
		
	size_t l__len = str_len(l__xml, 2000);
		
	spxml_node_t l__node;
	l__node.children = NULL;
	 
	const utf8_t *l__v = spxml_create_tree(l__xml, l__len, &l__node);
	if (l__v == NULL) 
	{
		dc_printf("Invalid SPXML!\n"); 
		return 1;
	}
	 else
	{
		spxml_display_tree(&l__node, 0);
	}
	
	dc_printf("\n\nThis was a small testing of a simple XML parser running on HydrixOS.\n\n");
	
	spxml_destroy_tree(&l__node);
	
	return 0;
}

void sub_thread(thread_t *thr);
int x = 0;
void sub_thread(thread_t *thr)
{
	/* Test the debugger */
	cdbg_connect();

	dc_printf("Welcome to Thread 0x%X! I've a stack of %i KiB size.\n\n", thr->thread_sid, thr->stack_sz / 1024);
	dc_printf("This program is stupid. It just replies what you've typed.\n");
	dc_printf("There are two small commands:\n");
	dc_printf("\t* fork\tTo duplicate this thread on a second terminal.\n");
	dc_printf("\t* xml\tTo parse a simple XML-File\n");
	dc_printf("\t* blink\tJust a stupid multi-tasking demo\n");
	dc_printf("\t* fblink\tJust a stupid performance demo\n");

	while(1) 
	{
		utf8_t l__buf[100];
			
		dc_set_termcolor(DBGCOL_RED);
		dc_gets(99, l__buf);
		dc_set_termcolor(DBGCOL_YELLOW);
		if (!str_compare(l__buf, "xml", 4))
		{
			testparser();
		}
		 else if (!str_compare(l__buf, "fork", 5))
		{
			blthr_awake(blthr_create(&sub_thread, 8192));		
		}
		 else if (!str_compare(l__buf, "blink", 6))
		{
			long l__c = 0;
			while(1) {dc_printf("%i\n", l__c++); blthr_yield(0);}
		}
		 else if (!str_compare(l__buf, "fblink", 7))
		{
			long l__c = 0;
			while(1) {dc_printf("%i\n", l__c++);}
		}
		{
			dc_printf("%s\n", l__buf);
		}
	}

}

/*
 * init_main
 *
 * Startup code of the init application. This function will
 * start up any subprocess of init.
 *
 */
int init_main(void)
{
	iprintf("hyInit v 0.0.3 started.\n");
	
	iprintf("Starting the low-level debugger...\n");
	/* Starting the debugger */
	initproc_debugger_sid = init_fork(INITPROC_DEBUGGER);
	
	blthr_awake(blthr_create(&sub_thread, 8192));
	
	/* Freeze forever. */
	while(1) blthr_freeze(*tls_my_thread);
	
	return 0;
}
