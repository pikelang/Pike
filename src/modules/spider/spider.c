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

RCSID("$Id: spider.c,v 1.70 1998/07/13 14:36:39 grubba Exp $");

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
#include "accesseddb.h"

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


extern void f_parse_tree(INT32 argc);


void f_nice(INT32 args)
{
#ifdef HAVE_NICE
  int ta = sp[-1].u.integer;
  if(!args) error("You must supply an argument to nice(int)!\n");
  pop_n_elems(args);
  push_int(nice(ta));
#endif
}

void f_http_decode_string(INT32 args)
{
   int proc;
   char *foo,*bar,*end;
   struct pike_string *newstr;

   if (!args || sp[-args].type != T_STRING)
     error("Invalid argument to http_decode_string(STRING);\n");

   foo=bar=sp[-args].u.string->str;
   end=foo+sp[-args].u.string->len;

   /* count '%' characters */
   for (proc=0; foo<end; ) if (*foo=='%') { proc++; foo+=3; } else foo++;

   if (!proc) { pop_n_elems(args-1); return; }

   /* new string len is (foo-bar)-proc*2 */
   newstr=begin_shared_string((foo-bar)-proc*2);
   foo=newstr->str;
   for (proc=0; bar<end; foo++)
      if (*bar=='%') 
      { 
	 if (bar<end-2)
	    *foo=(((bar[1]<'A')?(bar[1]&15):((bar[1]+9)&15))<<4)|
	       ((bar[2]<'A')?(bar[2]&15):((bar[2]+9)&15));
	 else
	    *foo=0;
	 bar+=3;
      }
      else { *foo=*(bar++); }
   pop_n_elems(args);
   push_string(end_shared_string(newstr)); 
}

void f_parse_accessed_database(INT32 args)
{
  int cnum=0, i, num=0;

  struct array *arg;
  if(args != 1)
    error("Wrong number of arguments to parse_accessed_database(string)\n");

  push_string(make_shared_string("\n"));
  f_divide(2);
  add_ref(arg = sp[-1].u.array);
  /* The initial string is gone, but the array is there now. */
  pop_stack();

  for (i = 0; i < arg->size; i++)
  {
    int j=0,k=0;
    char *s=0;
    s=(char *)(ITEM(arg)[i].u.string->str);
    k=(ITEM(arg)[i].u.string->len);
    for(j=k; j>0 && s[j-1]!=':'; j--);
    if(j>0)
    {
      push_string(make_shared_binary_string(s, j-1));
      k=atoi(s+j);
      if(k>cnum)
	cnum=k;
      push_int(k);
      num++;
    }
  }
  free_array(arg);
  f_aggregate_mapping(num*2);
  push_int(cnum);
  f_aggregate(2);
}



#ifdef I_SENDFD
#define HAVE_SEND_FD
void f_send_fd(INT32 args)
{
  int sock_fd, fd, tmp;

  if(args != 2) error("RTSL\n");

  sock_fd = sp[-args].u.integer;
  fd =  sp[-args+1].u.integer;
  pop_stack();
  pop_stack();

  THREADS_ALLOW();
  tmp=ioctl(sock_fd, I_SENDFD, fd);
  THREADS_DISALLOW();

  while(tmp == -1)
  {
    switch(errno)
    {
     case EINVAL:
      perror("Strange error while sending fd");
      push_int(0);
      return;

     case EIO:
     case EBADF:
     case ENOTSOCK:
     case ESTALE:
      perror("Cannot send fd");
      push_int(0);
      return;

     case EWOULDBLOCK:
     case EINTR:
     case ENOMEM:
#ifdef ENOSR
     case ENOSR:
#endif
      THREADS_ALLOW();
      tmp=ioctl(sock_fd, I_SENDFD, fd);
      THREADS_DISALLOW();
      continue;

     default:
      perror("Unknown error while ioctl(I_SENDFD)ing");
      push_int(0);
      return;
    }
  }
  push_int(1);
  return;
}

