/*
 *
 * spxml.c
 *
 * (C)2006 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library').   
 *
 * Simple XML Parser
 *
 */
#include <hydrixos/types.h>
#include <hydrixos/tls.h>
#include <hydrixos/errno.h>
#include <hydrixos/mem.h>
#include <hydrixos/stdfun.h>
#include <hydrixos/spxml.h>
	
/* From which SPXMLEVENT_-number we can ignore events? */
#define SPXML_IGNORABLE_EVENTS		1
	
/* Table of possible SPXML evenets */
static struct
{
	int		type;		/* Type ID */
	const utf8_t*	begin;		/* Sequence begin */
	size_t		blen;		/* Size of sequence begin in bytes */
	const utf8_t*	end;		/* Sequence end */
	size_t		elen;		/* Size of sequence end in bytes */
}spxml_event_types[5] =
{
	{SPXMLEVENT_COMMENT, "<!--", 4, "-->", 3},
	{SPXMLEVENT_PROCESSING, "<?", 2, "?>", 2},
	{SPXMLEVENT_EMPTY_TAG, "<", 1, "/>", 2},
	{SPXMLEVENT_END_TAG, "</", 2, ">", 1},
	{SPXMLEVENT_BEGIN_TAG, "<", 1, ">", 1}
};
	
/* Table of convertable entity references */
#define SPXML_REFTABLE_LEN		5

static struct 
{
	const utf8_t*	ref;		/* The entity reference */
	size_t		ref_len;	/* Length of the reference */
	
	const utf8_t*	ch_replace;	/* The replacement */
	size_t		ch_len;		/* The len of the replacement */
}spxml_entity_reftable[SPXML_REFTABLE_LEN] =
	{
		{"&amp;", 5, "&", 1},
		{"&lt;", 4, "<", 1},
		{"&gt;", 4, ">", 1},
		{"&apos;", 6, "'", 1},
		{"&quot;", 6, "\"", 1}
	};
		
/*
 * spxml_replace_stdentities(text, max)
 *
 * Converts als standard entity references in "text" to normal
 * UTF-8 characters. The "text" will have at least a length of "len"
 * bytes.
 *
 * All entities are stored in the entity table. The referenced
 * entities may be only smaller than the entity reference.
 *
 * Return value:
 *	Copy of the input string "text" with converted entity references.
 */
utf8_t* spxml_replace_stdentities(const utf8_t *text, size_t len)
{
	const utf8_t *l__ptr = text;
	utf8_t *l__out = mem_alloc(len);
	if (l__out == NULL) return NULL;
	utf8_t *l__outptr = l__out;
	size_t l__len = len;
	
	if (text == NULL) 
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		return NULL;
	}
	
	do
	{
		if (*l__ptr == '&')
		{
			int l__i = SPXML_REFTABLE_LEN;

			/* Search the reference in the reference table */
			while (l__i --)
			{
				if (!str_compare(l__ptr, spxml_entity_reftable[l__i].ref, spxml_entity_reftable[l__i].ref_len))
				{
					str_copy(l__outptr, spxml_entity_reftable[l__i].ch_replace, spxml_entity_reftable[l__i].ch_len);
					
					l__len -= spxml_entity_reftable[l__i].ref_len - 1;
					
					l__outptr += spxml_entity_reftable[l__i].ch_len;
					l__ptr += spxml_entity_reftable[l__i].ref_len - 1;
					
					break;
				}
			}	
			
			/* Found sthg.? Continue, else just copy the entity reference */
			if (l__i > -1) continue;
		}
	
		/* Copy normal charracters */
		*l__outptr++ = *l__ptr;
	}while((*l__ptr ++) && (l__len --));

	return l__out;

}

/*
 * spxml_end_of_event(&val, str, len)
 *
 * Searches the end of the event "val" which is marked
 * by "str". In returns the end position in to the structure
 * val. The string "str" has a maximum length of "len"
 *
 *	1	End of event found
 *	0	End of event not found
 *
 */
static inline int spxml_end_of_event(spxml_event_t *val, const utf8_t *str, size_t len)
{
	utf8_t *l__end = str_find(val->position, val->len, str, len + 1);
	if (l__end == NULL) return 0;
	
	val->len = l__end - val->position;
	val->total_len = (l__end - val->position) + spxml_event_types[val->type].elen;
	
	return 1;
}

/*
 * spxml_probe_element(&val, type)
 *
 * Test if the event "val" is a SPXML element of type "type",
 * which normally begins with "begin" and ends with "end".
 * "begin" has a length of "blen" bytes and "end" a length
 * of "elen" bytes.
 * The length element of "val" (val->len) has to be specified
 * to run this function.
 *
 * Return Value:
 * 	1 	Element found, the datas will be passed to "val"
 *	0	Element not found
 *
 */
