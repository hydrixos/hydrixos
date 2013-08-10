/*
 *
 * meminit.c
 *
 * (C)2004 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying'). 
 *
 * Memory initialization
 *
 */
#include <hydrixos/types.h>
#include <stdio.h>
#include <setup.h>
#include <mem.h>


/* Memory size informations; Initialized in kmod_init() */
size_t total_mem_size = 0;	/* Total size of RAM (lower and upper) */
size_t normal_mem_free = 0;	/* Size of free normal zone RAM */
size_t high_mem_free = 0;	/* Size of free high zone RAM */
uintptr_t mem_start = 0;	/* Start of the freely usable memory */

/* Structure of the middle memory area */
uint32_t *ikp_start = NULL;	/* Start address of the IKP */
size_t ikp_size = 0;		/* Size of the IKP (should be 1 MiB + 4096 KiB) */

memory_zone_t	normal_zone;	/* Descriptor of the normal zone */
memory_zone_t	high_zone;	/* Descriptor of the high zone */

/* Page buffer table */
uintptr_t page_buf_table_start = 0;	/* Start address of the PBT */
size_t page_buf_table_sz = 0;		/* Size of the PBT */
pbtab_t* page_buf_table = NULL;		/* Pointer to the PBT */

/* Page share table */
uintptr_t page_share_table_start = 0;		/* Start address of the PST */
long page_share_table_count = 0;		/* Count of entries in the PST */
size_t page_share_table_sz = 0;			/* Size of the PST */
sidlist_t *page_share_table_free = NULL;	/* Pointer to the PST (free entries list) */

/* Page buffer */
uintptr_t page_buffer_start = 0;	/* Start address of the page buffer */
void* page_buffer = NULL;		/* Pointer to the begining of the page buffer */
size_t page_buffer_sz = 0;		/* Size of page buffer */

/*
 * kmem_dump_tables
 *
 */
static void kmem_dump_tables(void)
{
	kprintf("\nDump of kernel tables:\n");
	kprintf("----------------------\n");
	kprintf("Start of useable memory: 0x%X\n", mem_start);
	kprintf("IKP Start: 0x%X - IKP Size %i KiB\n", 
		(uintptr_t)ikp_start, 
		ikp_size / 1024
	       );
	kprintf("PBT Start: 0x%X - PBT Size %i KiB\n", 
		page_buf_table_start, 
		page_buf_table_sz / 1024
	       );
	kprintf("PST Start: 0x%X - PST Size %i KiB (%i Entries)\n", 
		page_share_table_start, 
		page_share_table_sz / 1024,
		page_share_table_count
	       );
	kprintf("NZS Start: 0x%X - NZS Size %i KiB\n", 
		normal_zone.frame_stack_start, 
		normal_zone.frame_stack_sz / 1024
	       );	
	kprintf("HZS Start: 0x%X - HZS Size %i KiB\n", 
		high_zone.frame_stack_start, 
		high_zone.frame_stack_sz / 1024
	       );	
	kprintf("PBUF Start: 0x%X - PBUF Size %i KiB\n", 
		(uintptr_t)page_buffer, 
		page_buffer_sz / 1024
	       );		         
	kprintf("----------------------\n\n");
}

/*
 * kmem_init_tables
 *
 * Initializes the kernel managment tables IKP, PBT, FMT
 * and the page frame buffer.
 *
 */
int kmem_init_tables(void)
{
	long l__n = 0;
	/*
	 * Initalize the table pointers and size informations
	 *
	 */
	ikp_start = (void*)(uintptr_t)((mem_start & (~0xFFFU)) + 4096);
	ikp_size = 1024*1024 + 4096;
	normal_mem_free -= ikp_size + 4096;
	
	page_buf_table_start = (((uintptr_t)ikp_start) + ikp_size);
	page_buf_table = (void*)page_buf_table_start;
	page_buf_table_sz =   ((total_mem_size - (1024 * 1024)) / 4096) 
			       * sizeof(pbtab_t);
			       
	page_share_table_start = (page_buf_table_start + page_buf_table_sz);
	page_share_table_free = (void*)page_share_table_start;
	page_share_table_count = (((total_mem_size - (1024 * 1024)) / 4096) * PST_PERCENT) / 100;
	page_share_table_sz = page_share_table_count * sizeof(sidlist_t);
			      
			      	
		
	/* Normal zone stack */
	
	normal_zone.frame_stack_start =  page_share_table_start 
				       + page_share_table_sz;
	normal_zone.frame_stack_count =   normal_mem_free / 4096;
	normal_zone.frame_stack_sz = normal_zone.frame_stack_count * sizeof(uint32_t);
	normal_zone.frame_stack_ptr = (void*)(  normal_zone.frame_stack_start 
					      + normal_zone.frame_stack_sz
					     );
	
	/* High zone stack */
	high_zone.frame_stack_start =     normal_zone.frame_stack_start 
					+ normal_zone.frame_stack_sz;
	high_zone.frame_stack_count = high_mem_free / 4096;
	high_zone.frame_stack_sz = high_zone.frame_stack_count * sizeof(uint32_t);
	high_zone.frame_stack_ptr = (void*)(  high_zone.frame_stack_start 
					    + high_zone.frame_stack_sz
					   );
	
	/* Actualize memory datas */	
	normal_mem_free -=   normal_zone.frame_stack_sz 
		          + high_zone.frame_stack_sz
		          + page_buf_table_sz
			  + page_share_table_sz
		          ;
	normal_mem_free &= (~0xFFFU);
	normal_mem_free -= 4096;
	
	normal_zone.frame_stack_count = (normal_mem_free / 4096);
	
	page_buffer_start = 
			((    (  high_zone.frame_stack_start 
			       + high_zone.frame_stack_sz
			      )
			   &  (~0xFFFU)
			 ) 
			 + 4096
			);
	page_buffer = (void*)page_buffer_start;
	page_buffer_sz = normal_mem_free + high_mem_free;

	/*
	 * Initialize the PBT and PST
	 *
	 */
	
	/* Initialize the PBT. Set each entry to "unused". */
	l__n = page_buf_table_sz / sizeof(pbtab_t);
	while(l__n --)
	{
		page_buf_table[l__n].usage = 0;
		page_buf_table[l__n].owner.single.pid = 0;
		page_buf_table[l__n].owner.single.u_adr = 0;
	}
	
	/* Initialize the PST */
	for (l__n = 0;
	     l__n < (page_share_table_count - 1);
	     l__n ++
	    )
	{
		page_share_table_free[l__n].user.pid = 0;
		page_share_table_free[l__n].user.u_adr = 0;
		page_share_table_free[l__n].nxt = &page_share_table_free[l__n + 1];
	}
	page_share_table_free[l__n -1].nxt = NULL;
	
	/*
	 * Initialize the Normal / High Zone page frame stacks
	 *
	 */
	/* Initialize the normal zone */
	l__n = normal_zone.frame_stack_count;
	uint32_t l__adr = (uintptr_t)page_buffer;
	
	while (l__n --)
	{
		normal_zone.frame_stack_ptr --;
		*(normal_zone.frame_stack_ptr) = l__adr;
		l__adr += 4096;
	}
	
	
	/* Initialize the high zone */
	l__n = high_zone.frame_stack_count;
	
	while (l__n --)
	{
		high_zone.frame_stack_ptr --;
		*(high_zone.frame_stack_ptr) = l__adr;
		l__adr += 4096;
	}	
	
	#ifdef DEBUG_MODE
		kmem_dump_tables();
	#endif

	/*
	 * Initialize the IKP and exit
	 *
	 */
	return kmem_init_krnl_spc();
}

