/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: spider.c,v 1.127 2004/09/19 00:51:13 nilsson Exp $
*/

#include "global.h"
#include "config.h"


#include "machine.h"

#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "fdlib.h"
#include "stralloc.h"
#include "pike_macros.h"
#include "machine.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "array.h"
#include "builtin_functions.h"
#include "module_support.h"
#include "backend.h"
#include "threads.h"
#include "operators.h"
#include "pike_security.h"

RCSID("$Id: spider.c,v 1.127 2004/09/19 00:51:13 nilsson Exp $");

#ifdef HAVE_PWD_H
#include <pwd.h>
#undef HAVE_PWD_H
#endif

#include "defs.h"

#ifdef HAVE_SYS_CONF_H
#include <sys/conf.h>
#endif

#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#include <errno.h>

/* #include <stdlib.h> */

#include "dmalloc.h"


#define sp Pike_sp

#define MAX_PARSE_RECURSE 102

void do_html_parse(struct pike_string *ss,
		   struct mapping *cont,struct mapping *single,
		   int *strings,int recurse_left,
		   struct array *extra_args);

void do_html_parse_lines(struct pike_string *ss,
			 struct mapping *cont,struct mapping *single,
			 int *strings,int recurse_left,
			 struct array *extra_args,
			 int line);

/*! @module spider
 */

/*! @decl array(mapping(string:int)|int) @
 *!                     parse_accessed_database(string database)
 */
void f_parse_accessed_database(INT32 args)
{
  ptrdiff_t cnum = 0, i;
  struct array *arg;
  struct mapping *m;

  if(!args)
    SIMPLE_TOO_FEW_ARGS_ERROR("parse_accessed_database",1);

  if ((sp[-args].type != T_STRING) || (sp[-args].u.string->size_shift)) {
    Pike_error("Bad argument 1 to parse_accessed_database(string(8)).\n");
  }

  /* Pop all but the first argument */
  pop_n_elems(args-1);

  push_constant_text("\n");
  f_divide(2);

  if (sp[-1].type != T_ARRAY) {
    Pike_error("Expected array as result of string-division.\n");
  }

  /* The initial string is gone, but the array is there now. */
  arg = sp[-1].u.array;

  push_mapping(m = allocate_mapping(arg->size));

  for(i = 0; i < arg->size; i++)
  {
    ptrdiff_t j,k;
    char *s;
    s=(char *)(ITEM(arg)[i].u.string->str);
    k=(ITEM(arg)[i].u.string->len);
    for(j=k; j>0 && s[j-1]!=':'; j--);
    if(j>0)
    {
      push_string(make_shared_binary_string(s, j-1));
      k=atoi(s+j);
      if(k>cnum)
	cnum=k;
      push_int(DO_NOT_WARN(k));
      mapping_insert(m, sp-2, sp-1);
      pop_n_elems(2);
    }
  }
  stack_swap();
  pop_stack();
  push_int(DO_NOT_WARN(cnum));
  f_aggregate(2);
}

/*! @decl string parse_html(string html, @
 *!                         mapping(string:function(string, mapping(string:string), mixed ...:string|array)) tag_callbacks, @
 *!                         mapping(string:function(string, mapping(string:string), string, mixed ...:string|array)) container_callbacks, @
 *!                         mixed ... extras)
 */
