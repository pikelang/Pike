#ifndef CPP_H
#define CPP_H

#ifndef STRUCT_HASH_ENTRY_DECLARED
struct hash_entry;
#define STRUCT_HASH_ENTRY_DECLARED
#endif

/* Prototypes begin here */
struct pike_predef_s;
struct define_part;
struct define_argument;
struct define;
struct cpp;
void cpp_error(struct cpp *this,char *err);
void PUSH_STRING(char *str,
		 INT32 len,
		 dynamic_buffer *buf);
void free_one_define(struct hash_entry *h);
void f_cpp(INT32 args);
void init_cpp(void);
void add_predefine(char *s);
void exit_cpp(void);
/* Prototypes end here */

#endif
