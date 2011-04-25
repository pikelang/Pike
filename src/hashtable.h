/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "global.h"

#define AVERAGE_HASH_LENGTH 16
#define NEW_HASHTABLE_SIZE 4

#ifndef STRUCT_HASH_ENTRY_DECLARED
#define STRUCT_HASH_ENTRY_DECLARED
#endif
struct hash_entry
{
  struct hash_entry *next;
  struct pike_string *s;
};

#ifndef STRUCT_HASH_TABLE_DECLARED
#define STRUCT_HASH_TABLE_DECLARED
#endif
struct hash_table
{
  INT32 mask;
  INT32 entries;
  struct hash_entry *htable[1];
};

/* Prototypes begin here */
struct hash_entry *hash_lookup(struct hash_table *h, struct pike_string *s);
struct hash_table *create_hash_table(void);
struct hash_table *hash_rehash(struct hash_table *h,int size);
struct hash_table *hash_insert(struct hash_table *h, struct hash_entry *s);
struct hash_table *hash_unlink(struct hash_table *h, struct hash_entry *s);
void map_hashtable(struct hash_table *h, void (*fun)(struct hash_entry *));
void free_hashtable(struct hash_table *h,
		    void (*free_entry)(struct hash_entry *));
/* Prototypes end here */

#endif