void f_parse_html(INT32 args)
{
  struct pike_string *ss;
  struct mapping *cont,*single;
  int strings;
  struct array *extra_args;
  ONERROR serr, cerr, eerr, sserr;

  if (args<3||
      sp[-args].type!=T_STRING||
      sp[1-args].type!=T_MAPPING||
      sp[2-args].type!=T_MAPPING)
    Pike_error("Bad argument(s) to parse_html.\n");

  ss=sp[-args].u.string;
  if(!ss->len)
  {
    pop_n_elems(args);
    push_text("");
    return;
  }

  add_ref(ss);
  add_ref(single=sp[1-args].u.mapping);
  add_ref(cont=sp[2-args].u.mapping);

  SET_ONERROR(serr, do_free_mapping, single);
  SET_ONERROR(cerr, do_free_mapping, cont);
  SET_ONERROR(sserr, do_free_string, ss);

  if (args>3)
  {
    f_aggregate(args-3);
    add_ref(extra_args=sp[-1].u.array);
    pop_stack();
    SET_ONERROR(eerr, do_free_array, extra_args);
  }
  else extra_args=NULL;

  pop_n_elems(3);

  strings=0;
  do_html_parse(ss,cont,single,&strings,MAX_PARSE_RECURSE,extra_args);

  if (extra_args) {
    UNSET_ONERROR(eerr);
    free_array(extra_args);
  }


  UNSET_ONERROR(sserr);
  UNSET_ONERROR(cerr);
  UNSET_ONERROR(serr);

  free_mapping(cont);
  free_mapping(single);
  if(strings > 1)
    f_add(strings);
  else if(!strings)
    push_text("");
}


/*! @decl string parse_html_lines(string html, @
 *!                         mapping(string:function(string, mapping(string:string), int, mixed ...:string|array)) tag_callbacks, @
 *!                         mapping(string:function(string, mapping(string:string), string, int, mixed ...:string|array)) container_callbacks, @
 *!                         mixed ... extras)
 */
void f_parse_html_lines(INT32 args)
{
  struct pike_string *ss;
  struct mapping *cont,*single;
  int strings;
  struct array *extra_args;
  ONERROR serr, cerr, eerr, sserr;
  if (args<3||
      sp[-args].type!=T_STRING||
      sp[1-args].type!=T_MAPPING||
      sp[2-args].type!=T_MAPPING)
    Pike_error("Bad argument(s) to parse_html_lines.\n");

  ss=sp[-args].u.string;
  if(!ss->len)
  {
    pop_n_elems(args);
    push_text("");
    return;
  }

  sp[-args].type=T_INT;

  add_ref(single=sp[1-args].u.mapping);
  add_ref(cont=sp[2-args].u.mapping);
  
  if (args>3)
  {
    f_aggregate(args-3);
    add_ref(extra_args=sp[-1].u.array);
    pop_stack();
    SET_ONERROR(eerr, do_free_array, extra_args);
  }
  else extra_args=NULL;

  pop_n_elems(3);
/*   fprintf(stderr, "sp=%p\n", sp); */
  SET_ONERROR(serr, do_free_mapping, single);
  SET_ONERROR(cerr, do_free_mapping, cont);
  SET_ONERROR(sserr, do_free_string, ss);
  strings=0;
  do_html_parse_lines(ss,cont,single,&strings,MAX_PARSE_RECURSE,extra_args,1);

  UNSET_ONERROR(sserr);
  UNSET_ONERROR(cerr);
  UNSET_ONERROR(serr);

  if(extra_args) {
    UNSET_ONERROR(eerr);
    free_array(extra_args);
  }

  free_mapping(cont);
  free_mapping(single);
  if(strings > 1)
    f_add(strings);
  else if(!strings)
    push_text("");
/*   fprintf(stderr, "sp=%p (strings=%d)\n", sp, strings); */
}

static char start_quote_character = '\000';
static char end_quote_character = '\000';

/*! @decl void set_end_quote(int quote)
 */
void f_set_end_quote(INT32 args)
{
  if(args < 1 || sp[-1].type != T_INT)
    Pike_error("Wrong argument to set_end_quote(int CHAR)\n");
  end_quote_character = sp[-1].u.integer;
}

/*! @decl void set_start_quote(int quote)
 */
void f_set_start_quote(INT32 args)
{
  if(args < 1 || sp[-1].type != T_INT)
    Pike_error("Wrong argument to set_start_quote(int CHAR)\n");
  start_quote_character = sp[-1].u.integer;
}


#define PUSH() do{\
     if(i>=j){\
       push_string(make_shared_binary_string(s+j,i-j));\
       strs++;\
       j=i;\
     } }while(0)

