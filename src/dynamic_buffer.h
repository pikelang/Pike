/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef DYNAMIC_BUFFER_H
#define DYNAMIC_BUFFER_H

#define BUFFER_BEGIN_SIZE 4080

#include "types.h"

struct string_s
{
  char *str;
  SIZE_T len;
};

typedef struct string_s string;

struct dynamic_buffer_s
{
  string s;
  SIZE_T bufsize;
};

typedef struct dynamic_buffer_s dynamic_buffer;

/* Prototypes begin here */
char *low_make_buf_space(INT32 space,dynamic_buffer *buf);
void low_my_putchar(char b,dynamic_buffer *buf);
void low_my_binary_strcat(const char *b,INT32 l,dynamic_buffer *buf);
void low_init_buf(dynamic_buffer *buf);
void low_init_buf_with_string(string s,dynamic_buffer *buf);
string complex_free_buf(void);
void toss_buffer(dynamic_buffer *buf);
char *simple_free_buf(void);
struct lpc_string *low_free_buf(dynamic_buffer *buf);
struct lpc_string *free_buf(void);
char *make_buf_space(INT32 space);
void my_putchar(char b);
void my_binary_strcat(const char *b,INT32 l);
void my_strcat(const char *b);
void init_buf(void);
void init_buf_with_string(string s);
char *return_buf(void);
/* Prototypes end here */

#endif