#else
#if 0
     /*
    / Shuffle is only compiled on Solaris anyway. It is _not_ easy to fix
   / this for _all_ systems (SYSV, BSDI, BSD, Linux all use different
  / styles)
*/
#if HAVE_SENDMSG
#define HAVE_SEND_FD
void f_send_fd(INT32 args)
{
  struct iovec iov; 
  struct msghdr msg;
  int sock_fd, fd, tmp;

  if(args != 2) error("RTSL\n");
  sock_fd = sp[-args].u.integer;
  fd =  sp[-args+1].u.integer;
  pop_stack();
  pop_stack();

  iov.iov_base         = NULL;
  iov.iov_len          = 0;
  msg.msg_iov          = &iov;
  msg.msg_iovlen       = 1;
  msg.msg_name         = NULL;
  msg.msg_accrights    = (caddr_t)&fd;
  msg.msg_accrightslen = sizeof(fd);


  THREADS_ALLOW();
  tmp=sendmsg(sock_fd, &msg, 0);
  THREADS_DISALLOW();

  while(tmp == -1)
  {
    switch(errno)
    {
     case EINVAL:
      perror("Strange error while sending fd");
      push_int(0);
      return;

     case EIO:
     case EBADF:
     case ENOTSOCK:
     case ESTALE:
      perror("Cannot send fd");
      push_int(0);
      return;

     case EWOULDBLOCK:
     case EINTR:
     case ENOMEM:
     case ENOSR:
      continue;

     default:
      perror("Unknown error while semdmsg()ing");
      push_int(0);
      return;
    }
  }
  push_int(1);
  return;
}
#endif
#endif
#endif

void f_parse_html(INT32 args)
{
  struct pike_string *ss;
  struct mapping *cont,*single;
  int strings;
  struct array *extra_args;
   
  if (args<3||
      sp[-args].type!=T_STRING||
      sp[1-args].type!=T_MAPPING||
      sp[2-args].type!=T_MAPPING)
    error("Bad argument(s) to parse_html.\n");

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

  if (args>3)
  {
    f_aggregate(args-3);
    add_ref(extra_args=sp[-1].u.array);
    pop_stack();
  }
  else extra_args=NULL;

  pop_n_elems(3);

  strings=0;
  do_html_parse(ss,cont,single,&strings,MAX_PARSE_RECURSE,extra_args);

  if (extra_args) free_array(extra_args);

  free_mapping(cont);
  free_mapping(single);
  if(strings > 1)
    f_add(strings);
  else if(!strings)
    push_text("");
}


void f_parse_html_lines(INT32 args)
{
  struct pike_string *ss;
  struct mapping *cont,*single;
  int strings;
  struct array *extra_args;
   
  if (args<3||
      sp[-args].type!=T_STRING||
      sp[1-args].type!=T_MAPPING||
      sp[2-args].type!=T_MAPPING)
    error("Bad argument(s) to parse_html_lines.\n");

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
  }
  else extra_args=NULL;

  pop_n_elems(3);
/*   fprintf(stderr, "sp=%p\n", sp); */

  strings=0;
  do_html_parse_lines(ss,cont,single,&strings,MAX_PARSE_RECURSE,extra_args,1);

  if (extra_args) free_array(extra_args);
  free_mapping(cont);
  free_mapping(single);
  if(strings > 1)
    f_add(strings);
  else if(!strings)
    push_text("");
/*   fprintf(stderr, "sp=%p (strings=%d)\n", sp, strings); */
}

char start_quote_character = '\000';
char end_quote_character = '\000';

void f_set_end_quote(INT32 args)
{
  if(args < 1 || sp[-1].type != T_INT)
    error("Wrong argument to set_end_quote(int CHAR)\n");
  end_quote_character = sp[-1].u.integer;
}