#define SKIP_SPACE()  while (i<len && ISSPACE(((unsigned char *)s)[i])) i++
#define STARTQUOTE(C) do{PUSH();j=i+1;inquote = 1;endquote=(C);}while(0)
#define ENDQUOTE() do{PUSH();j++;inquote=0;endquote=0;}while(0)

ptrdiff_t extract_word(char *s, ptrdiff_t i, ptrdiff_t len, int is_SSI_tag)
{
  int inquote = 0;
  char endquote = 0;
  ptrdiff_t j;      /* Start character for this word.. */
  int strs = 0;

  SKIP_SPACE();
  j=i;

  /* Should we really allow "foo"bar'gazonk' ? */

  for(;i<len; i++)
  {
    switch(s[i])
    {
    case ' ':  case '\t': case '\n':
    case '\r': case '>':  case '=':
      if(!inquote) {
	if (is_SSI_tag && (s[i] == '>') && (i-j == 2) &&
	    (s[j] == '-') && (s[j+1] == '-')) {
	  /* SSI tag that ends with "-->",
	   * don't add the "--" to the attribute.
	   */
	  j = i;	/* Skip */
	}
	goto done;
      }
      break;

     case '"':
     case '\'':
      if(inquote)
      {
	if(endquote==s[i])
	  ENDQUOTE();
      } else if(start_quote_character != s[i])
	STARTQUOTE(s[i]);
      else
	STARTQUOTE(end_quote_character);
      break;

    default:
      if(!inquote)
      {
	if(s[i] == start_quote_character)
	  STARTQUOTE(end_quote_character);
      }
      else if(endquote == end_quote_character) {
	if(s[i] == endquote) {
	  if(!--inquote)
	    ENDQUOTE();
	  else if(s[i] == start_quote_character)
	    inquote++;
	}
      }
      break;
    }
  }
done:
  if(!strs || i-j > 0) PUSH();
  if(strs > 1)
    f_add(strs);
  else if(!strs)
    push_text("");

  SKIP_SPACE();
  return i;
}
#undef PUSH
#undef SKIP_SPACE


ptrdiff_t push_parsed_tag(char *s, ptrdiff_t len)
{
  ptrdiff_t i=0;
  struct svalue *oldsp;
  int is_SSI_tag;

  /* NOTE: At entry sp[-1] is the tagname */
  is_SSI_tag = (sp[-1].type == T_STRING) &&
    (!strncmp(sp[-1].u.string->str, "!--", 3));

  /* Find X=Y pairs. */
  oldsp = sp;

  while (i<len && s[i]!='>')
  {
    ptrdiff_t oldi;
    oldi = i;
    i = extract_word(s, i, len, is_SSI_tag);
    f_lower_case(1);            /* Since SGML wants us to... */
    if (i+1 >= len || (s[i] != '='))
    {
      /* No 'Y' part here. Assign to 'X' */
      if (sp[-1].u.string->len) {
	assign_svalue_no_free(sp,sp-1);
	sp++;
      } else {
	/* Empty string -- throw away */
	pop_stack();
      }
    } else {
      i = extract_word(s, i+1, len, is_SSI_tag);
    }
    if(oldi == i) break;
  }
  f_aggregate_mapping(DO_NOT_WARN(sp - oldsp));
  if(i<len) i++;

  return i;
}

INLINE int tagsequal(char *s, char *t, ptrdiff_t len, char *end)
{
  if(s+len >= end)  return 0;

  while(len--) {
    if(tolower(*((unsigned char *)(t++))) !=
       tolower(*((unsigned char *)(s++))))
      return 0;
  }

  switch(*s) {
  case '>':
  case ' ':
  case '\t':
  case '\n':
  case '\r':
    return 1;
  default:
    return 0;
  }
}

