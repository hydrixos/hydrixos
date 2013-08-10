/*
 *
 * list.h
 *
 * (C)2005 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library').  
 *
 * HydrixOS API: Double linked lists
 *
 */
#ifndef _LIST_H
#define _LIST_H

#include <hydrixos/types.h>

/*
 * list_t datatype
 *
 * This datatype can be used to build linked list
 * using the lst_* and genlst_*-Macros. 
 *
 */ 
typedef struct {
	void		*p;		/* Previous element of the list */
	void		*n;		/* Next element of the list */	
}list_t;

/*
 * genlst_init(element, field) == (___list, ___le)
 *
 * Initializes an list-element 'element' to NULL, so
 * it can be used as the initializing element of a 
 * linked list. The informations needed for the linked 
 * list are saved in the structure element 'field' as
 * a data structure of the type "list_t"
 *
 * Normally the informations about the linked list are
 * stored in the element "ls". So you will rather use
 * the lst_init-Macro.
 *
 */ 
#define genlst_init(___list, ___le)	{(___list)->___le.n = NULL; (___list)->___le.p = NULL;}

/*
 * genlst_add(list, field, element) == (___list, ___le, ___element)
 *
 * Adds the structure 'element' to the end of a linked 
 * list represented by one of its members called 'list'.
 * The informations needed for the linkd list in 
 * "element" are stored in the structure element 
 * 'field' as a data structure of the type "list_t".
 *
 * All members of the linked list need to have the
 * same type.
 *
 * Normally the informations about the linked list are
 * stored in the element "ls". So you will rather use
 * the lst_add-Macro.
 * 
 */
#define genlst_add(___list, ___le, ___element) \
			 	{\
					typeof(___list) ___t = (___list);\
					\
					if ((___list) == NULL)\
					{\
						___list = ___element;\
						genlst_init(___list, ___le);\
					}\
					 else\
					{\
						while ((___t)->___le.n != NULL)\
						{\
							(___t) = (typeof(___list))((___t)->___le.n);\
						}\
						\
						(___t)->___le.n = ___element;\
						(___element)->___le.n = NULL;\
						(___element)->___le.p = (___t);\
					}\
				}
				
/*
 * genlst_del(element, field) == (___element, ___le)
 *
 * Removes the structure 'element' from its current linked
 * list. The informations about the linked list are stored
 * in the structure element 'field' as a data structure of
 * the type "list_t".
 *
 *
 * All members of the linked list need to have the
 * same type.
 *
 * Normally the informations about the linked list are
 * stored in the element "ls". So you will rather use
 * the lst_del-Macro.
 * 
 */		
#define genlst_del(___element, ___le) \
				({\
					typeof(___element) ___t = NULL;\
					typeof(___element) ___o = NULL;\
					\
					if ((___element)->___le.n != NULL)\
					{\
						(___t) = (typeof(___element))((___element)->___le.n);\
						(___o) = (typeof(___element))((___element)->___le.p);\
						(___t)->___le.p = (___element)->___le.p;\
						if (___o != NULL)\
							(___o)->___le.n = ___t;\
					}\
					 else if ((___element)->___le.p != NULL)\
					{\
						(___t) = (typeof(___element))((___element)->___le.p);\
						(___o) = (typeof(___element))((___element)->___le.n);\
						(___t)->___le.n = (___element)->___le.n;\
						if (___o != NULL)\
							(___o)->___le.p = ___t;\
					}\
					\
					(___element)->___le.n = NULL;\
					(___element)->___le.p = NULL;\
					\
					___t;\
				})
#define genlst_dellst(___list, ___element, ___le)	({if (___element == ___list) ___list = ___element->___le.n; genlst_del(___element, ___le);})

#define genlst_next(___element, ___le)	({typeof(___element) ___t = (___element)->___le.n; ___t;})				
#define genlst_prev(___element, ___le)	({typeof(___element) ___t = (___element)->___le.p; ___t;})
				
/*
 * Normally we use the 'lst_*'-Macros. 
 * They are the same thing like the 'genlst_*'-Macros,
 * but they use the standard-Element 'ls' for the information
 * block of the list.
 *
 */
#define lst_init(___list)		genlst_init(___list, ls)
#define lst_add(___list, ___element)	genlst_add(___list, ls, ___element)
#define lst_del(___element)		genlst_del(___element, ls)
#define lst_dellst(___list, ___element)	genlst_dellst(___list, ___element, ls)
#define lst_next(___element)		genlst_next(___element, ls)
#define lst_prev(___element)		genlst_prev(___element, ls)			
				
#endif


