/*
 *
 * spxml.h
 *
 * (C)2005 by Friedrich Gräter
 *
 * This file is distributed under the terms of
 * the GNU Lesser General Public License, Version 2. You
 * should have received a copy of this license (e.g. in 
 * the file 'copying.library'). 
 *
 * Simple SPXML parser
 *
 */ 
#ifndef _SPSPXML_H
#define _SPSPXML_H

#include <hydrixos/types.h>
#include <hydrixos/list.h>
#include <hydrixos/mutex.h>
#include <hydrixos/hymk.h>

/* Structure of a parsed SPXML node */
typedef struct spxml_node_st
{
	int		type;		/* SPXML Event type (SPXMLEVENT_) */
	
	const utf8_t*	tag;		/* Pointer to the SPXML tag (without < >) */
	size_t		tag_len;	/* Size of the SPXML tag (only the tag) */
	
	struct spxml_node_st	*children;	/* List of children elements */
	list_t			ls;		/* List of elements of the same parent */
}spxml_node_t;

/* Structure of a SPXML event (event = something found within the text during parsing) */
typedef struct
{
	const utf8_t*	position;	/* Pointer to the event begin */
	size_t		total_len;	/* Total size of the event */
	
	const utf8_t*	content;	/* Pointer to the begin of the event's content */
	size_t		len;		/* Size of the event's content */
	
	
	int		type;		/* Type (SPXMLEVENT_*-Macros) */
}spxml_event_t;

	/* <!-- Comments --> */
#define SPXMLEVENT_COMMENT		0
	/* <? Processing Informations ?> */
#define SPXMLEVENT_PROCESSING		1
	/* <Empty Tag/> */
#define SPXMLEVENT_EMPTY_TAG		2
	/* </End_Tag> */
#define SPXMLEVENT_END_TAG		3
	/* <Begin_Tag> */
#define SPXMLEVENT_BEGIN_TAG		4	
	/* Data */
#define SPXMLEVENT_DATA			5	
	/* Element contains only whitespaces (LF, CR, TAB, SPACE) */
#define SPXMLEVENT_WHITESPACE		6
	/* EOF */
#define SPXMLEVENT_EOF			7
	/* Invalid Element */
#define SPXMLEVENT_INVALID		8
	
/* Functions */
utf8_t* spxml_replace_stdentities(const utf8_t *text, size_t len);

const utf8_t* spxml_create_tree(const utf8_t *xml, size_t len, spxml_node_t *node);
void spxml_destroy_tree(spxml_node_t *node);

spxml_node_t* spxml_resolve_path(const utf8_t *path, spxml_node_t *node);

#endif