ptrdiff_t find_endtag(struct pike_string *tag, char *s, ptrdiff_t len,
		      ptrdiff_t *aftertag)
{
  ptrdiff_t num=1;

  ptrdiff_t i,j;

  for (i=j=0; i < len; i++)
  {
    for (; i<len && s[i]!='<'; i++);
    if (i>=len) break;
    j=i++;
    for(; i<len && (s[i]==' ' || s[i]=='\t' || s[i]=='\n' || s[i]=='\r'); i++);
    if (i>=len) break;
    if (s[i]=='/')
    {
      if(tagsequal(s+i+1, tag->str, tag->len, s+len) && !(--num))
	break;
    } else if(tagsequal(s+i, tag->str, tag->len, s+len)) {
      ++num;
    }
  }

  if(i >= len)
  {
    *aftertag=len;
    j=i;              /* no end */
  } else {
    for (; i<len && s[i] != '>'; i++);
    *aftertag = i + (i<len?1:0);
  }
  return j;
}

void do_html_parse(struct pike_string *ss,
		   struct mapping *cont,struct mapping *single,
		   int *strings,int recurse_left,
		   struct array *extra_args)
{
  ptrdiff_t i,j,k,l,m,len,last;
  char *s;
  struct svalue sval1,sval2;
  struct pike_string *ss2;

  if (!ss->len)
  {
    free_string(ss);
    return;
  }

  if (!recurse_left)
  {
    push_string(ss);
    (*strings)++;
    return;
  }

  s=ss->str;
  len=ss->len;