static inline int spxml_probe_element(spxml_event_t *val, int type)
{
	utf8_t *l__buf = mem_alloc(5);
	str_copy(l__buf, val->position, 4);
	l__buf[4] = 0;	
	mem_free(l__buf);
		
	if (!str_compare(val->position, spxml_event_types[type].begin, spxml_event_types[type].blen))
	{
		val->type = type;
		if (!spxml_end_of_event(val, spxml_event_types[type].end, spxml_event_types[type].elen)) return 0;
		
		val->content = val->position + spxml_event_types[type].blen;
		val->len -= spxml_event_types[type].blen;

		return 1;
	}
	 else
	{
		return 0;
	}
}

/*
 * spxml_is_whitespace(str, len)
 *
 * Test if the string "str" contains only 
 * whitespace up to "len".
 *
 * Return value:
 *	2	If it contains the 0 charracter
 *	1	If it contains only whitespaces
 *	0	Other charracters found
 */
static inline int spxml_is_whitespace(const utf8_t *str, size_t len)
{
	while (len --)
	{
		if ((*str != 13) && (*str != 10) && (*str != '\t') && (*str != ' ') && (*str != 0))
		{
			return 0;
		}
		
		if (*str == 0)
			return 2;
		
		str ++;
	}
	
	return (*str == 0) ? 2 : 1;
}

/*
 * spxml_next_event(xml, len, &evt)
 *
 * Searches the string "xml" for the next SPXML event and
 * returns informations about it. The string has a maximum
 * length of "len" bytes. The return value will be passed to
 * "evt".
 *
 */
 
static inline void spxml_next_event(const utf8_t *xml, size_t len, spxml_event_t *evt)
{
	evt->position = xml;
	
	/* Test parameters */
	if (evt == NULL) return;
	if ((xml == NULL) || (len == 0)) {evt->type = SPXMLEVENT_INVALID; return;}	
	
	/* End of file? */
	if (*xml == 0)
	{
		evt->len = 1;
		evt->type = SPXMLEVENT_EOF;
		evt->total_len = evt->len;
		evt->content = evt->position;		
		
		return;
	}
	
	/* No SPXML element? */
	if (*xml != '<')
	{
		/* Search for next element */
		const utf8_t *l__end = str_char(evt->position, '<', len);
		
		if (l__end == NULL)
		{
			evt->type = SPXMLEVENT_INVALID;
		}
		 else
		{
			evt->len = l__end - evt->position;
			evt->total_len = evt->len;
			evt->content = evt->position;
		}
		
		/* Is it a whitespace or EOF content? */
		int l__is_ws = spxml_is_whitespace(evt->position, evt->total_len);

		switch (l__is_ws)
		{
			case 1:
				evt->type = SPXMLEVENT_WHITESPACE;
				break;
			case 2:
				evt->type = SPXMLEVENT_EOF;
				break;
			default:
				evt->type =  SPXMLEVENT_DATA;
		}
			
		return;
	}
	
	evt->len = len;
	
	/* What kind of element do we have? */
	if (spxml_probe_element(evt, SPXMLEVENT_COMMENT)) return;
	if (spxml_probe_element(evt, SPXMLEVENT_PROCESSING)) return;
	if (spxml_probe_element(evt, SPXMLEVENT_END_TAG)) return;
	if (spxml_probe_element(evt, SPXMLEVENT_BEGIN_TAG)) 
	{
		/* Begin tag or empty tag? */
		
		if (evt->content[evt->len - 1] == '/')
		{
			evt->len -= 1;
			evt->type = SPXMLEVENT_EMPTY_TAG;
		}
				
		return;
	}
	
	evt->type = SPXMLEVENT_INVALID;
	
	return;
}

/*
 * spxml_create_tree(xml, len, node)
 *
 * Analyses the SPXML content "xml" which has the
 * length of "len" bytes. It creates a new
 * element "node" according to the first SPXML element
 * found in "xml". All sub-elements of the first
 * node will be add to the children list of "node".
 * If there is no sub-element, the function will
 * just configure "node" and exit.
 *
 * Return value:
 *	Pointer to the end of the SPXML element
 *	within "xml".
 *
 *	NULL		error
 *
 */
