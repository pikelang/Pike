/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "types.h"

#define AVERAGE_HASH_LENGTH 16
#define NEW_HASHTABLE_SIZE 4

struct hash_entry
{
  struct hash_entry *next;
  struct lpc_string *s;
};

struct hash_table
{
  INT32 mask;
  INT32 entries;
  struct hash_entry *htable[1];
};

/* Prototypes begin here */
struct hash_entry *hash_lookup(struct hash_table *h, struct lpc_string *s);
struct hash_table *create_hash_table();
struct hash_table *hash_rehash(struct hash_table *h,int size);
struct hash_table *hash_insert(struct hash_table *h, struct hash_entry *s);
struct hash_table *hash_unlink(struct hash_table *h, struct hash_entry *s);
void map_hashtable(struct hash_table *h, void (*fun)(struct hash_entry *));
void free_hashtable(struct hash_table *h,
		    void (*free_entry)(struct hash_entry *));
/* Prototypes end here */

#endif