  last=0;
  for (i=0; i<len-1;)
  {
    if (s[i]=='<')
    {
      ptrdiff_t n;
      /* skip all spaces */
      i++;
      for (n=i;n<len && ISSPACE(((unsigned char *)s)[n]); n++);
      /* Find tag name
       *
       * Ought to handle the <"tag"> and <'tag'> cases too.
       */
      for (j=n; j<len && s[j]!='>' && !ISSPACE(((unsigned char *)s)[j]); j++);

      if (j==len) break; /* end of string */

      push_string(make_shared_binary_string((char *)s+n, j-n));
      f_lower_case(1);
      add_ref(sval2.u.string = sp[-1].u.string);
      sval2.type=T_STRING;
      pop_stack();

      /* Is this a non-container? */
      mapping_index_no_free(&sval1,single,&sval2);

      if (sval1.type==T_STRING)
      {
	int quote = 0;
	/* A simple string ... */
	if (last < i-1)
	{
	  push_string(make_shared_binary_string(s+last, i-last-1));
	  (*strings)++;
	}

	assign_svalue_no_free(sp++,&sval1);
	(*strings)++;
	free_svalue(&sval1);
	free_svalue(&sval2);

	/* Scan ahead to the end of the tag... */
	for (; j<len; j++) {
	  if (quote) {
	    if (s[j] == quote) {
	      quote = 0;
	    }
	  } else if (s[j] == '>') {
	    break;
	  } else if ((s[j] == '\'') || (s[j] == '\"')) {
	    quote = s[j];
	  }
	}
	if (j < len) {
	  j++;
	}
	i=last=j;
	continue;
      }
      else if (sval1.type!=T_INT)
      {
	ONERROR sv1, sv2;
	/* Hopefully something callable ... */
	assign_svalue_no_free(sp++,&sval2);
	k = push_parsed_tag(s+j,len-j);
	if (extra_args)
	{
	  add_ref(extra_args);
	  push_array_items(extra_args);
	}
	SET_ONERROR(sv1, do_free_svalue, &sval1);
	SET_ONERROR(sv2, do_free_svalue, &sval2);
	dmalloc_touch_svalue(&sval1);
	apply_svalue(&sval1,2+(extra_args?extra_args->size:0));
	UNSET_ONERROR(sv2);
	UNSET_ONERROR(sv1);
	free_svalue(&sval2);
	free_svalue(&sval1);

	if (sp[-1].type==T_STRING)
	{
	  copy_shared_string(ss2,sp[-1].u.string);
	  pop_stack();
	  if (last!=i-1)
	  {
	    push_string(make_shared_binary_string(s+last,i-last-1));
	    (*strings)++;
	  }
	  i=last=j+k;
	  do_html_parse(ss2,cont,single,strings,recurse_left-1,extra_args);
	  continue;
	} else if (sp[-1].type==T_ARRAY) {
	  push_text("");
	  f_multiply(2);
	  copy_shared_string(ss2,sp[-1].u.string);
	  pop_stack();

	  if (last != i-1)
	  {
	    push_string(make_shared_binary_string(s+last,i-last-1));
	    (*strings)++;
	  }
	  i=last=j+k;

	  push_string(ss2);
	  (*strings)++;
	  continue;
	}
	pop_stack();
	continue;
      }

      /* Is it a container then? */
      free_svalue(&sval1);
      mapping_index_no_free(&sval1,cont,&sval2);
      if (sval1.type==T_STRING)
      {
	if (last < i-1)
	{
	  push_string(make_shared_binary_string(s+last, i-last-1));
	  (*strings)++;
	}

	assign_svalue_no_free(sp++,&sval1);
	free_svalue(&sval1);
	(*strings)++;

	find_endtag(sval2.u.string,s+j,len-j,&l);
	free_svalue(&sval2);
	j+=l;
	i=last=j;
	continue;
      }
      else if (sval1.type != T_INT)
      {
	ONERROR sv1, sv2;
	assign_svalue_no_free(sp++, &sval2);
	m = push_parsed_tag(s+j, len-j) + j;
	k = find_endtag(sval2.u.string, s+m, len-m, &l);
	push_string(make_shared_binary_string(s+m, k));
	m += l;
        /* M == just after end tag, from s */

	if (extra_args)
	{
	  add_ref(extra_args);
	  push_array_items(extra_args);
	}

	SET_ONERROR(sv1, do_free_svalue, &sval1);
	SET_ONERROR(sv2, do_free_svalue, &sval2);
	dmalloc_touch_svalue(&sval1);
	apply_svalue(&sval1,3+(extra_args?extra_args->size:0));
	UNSET_ONERROR(sv2);
	UNSET_ONERROR(sv1);
	free_svalue(&sval1);
	free_svalue(&sval2);

	if (sp[-1].type==T_STRING)
	{
	  copy_shared_string(ss2,sp[-1].u.string);
	  pop_stack();

	  /* i == '<' + 1 */
	  /* last == end of previous tags '>' + 1 */
	  if (last < i-1)
	  {
	    push_string(make_shared_binary_string(s+last, i-last-1));
	    (*strings)++;
	  }
	  i=last=j=m;
	  do_html_parse(ss2,cont,single,strings,recurse_left-1,extra_args);
	  continue;

	} else if (sp[-1].type==T_ARRAY) {
	  push_text("");
	  f_multiply(2);
	  copy_shared_string(ss2,sp[-1].u.string);
	  pop_stack();

	  if (last < i-1)
	  {
	    push_string(make_shared_binary_string(s+last, i-last-1));
	    (*strings)++;
	  }
	  i=last=j=m;
	  push_string(ss2);
	  (*strings)++;
	  continue;
	}
	pop_stack();
	continue;
      }
      free_svalue(&sval1);
      free_svalue(&sval2);
      i=j;
    }
    else
      i++;
  }

  if (last==0)
  {
    push_string(ss);
    (*strings)++;
  }
  else if (last<len)
  {
    push_string(make_shared_binary_string(s+last,len-last));
    free_string(ss);
    (*strings)++;
  }
  else
  {
    free_string(ss);
  }
}


#define PARSE_RECURSE(END) do {					\
  copy_shared_string(ss2,sp[-1].u.string); 			\
  pop_stack();							\
  if (last!=i-1)						\
  {								\
    push_string(make_shared_binary_string(s+last,i-last-1));	\
    (*strings)++; 						\
  }								\
  for (;i<END; i++) if (s[i]==10) line++;			\
  i=last=j=END;							\
  do_html_parse_lines(ss2,cont,single,strings,			\
		      recurse_left-1,extra_args,line);		\
} while(0)


