/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
void enter_program_hash PROT((struct program *));
void remove_program_hash PROT((struct program *));
struct program *lookup_program_hash PROT((char *));
char *show_otable_status PROT((int verbose));
void enter_my_object_hash(struct object *ob);
void remove_my_object_hash(struct object *ob);
void uninit_otable(void);
void uninit_ohash(void);