void f_set_start_quote(INT32 args)
{
  if(args < 1 || sp[-1].type != T_INT)
    error("Wrong argument to set_start_quote(int CHAR)\n");
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

int extract_word(char *s, int i, int len, int is_SSI_tag)
{
  int inquote = 0;
  char endquote = 0;
  int j;      /* Start character for this word.. */
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


int push_parsed_tag(char *s,int len)
{
  int i=0;
  struct svalue *oldsp;
  int is_SSI_tag;

  /* NOTE: At entry sp[-1] is the tagname */
  is_SSI_tag == (sp[-1].type == T_STRING) &&
    (!strncmp(sp[-1].u.string->str, "!--", 3));

  /* Find X=Y pairs. */
  oldsp = sp;

  while (i<len && s[i]!='>')
  {
    int oldi;
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
  f_aggregate_mapping(sp-oldsp);
  if(i<len) i++;

  return i;
}

INLINE int tagsequal(char *s, char *t, int len, char *end)
{
  if(s+len >= end)  return 0;

  while(len--) if(tolower(*(t++)) != tolower(*(s++)))
    return 0;

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

int find_endtag(struct pike_string *tag, char *s, int len, int *aftertag)
{
  int num=1;

  int i,j;

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
  int i,j,k,l,m,len,last;
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
      int n;
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
	/* A simple string ... */
	assign_svalue_no_free(sp++,&sval1);
	free_svalue(&sval1);
	(*strings)++;
	find_endtag(sval2.u.string ,s+j, len-j, &l);
	free_svalue(&sval2);
	j+=l;
	i=last=j;
	continue;
      }
      else if (sval1.type!=T_INT)
      {
	/* Hopefully something callable ... */
	assign_svalue_no_free(sp++,&sval2);
	k=push_parsed_tag(s+j,len-j); 
	if (extra_args)
	{
	  add_ref(extra_args);
	  push_array_items(extra_args);
	}

	apply_svalue(&sval1,2+(extra_args?extra_args->size:0));
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
	assign_svalue_no_free(sp++,&sval2);
	m=push_parsed_tag(s+j,len-j) + j;
	k=find_endtag(sval2.u.string,s+m,len-m,&l);
	push_string(make_shared_binary_string(s+m,k));
	m+=l;
        /* M == just after end tag, from s */

	if (extra_args)
	{
	  add_ref(extra_args);
	  push_array_items(extra_args);
	}

	apply_svalue(&sval1,3+(extra_args?extra_args->size:0));
	free_svalue(&sval1);
	free_svalue(&sval2);

	if (sp[-1].type==T_STRING)
	{
	  copy_shared_string(ss2,sp[-1].u.string);
	  pop_stack();

	  /* i == first '<' ? */
	  /* last == end of last tag */
	  if (last!=i-1)
	  { 
	    push_string(make_shared_binary_string(s+last,i-last-1)); 
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

	  if (last!=i-1)
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

struct svalue empty_string;
void do_html_parse_lines(struct pike_string *ss,
			 struct mapping *cont,struct mapping *single,
			 int *strings,int recurse_left,
			 struct array *extra_args,
			 int line)
{
  int i,j,k,l,m,len,last;
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
/* 	mapping_index_no_free(&sval1,single,&empty_string); */
	
      if (sval1.type==T_STRING)
      {
	/* A simple string ... */
	*(sp++)=sval1;
#ifdef DEBUG
	sval1.type=99;
#endif
	(*strings)++;
/* 	find_endtag(sval2.u.string ,s+j, len-j, &l); * bug /law 960805 */
	free_svalue(&sval2);
	j+=l;
	i=last=j;
	continue;
      }
      else if (sval1.type!=T_INT)
      {
	*(sp++)=sval2;
#ifdef DEBUG
	sval2.type=99;
#endif
	k=push_parsed_tag(s+j,len-j);
	push_int(line);
	if (extra_args)
	{
	  add_ref(extra_args);
	  push_array_items(extra_args);
	}
	apply_svalue(&sval1,3+(extra_args?extra_args->size:0));
	HANDLE_RETURN_VALUE(j+k);
	continue;
      }

      /* free_svalue(&sval1); Not needed. The type is always T_INT */
      /* Is it a container then? */

      mapping_index_no_free(&sval1,cont,&sval2);
      if(sval1.type == T_INT)
	mapping_index_no_free(&sval1,cont,&empty_string);
      if (sval1.type==T_STRING)
      {
	*(sp++)=sval1;
#ifdef DEBUG
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
	*(sp++)=sval2;
#ifdef DEBUG
	sval2.type=99;
#endif
	m=push_parsed_tag(s+j,len-j) + j;
	k=find_endtag(sval2.u.string,s+m,len-m,&l);
	push_string(make_shared_binary_string(s+m,k));
	m+=l;
        /* M == just after end tag, from s */
	push_int(line);
	if (extra_args)
	{
	  add_ref(extra_args);
	  push_array_items(extra_args);
	}
	apply_svalue(&sval1,4+(extra_args?extra_args->size:0));
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


static int does_match(char *s,int len,char *m,int mlen)
{
  int i,j;
  for (i=j=0; i<mlen&&j<len; )
    switch (m[i])
    {
     case '?': 
      j++; i++; 
      break;
     case '*': 
      i++;
      if (i==mlen) return 1;	/* slut */
      for (;j<len;j++)
	if (does_match(s+j,len-j,m+i,mlen-i)) return 1;
      return 0;
     default: 
      if (m[i]!=s[j]) return 0;
      j++; i++;
    }
  if (i==mlen && j==len) return 1;
  return 0;
}


#ifndef HAVE_INT_TIMEZONE
int _tz;
#else
extern long int timezone;
#endif

void f_timezone(INT32 args)
{
  pop_n_elems(args);
#ifndef HAVE_INT_TIMEZONE
  push_int(_tz);
#else
  push_int(timezone);
#endif
}

#ifdef HAVE_PERROR
void f_real_perror(INT32 args)
{
  pop_n_elems(args);
  perror(NULL);
}
#endif

void f_get_all_active_fd(INT32 args)
{
  int i,fds,q;
  struct stat foo;
  
  pop_n_elems(args);
  for (i=fds=0; i<MAX_OPEN_FILEDESCRIPTORS; i++)
  {
    int q;
    THREADS_ALLOW();
    q = fstat(i,&foo);
    THREADS_DISALLOW();
    if(!q)
    {
      push_int(i);
      fds++;
    }
  }
  f_aggregate(fds);
}

void f_fd_info(INT32 args)
{
  static char buf[256];
  int i;
  struct stat foo;

  if (args<1||
      sp[-args].type!=T_INT)
    error("Illegal argument to fd_info\n");
  i=sp[-args].u.integer;
  pop_n_elems(args);
  if (fstat(i,&foo))
  {
    push_string(make_shared_string("non-open filedescriptor"));
    return;
  }
  sprintf(buf,"%o,%ld,%d,%ld",
	  (unsigned int)foo.st_mode,
	  (long)foo.st_size,
	  (int)foo.st_dev,
	  (long)foo.st_ino);
  push_string(make_shared_string(buf));
}

struct pike_string *fd_marks[MAX_OPEN_FILEDESCRIPTORS];

void f_mark_fd(INT32 args)
{
  int fd;
  struct pike_string *s;
  if (args<1
      || sp[-args].type!=T_INT 
      || (args>2 && sp[-args+1].type!=T_STRING))
    error("Illegal argument(s) to mark_fd(int,void|string)\n");
  fd=sp[-args].u.integer;
  if(fd>MAX_OPEN_FILEDESCRIPTORS || fd < 0)
    error("Fd must be in the range 0 to %d\n", MAX_OPEN_FILEDESCRIPTORS);
  if (args<2)
  {
    int len;
    char *tmp;
    char buf[20];
    struct stat fs;

    
    pop_stack();
    if(!fstat(fd,&fs))
    {
      if(fd_marks[fd])
      {
	ref_push_string(fd_marks[fd]);
      } else {
	push_text("");
      }
      return;
    } else {
      if(fd_marks[fd])
      {
	free_string(fd_marks[fd]);
	fd_marks[fd]=0;
      }
      push_int(0);
      return;
    }
  }
  
  add_ref(s=sp[-args+1].u.string);
  if(fd_marks[fd])
    free_string(fd_marks[fd]);
  fd_marks[fd]=s;
  pop_n_elems(args);
  push_int(0);
}

#if 0
void f_fcgi_create_listen_socket(INT32 args)
{
  int fd, true;
  struct sockaddr_in addr;
  if(!args)
    error("No args?\n");
  
  true=1;
  fd=socket(AF_INET, SOCK_STREAM, 0);
  if(fd < 0)
    error("socket() failed.\n");
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&true, sizeof(int));
  get_inet_addr(&addr, "127.0.0.1");
  addr.sin_port = htons( ((u_short)sp[-args].u.integer) );
  if(bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0 ||
     listen(fd, 4) < 0 )
    error("Failed to bind() or listen\n");
  if(fd)
  {
    dup2(fd, 0);
    close(fd);
  }
  pop_n_elems(args);
  push_int(1);
}
#endif

static void program_name(struct program *p)
{
  char *f;
  ref_push_program(p);
  APPLY_MASTER("program_name", 1);
  if(sp[-1].type == T_STRING)
    return;
  pop_stack();
  f=(char *)(p->linenumbers+1);

  if(!p->linenumbers || !strlen(f))
    push_text("Unknown program");

  push_text(f);
}

void f__dump_obj_table(INT32 args)
{
  struct object *o;
  int n=0;
  pop_n_elems(args);
  o=first_object;
  while(o) 
  { 
    if(o->prog)
      program_name(o->prog);
    else 
      push_string(make_shared_binary_string("No program (Destructed?)",24));
    push_int(o->refs);
    f_aggregate(2);
    ++n;
    o=o->next;
  }
  f_aggregate(n);
}

#ifndef MIN
#define MIN(A,B) ((A)<(B)?(A):(B))
#endif

#if 0
extern char **ARGV;
void f_name_process(INT32 args)
{
  char *title;
  int len, i;
#ifdef SOLARIS
  int fd;
#endif
  get_all_args("name_process", args, "%s", &title);

  len = MIN(strlen(ARGV[0]),strlen(title));
  for(i=0; i<len; i++) ARGV[0][i]=title[i];
  
  pop_n_elems(args);
}
#endif

#ifdef ENABLE_STREAMED_PARSER
#include "streamed_parser.h"

static struct program *streamed_parser;

#endif /* ENABLE_STREAMED_PARSER */

extern void init_udp(void);


/* Hohum. Here we go. This is try number three for a more optimized Roxen. */

#ifdef _REENTRANT
#define BUFFER (8192)

struct thread_args
{
  struct thread_args *next;
  struct object *from;
  struct object *to;
  int to_fd, from_fd;
  struct svalue cb;
  struct svalue args;
  int len;
  int sent;
  char buffer[BUFFER];
};

MUTEX_T done_lock;
struct thread_args *done;

/* WARNING! This function is running _without_ any stack etc. */

#define MY_MIN(a,b) ((a)<(b)?(a):(b))
void do_shuffle(void *_a)
{
  struct thread_args *a = (struct thread_args *)_a;

#ifdef DIRECTIO_ON
  if(a->len > (65536*2))
    directio(a->from_fd, DIRECTIO_ON);
#endif

  while(a->len)
  {
    int nread, written=0;
    nread = fd_read(a->from_fd, a->buffer, MY_MIN(BUFFER,a->len));
    if(nread <= 0) {
      if(errno == EINTR)
	continue;
      else
	break;
    }

    while(nread)
    {
      int nsent = fd_write(a->to_fd, a->buffer+written, nread);
      if(nsent < 0) {
	if(errno != EINTR)
	  goto end;
	else 
	  continue;
      }
      written += nsent;
      a->sent += nsent;
      nread -= nsent;
      a->len -= nsent;
    }
  }

  /* We are done. It is up to the backend callback to call the 
   * finish function
   */
 end:
  mt_lock(&done_lock);
  a->next = done;
  done = a;
  mt_unlock(&done_lock);
  wake_up_backend();
}

static int num_shuffles = 0;
static struct callback *my_callback;

void finished_p(struct callback *foo, void *b, void *c)
{
  while(done)
  {
    struct thread_args *d;

    mt_lock(&done_lock);
    d = done;
    done = d->next;
    mt_unlock(&done_lock);

    num_shuffles--;

    push_int( d->sent );
    *(sp++) = d->args;
    push_object( d->from );
    push_object( d->to );
    apply_svalue( &d->cb, 4 );
    free_svalue( &d->cb );
    pop_stack();
    free(d);
  }

  if(!num_shuffles)
  {
    remove_callback( foo );
    my_callback = 0;
  }
}

void f_shuffle(INT32 args)
{
  struct thread_args *a = malloc(sizeof(struct thread_args));
  struct svalue *q, *w;
  get_all_args("shuffle", args, "%o%o%*%*%d", &a->from, &a->to,&q,&w,&a->len);
  a->sent = 0;

  num_shuffles++;
  apply(a->to, "query_fd", 0); 
  apply(a->from, "query_fd", 0);
  get_all_args("shuffle", 2, "%d%d", &a->to_fd, &a->from_fd);

  add_ref(a->from);
  add_ref(a->to); 
  
  assign_svalue_no_free(&a->cb, q);
  assign_svalue_no_free(&a->args, w);
  
  th_farm(do_shuffle, (void *)a);

  if(!my_callback)
    my_callback = add_backend_callback( finished_p, 0, 0 );

  pop_n_elems(args+2);
}
#endif


void pike_module_init(void) 
{
  ref_push_string(make_shared_string(""));
  empty_string = sp[-1];
  pop_stack();


#ifdef _REENTRANT
  add_function("shuffle", f_shuffle, 
	       "function(object,object,function,mixed,int:void)", 0);
#endif

#if 0
  add_efun("fcgi_create_listen_socket", f_fcgi_create_listen_socket,
	   "function(int:int)", OPT_SIDE_EFFECT);
#endif
  
  add_efun("http_decode_string",f_http_decode_string,"function(string:string)",
	   OPT_TRY_OPTIMIZE);

  add_efun("set_start_quote",f_set_start_quote,"function(int:int)",OPT_EXTERNAL_DEPEND);

  add_efun("set_end_quote",f_set_end_quote,"function(int:int)",OPT_EXTERNAL_DEPEND);

#ifdef HAVE_SEND_FD
  /* Defined above */
  add_efun("send_fd", f_send_fd, "function(int,int:int)", OPT_EXTERNAL_DEPEND);
#endif

  add_efun("parse_accessed_database", f_parse_accessed_database,
	   "function(string:array)", OPT_TRY_OPTIMIZE);

  add_efun("_dump_obj_table", f__dump_obj_table, "function(:array(array))", 
	   OPT_EXTERNAL_DEPEND);

  add_efun("parse_html",f_parse_html,
	   "function(string,mapping(string:function(string|void,mapping(string:string)|void,mixed ...:string)),mapping(string:function(string|void,mapping(string:string)|void,string|void,mixed ...:string)),mixed ...:string)",
	   0);

  add_efun("parse_html_lines",f_parse_html_lines,
	   "function(string,mapping(string:function(string|void,mapping(string:string)|void,int|void,mixed ...:string)),mapping(string:function(string|void,mapping(string:string)|void,string|void,int|void,mixed ...:string)),mixed ...:string)",
	   0);
  
#ifdef HAVE_PERROR
  add_efun("real_perror",f_real_perror, "function(:void)",
	   OPT_EXTERNAL_DEPEND|OPT_SIDE_EFFECT);
#endif

  add_efun("discdate", f_discdate, "function(int:array)", 0);
  add_efun("stardate", f_stardate, "function(int,void|int:int)", 0);

  add_efun("timezone", f_timezone, "function(:int)", 0);
  add_efun("get_all_active_fd", f_get_all_active_fd, "function(:array(int))",
	   OPT_EXTERNAL_DEPEND);
  add_efun("nice", f_nice, "function(int:int)",
	   OPT_EXTERNAL_DEPEND|OPT_SIDE_EFFECT);
#if 0
  add_efun("name_process", f_name_process, "function(string:void)", 0);
#endif
  add_efun("fd_info", f_fd_info, "function(int:string)", OPT_EXTERNAL_DEPEND);
  add_efun("mark_fd", f_mark_fd, "function(int,void|mixed:mixed)",
	   OPT_EXTERNAL_DEPEND|OPT_SIDE_EFFECT);

  /* timezone() needs */
  { 
    time_t foo = (time_t)0;
    struct tm *g;

    g = localtime(&foo);
#ifndef HAVE_INT_TIMEZONE
    _tz = g->tm_gmtoff;
#endif
  }

  init_udp();
  init_accessdb_program(); /* Accessed database */

#ifdef ENABLE_STREAMED_PARSER
  start_new_program();
  add_storage( sizeof (struct streamed_parser) );
  add_function( "init", streamed_parser_set_data,
		"function(mapping(string:function(string,mapping(string:string),mixed:mixed)),mapping(string:function(string,mapping(string:string),string,mixed:mixed)),mapping(string:function(string,mixed:mixed)):void)", 0 );
  add_function( "parse", streamed_parser_parse, "function(string,mixed:string)", 0 );
  add_function( "finish", streamed_parser_finish, "function(void:string)", 0 );
  set_init_callback( streamed_parser_init );
  set_exit_callback( streamed_parser_destruct );
   
  streamed_parser = end_program();
  add_program_constant("streamed_parser", streamed_parser,0);
#endif /* ENABLE_STREAMED_PARSER */
}


void pike_module_exit(void)
{
  int i;

  free_string(empty_string.u.string);

#ifdef ENABLE_STREAMED_PARSER
  if(streamed_parser)
  {
    free_program(streamed_parser);
    streamed_parser=0;
  }
#endif /* ENABLE_STREAMED_PARSER */

  for(i=0; i<MAX_OPEN_FILEDESCRIPTORS; i++)
  {
    if(fd_marks[i])
    {
      free_string(fd_marks[i]);
      fd_marks[i]=0;
    }
  }
}