#define PARSE_RETURN(END) do{					\
  push_text("");						\
  f_multiply(2);						\
  (*strings)++;							\
  if (last!=i-1)						\
  {								\
    copy_shared_string(ss2,sp[-1].u.string);			\
    pop_stack();						\
    push_string(make_shared_binary_string(s+last,i-last-1)); 	\
    (*strings)++; 						\
    push_string(ss2);						\
  }								\
  for (;i<END; i++) if (s[i]==10) line++;			\
  i=last=END;							\
} while(0)

#define HANDLE_RETURN_VALUE(END) do {		\
  free_svalue(&sval1);                          \
  if (sp[-1].type==T_STRING)			\
  {						\
    PARSE_RECURSE(END);				\
    continue;					\
  } else if (sp[-1].type==T_ARRAY) {		\
    PARSE_RETURN(END);				\
    continue;					\
  }						\
  pop_stack();					\
} while(0)

static struct svalue empty_string_svalue;
void do_html_parse_lines(struct pike_string *ss,
			 struct mapping *cont,struct mapping *single,
			 int *strings,int recurse_left,
			 struct array *extra_args,
			 int line)
{
  ptrdiff_t i,j,k,l,m,len,last;
  char *s;
  struct svalue sval1,sval2;
  struct pike_string *ss2;

/*   fprintf(stderr, "sp=%p (strings=%d)\n", sp, *strings); */

  if (!ss->len)
  {
    free_string(ss);
    return;
  }

  if (!recurse_left)
  {
    push_string(ss);
    (*strings)++;
    return;
  }

  s=ss->str;
  len=ss->len;

  last=0;


  for (i=0; i<len-1;)
  {
    if (s[i]==10)
    {
      line++;
      i++;
    }  else if (s[i]=='<') {
      /* skip all spaces */
      i++;
      for (j=i; j<len && s[j]!='>' && !isspace(((unsigned char *)s)[j]); j++);

      if (j==len) break; /* end of string */

      push_string(make_shared_binary_string((char *)s+i, j-i));
      f_lower_case(1);
      add_ref(sval2.u.string = sp[-1].u.string);
      sval2.type=T_STRING;
      pop_stack();

      /* Is this a non-container? */
      mapping_index_no_free(&sval1,single,&sval2);
/*       if(sval1.type == T_INT) */
/* 	mapping_index_no_free(&sval1,single,&empty_string_svalue); */

      if (sval1.type==T_STRING)
      {
	int quote = 0;
	/* A simple string ... */
	if (last < i-1)
	{
	  push_string(make_shared_binary_string(s+last, i-last-1));
	  (*strings)++;
	}

	*(sp++)=sval1;
#ifdef PIKE_DEBUG
	sval1.type=99;
#endif
	(*strings)++;
	free_svalue(&sval2);

	/* Scan ahead to the end of the tag... */
	for (; j<len; j++) {
	  if (quote) {
	    if (s[j] == quote) {
	      quote = 0;
	    }
	  } else if (s[j] == '>') {
	    break;
	  } else if ((s[j] == '\'') || (s[j] == '\"')) {
	    quote = s[j];
	  }
	}
	if (j < len) {
	  j++;
	}
	i=last=j;
	continue;
      }
      else if (sval1.type!=T_INT)
      {
	ONERROR sv1;
	*(sp++)=sval2;
#ifdef PIKE_DEBUG
	sval2.type=99;
#endif
	k=push_parsed_tag(s+j,len-j);
	push_int(line);
	if (extra_args)
	{
	  add_ref(extra_args);
	  push_array_items(extra_args);
	}
	dmalloc_touch_svalue(&sval1);
	SET_ONERROR(sv1, do_free_svalue, &sval1);
	apply_svalue(&sval1,3+(extra_args?extra_args->size:0));
	UNSET_ONERROR(sv1);

	HANDLE_RETURN_VALUE(j+k);
	continue;
      }

      /* free_svalue(&sval1); Not needed. The type is always T_INT */
      /* Is it a container then? */

      mapping_index_no_free(&sval1,cont,&sval2);
      if(sval1.type == T_INT)
	mapping_index_no_free(&sval1,cont,&empty_string_svalue);
      if (sval1.type==T_STRING)
      {
	if (last < i-1)
	{
	  push_string(make_shared_binary_string(s+last, i-last-1));
	  (*strings)++;
	}

	*(sp++)=sval1;
#ifdef PIKE_DEBUG
	sval1.type=99;
#endif
	(*strings)++;
	find_endtag(sval2.u.string,s+j,len-j,&l);
	free_svalue(&sval2);
	j+=l;
	for (; i<j; i++) if (s[i]==10) line++;
	i=last=j;
	continue;
      }
      else if (sval1.type != T_INT)
      {
	ONERROR sv1;

	*(sp++)=sval2;
#ifdef PIKE_DEBUG
	sval2.type=99;
#endif
	m = push_parsed_tag(s+j, len-j) + j;
	k = find_endtag(sval2.u.string, s+m, len-m, &l);
	push_string(make_shared_binary_string(s+m, k));
	m += l;
        /* M == just after end tag, from s */
	push_int(line);
	if (extra_args)
	{
	  add_ref(extra_args);
	  push_array_items(extra_args);
	}
	SET_ONERROR(sv1, do_free_svalue, &sval1);
	dmalloc_touch_svalue(&sval1);
	apply_svalue(&sval1,4+(extra_args?extra_args->size:0));
	UNSET_ONERROR(sv1);

	HANDLE_RETURN_VALUE(m);
	continue;
      } else {
	free_svalue(&sval2);
      }
      i=j;
    }
    else
      i++;
  }

  if (last==0)
  {
    push_string(ss);
    (*strings)++;
  }
  else if (last<len)
  {
    push_string(make_shared_binary_string(s+last,len-last));
    free_string(ss);
    (*strings)++;
  }
  else
  {
    free_string(ss);
  }
}