const utf8_t* spxml_create_tree(const utf8_t *xml, size_t len, spxml_node_t *node)
{
	if ((xml == NULL) || (len == 0) || (node == NULL))
	{
		*tls_errno = ERR_INVALID_ADDRESS;
		return NULL;
	}
		
	/*
	 * What kind of event do we have at first level?
	 *
	 */
	spxml_event_t l__event = {NULL, 0, NULL, 0, SPXMLEVENT_INVALID};
	
	/* Search until we find something useful */
	do
	{	
		spxml_next_event(xml, len, &l__event);
		
		/* Repositionate pointer */
		xml = l__event.position + l__event.total_len;
		len -= l__event.total_len;
		
		/* Nothing found yet */
		if (len <= 0)
		{
			*tls_errno = ERR_INVALID_ARGUMENT;
			return NULL;
		}
			
	}while((l__event.type <= SPXML_IGNORABLE_EVENTS) || (l__event.type == SPXMLEVENT_WHITESPACE));
	
	/* Error? */
	if ((l__event.type == SPXMLEVENT_EOF) || (l__event.type == SPXMLEVENT_INVALID))
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		return NULL;
	}
		
	/* Configure the node */
	node->type = l__event.type;
	node->tag = l__event.content;
	node->tag_len = l__event.len;
	node->children = NULL;		
	
	xml = l__event.position;
	
	/* Single element, just exit */
	if ((l__event.type == SPXMLEVENT_DATA) || (l__event.type == SPXMLEVENT_EMPTY_TAG))
	{
		return xml + l__event.total_len;
	}
	
	xml += l__event.total_len;
	
	/* Element with possible subelements */
	if (l__event.type == SPXMLEVENT_BEGIN_TAG)
	{
		/* Until you find the "ending element", add the other subelements to our childrens list */
		do
		{
			spxml_event_t l__subevent = {NULL, 0, NULL, 0, SPXMLEVENT_INVALID};
			
			spxml_next_event(xml, len, &l__subevent);
			
			if ((l__subevent.type == SPXMLEVENT_EOF) || (l__subevent.type == SPXMLEVENT_INVALID))
			{
				*tls_errno = ERR_INVALID_ARGUMENT;
				return NULL;	
			}			
			
			if (l__subevent.type == SPXMLEVENT_WHITESPACE)
			{
				len -= l__subevent.total_len;
				xml = l__subevent.position + l__subevent.total_len;
				continue;
			}
			
			if (l__subevent.type == SPXMLEVENT_END_TAG)
			{
				return l__subevent.position + l__subevent.total_len;
			}
			else
			{
				if (l__subevent.type > SPXML_IGNORABLE_EVENTS)
				{
					const utf8_t *l__oldxml = xml;
					
					spxml_node_t *l__subnode = mem_alloc(sizeof(spxml_node_t));
					
					/* Create a children node */
					xml = spxml_create_tree(xml, len, l__subnode);
					
					if (xml != NULL)
					{
						lst_add(node->children, l__subnode);
					}
					 else
					{
						*tls_errno = ERR_INVALID_ARGUMENT;
						return NULL;					
					}

					len = len - ((uintptr_t)l__oldxml - (uintptr_t)xml);
				}
				 else
				{
					xml = l__subevent.position + l__subevent.total_len;
					len -= l__subevent.total_len;
				}
								
			}
		}while(len > 0);
	}

	*tls_errno = ERR_INVALID_ARGUMENT;
	return NULL;	
}

/*
 * spxml_destroy_tree(node)
 *
 * Frees the tree datastructures below the node "node".
 * The node "node" itself wouldn't be freed.
 *
 */
void spxml_destroy_tree(spxml_node_t *node)
{
	if (node == NULL)
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		return;
	}
	
	spxml_node_t *l__node = node->children;
	
	while(l__node != NULL)
	{
		spxml_destroy_tree(l__node);
		mem_free(l__node);
		
		l__node = l__node->ls.n;
	};
	
	node->children = NULL;
	
	return;
}

/*
 * spxml_resolve_path(path, node)
 *
 * Resolves the simplified XPath "path" within 
 * the SPXML tree "node" and returns the element found.
 *
 */
spxml_node_t* spxml_resolve_path(const utf8_t *path, spxml_node_t *node)
{
	const utf8_t *l__pos = NULL;
	size_t l__plen = 0;
	
	/* Test parameters */
	if ((path == NULL) || (node == NULL))
	{
		*tls_errno = ERR_INVALID_ADDRESS;
		return NULL;
	}
	
	/* Invalid path */
	if (*path != '/')
	{
		*tls_errno = ERR_INVALID_ARGUMENT;
		return NULL;
	}
	 else
	{
		path ++;
	}
	
	/* Search it */
	do
	{
		l__pos = path;
		l__plen = 0;
		
		/* Get the length of the current path element */
		do
		{
			l__plen ++;
			if (*l__pos == '/') break;
			
		}while(*l__pos++);
		
		if (*l__pos == 0) continue;
		l__plen --;
		
		if ((!str_compare(node->tag, path, l__plen)) && (l__plen  == node->tag_len))
		{
			path += l__plen;
			
			/* End of string (1)? */
			if ((*path == '/') && (path[1] == 0))
			{
				return node;
			}
			
			/* End of string (2)? */
			if (*path == 0)
			{
				/* We've found everything we need */
				return node;
			}
			else
			{
				/* Search at next level */
				node = node->children;
				path ++;
			}
		}
		 else
		{
			node = node->ls.n;
		}
	
	}while(node != NULL);
	
	*tls_errno = ERR_INVALID_ARGUMENT;
	return NULL;
}