/*! @decl array(int) get_all_active_fds()
 */
void f_get_all_active_fd(INT32 args)
{
  int i,fds,ne;
  PIKE_STAT_T foo;

  ne = MAX_OPEN_FILEDESCRIPTORS;

  pop_n_elems(args);
  for (i=fds=0; i<ne; i++)
  {
    int q;
    THREADS_ALLOW();
    q = fd_fstat(i,&foo);
    THREADS_DISALLOW();
    if(!q)
    {
      push_int(i);
      fds++;
    }
  }
  f_aggregate(fds);
}

/*! @decl string fd_info(int fd)
 */
void f_fd_info(INT32 args)
{
  static char buf[256];
  int i;
  PIKE_STAT_T foo;

  VALID_FILE_IO("spider.fd_info","status");

  if (args<1||
      sp[-args].type!=T_INT)
    Pike_error("Illegal argument to fd_info\n");
  i=sp[-args].u.integer;
  pop_n_elems(args);
  if (fd_fstat(i,&foo))
  {
    push_text("non-open filedescriptor");
    return;
  }
  sprintf(buf,"%o,%ld,%d,%ld",
	  (unsigned int)foo.st_mode,
	  (long)foo.st_size,
	  (int)foo.st_dev,
	  (long)foo.st_ino);
  push_text(buf);
}

static void program_name(struct program *p)
{
  char *f;
  INT32 n=0;
  ref_push_program(p);
  APPLY_MASTER("program_name", 1);
  if(sp[-1].type == T_STRING)
    return;
  pop_stack();
  f=(char *)(p->linenumbers+1);

  if(!p->linenumbers || !strlen(f))
    push_text("Unknown program");

  push_string( get_program_line( p, &n ) );
  push_text( ":" );
  push_int( n );
  f_add( 3 );
}

/*! @decl string _low_program_name(program prog)
 */
void f__low_program_name( INT32 args )
{
  struct program *p;
  get_all_args( "_low_program_name", args, "%p", &p );
  program_name( p );
  stack_swap();
  pop_stack();
}

/*! @decl array(array(string|int)) _dump_obj_table()
 */
void f__dump_obj_table(INT32 args)
{
  struct object *o;
  int n=0;

  ASSERT_SECURITY_ROOT("spider._dump_obj_table");

  pop_n_elems(args);
  o=first_object;
  while(o)
  {
    if(o->prog)
      program_name(o->prog);
    else
      push_text("No program (Destructed?)");
    push_int(o->refs);
    f_aggregate(2);
    ++n;
    o=o->next;
  }
  f_aggregate(n);
}

/*! @endmodule
 */

PIKE_MODULE_INIT
{
  push_constant_text("");
  empty_string_svalue = sp[-1];
  pop_stack();

  ADD_FUNCTION("_low_program_name",f__low_program_name,
	       tFunc(tPrg(tObj),tStr),0);

  /* function(int:int) */
  ADD_FUNCTION("set_start_quote",f_set_start_quote,
	       tFunc(tInt,tInt),OPT_EXTERNAL_DEPEND);

  /* function(int:int) */
  ADD_FUNCTION("set_end_quote",f_set_end_quote,
	   tFunc(tInt,tInt),OPT_EXTERNAL_DEPEND);

  /* function(string:array) */
  ADD_FUNCTION("parse_accessed_database", f_parse_accessed_database,
	       tFunc(tStr,tArray), OPT_TRY_OPTIMIZE);

  /* function(:array(array)) */
  ADD_FUNCTION("_dump_obj_table", f__dump_obj_table,
	       tFunc(tNone,tArr(tArray)), OPT_EXTERNAL_DEPEND);


  ADD_FUNCTION("parse_html",f_parse_html,
	   tFuncV(tStr
		  tMap(tStr,tOr(tStr,
				tFuncV(tOr(tStr,tVoid)
				       tOr(tMap(tStr,tStr),tVoid),
				       tMix,
				       tOr(tStr,tArr(tStr)))))
		  tMap(tStr,tOr(tStr,
				tFuncV(tOr(tStr,tVoid)
				       tOr(tMap(tStr,tStr),tVoid)
				       tOr(tStr,tVoid),
				       tMix,
				       tOr(tStr,tArr(tStr))))),
		  tMix,
		  tStr),
	   OPT_SIDE_EFFECT);


  ADD_FUNCTION("parse_html_lines",f_parse_html_lines,
	   tFuncV(tStr
		  tMap(tStr,tOr(tStr,
				tFuncV(tOr(tStr,tVoid)
				       tOr(tMap(tStr,tStr),tVoid)
				       tOr(tInt,tVoid),
				       tMix,
				       tOr(tStr,tArr(tStr)))))
		  tMap(tStr,tOr(tStr,
				tFuncV(tOr(tStr,tVoid)
				       tOr(tMap(tStr,tStr),tVoid)
				       tOr(tStr,tVoid)
				       tOr(tInt,tVoid),
				       tMix,
				       tOr(tStr,tArr(tStr))))),
		  tMix,
		  tStr),
	   0);

  /* function(int:array) */
  ADD_FUNCTION("discdate", f_discdate,tFunc(tInt,tArray), 0);

  /* function(int,void|int:int) */
  ADD_FUNCTION("stardate", f_stardate,tFunc(tInt tInt,tInt), 0);

  /* function(:array(int)) */
  ADD_FUNCTION("get_all_active_fd", f_get_all_active_fd,
	       tFunc(tNone,tArr(tInt)), OPT_EXTERNAL_DEPEND);

  /* function(int:string) */
  ADD_FUNCTION("fd_info", f_fd_info,tFunc(tInt,tStr), OPT_EXTERNAL_DEPEND);

  {
    extern void init_xml(void);
    init_xml();
  }
}


PIKE_MODULE_EXIT
{
  {
    extern void exit_xml(void);
    exit_xml();
  }
}
