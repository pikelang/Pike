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

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "stralloc.h"
#include "global.h"
#include "types.h"
#include "macros.h"
#include "object.h"
#include "add_efun.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "array.h"
#include "builtin_efuns.h"

#include <pwd.h>

#include "spider.h"
#include "conf.h"

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif


#define MAX_PARSE_RECURSE 1024

void do_html_parse(struct lpc_string *ss,
		   struct mapping *cont,struct mapping *single,
		   int *strings,int recurse_left,
		   struct array *extra_args);

extern void f_parse_tree(INT32 argc);

void f_parse_accessed_database(INT32 args)
{
  int cnum=0, i, num=0;

  struct array *arg;
  if(args != 1)
    error("Wrong number of arguments to parse_accessed_database(string)\n");

  push_string(make_shared_string("\n"));
  f_explode(2);
  arg = sp[-1].u.array;
  arg->refs++;
  /* The initial string is gone, but the array is there now. */
  pop_n_elems(args); 

  for (i = 0; i < arg->size; i++)
  {
    int j=0,k;
    char *s;
    s=(char *)(SHORT_ITEM(arg)[i].string->str);
    k=(SHORT_ITEM(arg)[i].string->len);
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

#if defined(__svr4__) && defined(sun)
#define SOLARIS
#endif

#ifdef SOLARIS
#include <errno.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/conf.h>
#include <stropts.h>

#if 0
void f_send_fd(INT32 args)
{
  struct iovec iov; 
  struct msghdr msg;
  int sock_fd, fd;

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

  while(sendmsg(sock_fd, &msg, 0) == -1)
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
#else
void f_send_fd(INT32 args)
{
  int sock_fd, fd;

  if(args != 2) error("RTSL\n");

  sock_fd = sp[-args].u.integer;
  fd =  sp[-args+1].u.integer;
  pop_stack();
  pop_stack();

  while(ioctl(sock_fd, I_SENDFD, fd) == -1)
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

void f_parse_html(INT32 args)
{
  struct lpc_string *ss;
  struct mapping *cont,*single;
  int strings;
  struct array *extra_args;
   
  if (args<3||
      sp[-args].type!=T_STRING||
      sp[1-args].type!=T_MAPPING||
      sp[2-args].type!=T_MAPPING)
    error("Bad argument(s) to parse_html.\n");

  ss=sp[-args].u.string;
  sp[-args].type=T_INT;

  single=sp[1-args].u.mapping; 
  cont=sp[2-args].u.mapping; 
  cont->refs++;
  single->refs++;

  if (args>3)
  {
    f_aggregate(args-3);
    extra_args=sp[-1].u.array;
    extra_args->refs++;
    pop_stack();
  }
  else extra_args=NULL;

  pop_n_elems(3);

  strings=0;
  do_html_parse(ss,cont,single,&strings,MAX_PARSE_RECURSE,extra_args);

  if (extra_args) free_array(extra_args);

  free_mapping(cont);
  free_mapping(single);
  f_sum(strings);
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
     if(i>j){\
       push_string(make_shared_binary_string(s+j,i-j));\
       strs++;\
       j=i;\
     } }while(0)
#define SKIP_SPACE()  while (i<len && s[i]!='>' && isspace(s[i])) i++

int extract_word(char *s, int i, int len)
{
  int inquote = 0;
  char endquote = 0;
  int j;      /* Start character for this word.. */
  int strs = 0;

  SKIP_SPACE();
  j=i;

  /* Should we allow "foo"bar'gazonk' ? We do now. */
  
  for(;i<len; i++)
  {
    switch(s[i])
    {
    case '-':  
      if(!endquote) 
	break;

    case ' ':
    case '\t':
    case '\n':
    case '\r':
    case '>':
    case '=':
      if(!inquote)
	goto done;
      break;

    case '"':
      if(inquote)
      {
	if(endquote=='"')
	{
	  PUSH();
	  inquote = 0;
	}
      }
      else
      {
	PUSH();
	j=i+1;
	inquote = 1;
	endquote = '"';
      }
      break;

    case '\'':
      if(inquote)
      {
	if(endquote == '\'')
	{
	  PUSH();
	  inquote = 0;
	}
      }
      else
      {
	PUSH();
	j=i+1;
	inquote = 1;
	endquote = '\'';
      }
      break;

    default:
      if(!inquote)
      {
	if(s[i] == start_quote_character)
	{
	  PUSH();
	  j=i+1;
	  inquote = 1;
	  endquote = end_quote_character;
	}
      } else if(endquote == end_quote_character) {
	if(s[i] == endquote)
	{
	  if(!--inquote)
	    PUSH();
	} else if(s[i] == start_quote_character) {
	  inquote++;
	}
      }
      break;
    }
  }
done:
  if(!strs || i-j > 2)
    PUSH();
  if(strs > 1)
  {
    f_sum(strs);
  } else if(strs == 0) {
    push_text("");
  }
  SKIP_SPACE();
  return i;
}
#undef PUSH
#undef SKIP_SPACE


int push_parsed_tag(char *s,int len)
{
  int i=0;
  struct svalue *oldsp;
  /* Find X=Y pairs. */
  oldsp = sp;

  while (i<len && s[i]!='>')
  {
    int oldi;
    oldi = i;
    i = extract_word(s, i, len);
    f_lower_case(1);            /* Since SGML want's us to... */
    if (i+1 >= len || (s[i] != '=') || (s[i]=='>') || (s[i+1]=='>'))
    {
      /* No 'Y' part here. Assign to 'X' */
      assign_svalue_no_free(sp,sp-1);
      sp++;
    } else {
      i = extract_word(s, i+1, len);
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

  if(s[len] != '>' && s[len] != ' ')
    return 0;

  while(len--) if(tolower(*(t++)) != tolower(*(s++)))
    return 0;

  return 1;
}

int find_endtag(struct lpc_string *tag, char *s, int len, int *aftertag)
{
  int num=1;

  int i,j;

  for (i=j=0; i < len; i++)
  {
    for (; i<len && s[i]!='<'; i++);
    if (i>=len) break;
    j=i++;
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

void do_html_parse(struct lpc_string *ss,
		   struct mapping *cont,struct mapping *single,
		   int *strings,int recurse_left,
		   struct array *extra_args)
{
  int i,j,k,l,m,len,last;
  unsigned char *s;
  struct svalue sval1,sval2;
  struct lpc_string *ss2;
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

  s=(unsigned char *)ss->str;
  len=ss->len;

  last=0;
  for (i=0; i<len-1;)
  {
    if (s[i]=='<')
    {
      /* skip all spaces */
      i++;
      for (j=i; j<len && s[j]!='>' && !isspace(s[j]); j++);

      if (j==len) break; /* end of string */

      push_string(make_shared_binary_string((char *)s+i, j-i));
      f_lower_case(1);
      sval2.u.string = sp[-1].u.string;
      sval2.u.string->refs++;
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
	assign_svalue_no_free(sp++,&sval2);
	k=push_parsed_tag(s+j,len-j); 
	if (extra_args)
	{
	  extra_args->refs++;
	  push_array_items(extra_args);
	}

	apply_svalue(&sval1,2+(extra_args?extra_args->size:0));
	if (sp[-1].type==T_STRING)
	{
	  free_svalue(&sval1);
	  free_svalue(&sval2);
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
	  free_svalue(&sval2);
	  free_svalue(&sval1);

	  f_implode(1);
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

	  free_svalue(&sval1);
	  continue;
	}
	pop_stack();
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
	  extra_args->refs++;
	  push_array_items(extra_args);
	}

	apply_svalue(&sval1,3+(extra_args?extra_args->size:0));

	if (sp[-1].type==T_STRING)
	{
	  free_svalue(&sval1);
	  free_svalue(&sval2);
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
	  free_svalue(&sval1);
	  free_svalue(&sval2);

	  f_implode(1);
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

void f_do_match(INT32 args)
{
  struct lpc_string *match,*str;
  struct array *a;
  int i;
  struct svalue *sval;

  if (args<2||
      sp[1-args].type!=T_STRING||
      (sp[-args].type!=T_STRING &&
       sp[-args].type!=T_ARRAY))
    error("Illegal arguments to do_match\n");

  copy_shared_string(match,sp[1-args].u.string);
  pop_n_elems(args-1);

  if (sp[-1].type==T_ARRAY)
  {
    a=sp[-1].u.array;
    a->refs++;
    pop_stack();
    i=a->size;
    if (!i) { push_int(0); return; }
    push_array_items(a);
    while (i--)
    {
      match->refs++;
      push_string(match);
      f_do_match(2);
      if (sp[-1].type!=T_INT)
      {
	sval=sp-1;
	sp--;
	pop_n_elems(i);
	sp[0]=*sval;
	sp++;
	free_string(match);
	return;
      }
      pop_stack();
    }
    push_int(0);
    free_string(match);
    return;
  }
  copy_shared_string(str,sp[-1].u.string);
  pop_stack();
   
  if (does_match(str->str,str->len,match->str,match->len)) 
  {
    push_string(str); 
    free_string(match); 
    return; 
  }
  push_int(0);
  free_string(str);
  free_string(match);
}

#if !HAVE_INT_TIMEZONE
int _tz;
#else
extern long int timezone;
#endif



void f_localtime(INT32 args)
{
  struct tm *tm;
  time_t t;
  if (args<1||
      sp[-1].type!=T_INT)
    error("Illegal argument to localtime");
  t=sp[-1].u.integer;
  tm=localtime(&t);
  pop_stack();
  push_string(make_shared_string("sec"));
  push_int(tm->tm_sec);
  push_string(make_shared_string("min"));
  push_int(tm->tm_min);
  push_string(make_shared_string("hour"));
  push_int(tm->tm_hour);

  push_string(make_shared_string("mday"));
  push_int(tm->tm_mday);
  push_string(make_shared_string("mon"));
  push_int(tm->tm_mon);
  push_string(make_shared_string("year"));
  push_int(tm->tm_year);

  push_string(make_shared_string("wday"));
  push_int(tm->tm_wday);
  push_string(make_shared_string("yday"));
  push_int(tm->tm_yday);
  push_string(make_shared_string("isdst"));
  push_int(tm->tm_isdst);

  push_string(make_shared_string("timezone"));
#if !HAVE_INT_TIMEZONE
  push_int(_tz);
#else
  push_int(timezone);
#endif
  f_aggregate_mapping(20);
}

void f_do_setuid(INT32 args)
{
  struct passwd *pw;
  int id;
  if(!args)
    error("Set uid to what?\n");

  if(sp[-1].type != T_INT)
    error("Set uid to _what_?\n");

  if(sp[-1].u.integer == -1)
  {
    pw = getpwnam("nobody");
    id = pw->pw_uid;
  } else
    id = sp[-1].u.integer;

  setuid(id);
  pop_n_elems(args-1);
}

void f_do_setgid(INT32 args)
{
  struct passwd *pw;
  int id;
  if(!args)
    error("Set gid to what?\n");

  if(sp[-1].type != T_INT)
    error("Set gid to _what_?\n");

  if(sp[-1].u.integer == -1)
  {
    pw = getpwnam("nobody");
    id = pw->pw_gid;
  } else
    id = sp[-1].u.integer;
  
  setgid(id);
  pop_n_elems(args-1);
}
#if defined(HAVE_SETEUID) || defined(HAVE_SETRESUID)
void f_do_seteuid(INT32 args)
{
  struct passwd *pw;
  int id;
  if(!args)
    error("Set uid to what?\n");

  if(sp[-1].type != T_INT)
    error("Set uid to _what_?\n");

  if(sp[-1].u.integer == -1)
  {
    pw = getpwnam("nobody");
    id = pw->pw_uid;
  } else
    id = sp[-1].u.integer;
#ifdef HAVE_SETEUID
  seteuid(id);
#else
  setresuid(-1, id, -1);
#endif
  pop_n_elems(args-1);
}
#endif

#if defined(HAVE_SETEGID) || defined(HAVE_SETRESGID)
void f_do_setegid(INT32 args)
{
  struct passwd *pw;
  int id;
  if(!args)
    error("Set gid to what?\n");

  if(sp[-1].type != T_INT)
    error("Set gid to _what_?\n");
  if(sp[-1].u.integer == -1)
  {
    pw = getpwnam("nobody");
    id = pw->pw_gid;
  } else
    id = sp[-1].u.integer;

#ifdef HAVE_SETEGID
  setegid(id);
#else
  setresgid(-1, id, -1);
#endif
  pop_n_elems(args-1);
}
#endif


void f_timezone(INT32 args)
{
  pop_n_elems(args);
#if !HAVE_INT_TIMEZONE
  push_int(_tz);
#else
  push_int(timezone);
#endif
}

#ifdef HAVE_SYSLOG

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

#ifdef HAVE_SYS_SYSLOG_H
#ifndef HAVE_SYSLOG_H
#include <sys/syslog.h>
#endif
#endif


#ifndef LOG_PID
#define LOG_PID 0
#endif
#ifndef LOG_AUTH
#define LOG_AUTH 0
#endif
#ifndef LOG_AUTHPRIV
#define LOG_AUTHPRIV 0
#endif
#ifndef LOG_CRON
#define LOG_CRON 0
#endif
#ifndef LOG_DAEMON
#define LOG_DAEMON 0
#endif
#ifndef LOG_KERN
#define LOG_KERN 0
#endif
#ifndef LOG_LOCAL0
#define LOG_LOCAL0 0
#endif
#ifndef LOG_LOCAL1
#define LOG_LOCAL1 0
#endif
#ifndef LOG_LOCAL2
#define LOG_LOCAL2 0
#endif
#ifndef LOG_LOCAL3
#define LOG_LOCAL3 0
#endif
#ifndef LOG_LOCAL4
#define LOG_LOCAL4 0
#endif
#ifndef LOG_LOCAL5
#define LOG_LOCAL5 0
#endif
#ifndef LOG_LOCAL6
#define LOG_LOCAL6 0
#endif
#ifndef LOG_LOCAL7
#define LOG_LOCAL7 0
#endif
#ifndef LOG_LPR
#define LOG_LPR 0
#endif
#ifndef LOG_MAIL
#define LOG_MAIL 0
#endif
#ifndef LOG_NEWS
#define LOG_NEWS 0
#endif
#ifndef LOG_SYSLOG
#define LOG_SYSLOG 0
#endif
#ifndef LOG_USER
#define LOG_USER 0
#endif
#ifndef LOG_UUCP
#define LOG_UUCP 0
#endif
#ifndef LOG_CONS
#define LOG_CONS 0
#endif
#ifndef LOG_NDELAY
#define LOG_NDELAY 0
#endif
#ifndef LOG_PERROR
#define LOG_PERROR 0
#endif
void f_openlog(INT32 args)
{
  int option=0, facility=0, i;
  if(args < 3)
    error("Wrong number of arguments to openlog(string,int,int)\n");
  if(sp[-args].type != T_STRING
     &&sp[-args+1].type != T_INT
     &&sp[-args+2].type != T_INT)
    error("Wrong type of arguments to openlog(string,int,int)\n");

  i=sp[-args+1].u.integer;
  
  if(i & (1<<0)) option |= LOG_CONS;
  if(i & (1<<1)) option |= LOG_NDELAY;
  if(i & (1<<2)) option |= LOG_PERROR;
  if(i & (1<<3)) option |= LOG_PID;

  i=sp[-args+2].u.integer;

  if(i & (1<<0)) facility |= LOG_AUTH; /* Don't use */
  if(i & (1<<1)) facility |= LOG_AUTHPRIV;
  if(i & (1<<2)) facility |= LOG_CRON;
  if(i & (1<<3)) facility |= LOG_DAEMON;
  if(i & (1<<4)) facility |= LOG_KERN;
  if(i & (1<<5)) facility |= LOG_LOCAL0;
  if(i & (1<<6)) facility |= LOG_LOCAL1;
  if(i & (1<<7)) facility |= LOG_LOCAL2;
  if(i & (1<<8)) facility |= LOG_LOCAL3;
  if(i & (1<<9)) facility |= LOG_LOCAL4;
  if(i & (1<<10)) facility |= LOG_LOCAL5;
  if(i & (1<<11)) facility |= LOG_LOCAL6;
  if(i & (1<<12)) facility |= LOG_LOCAL7;
  if(i & (1<<13)) facility |= LOG_LPR;
  if(i & (1<<14)) facility |= LOG_MAIL;
  if(i & (1<<15)) facility |= LOG_NEWS;
  if(i & (1<<16)) facility |= LOG_SYSLOG;
  if(i & (1<<17)) facility |= LOG_USER;
  if(i & (1<<18)) facility |= LOG_UUCP;
  
  openlog((char *)sp[-args].u.string->str, option, facility);
}

void f_syslog(INT32 args)
{
  int pri=0, i;

  if(args < 2)
    error("Wrong number of arguments to syslog(int, string)\n");
  if(sp[-args].type != T_INT ||
     sp[-args+1].type != T_STRING)
    error("Wrong type of arguments to syslog(int, string)\n");

  reference_shared_string(sp[-args+1].u.string);
  push_string(sp[-args+1].u.string);
  push_string(make_shared_string("%"));
  push_string(make_shared_string("%%"));
  f_replace(3);

  i=sp[-args].u.integer;
  if(i & (1<<0)) pri |= LOG_EMERG;
  if(i & (1<<1)) pri |= LOG_ALERT;
  if(i & (1<<2)) pri |= LOG_CRIT;
  if(i & (1<<3)) pri |= LOG_ERR;
  if(i & (1<<4)) pri |= LOG_WARNING;
  if(i & (1<<5)) pri |= LOG_NOTICE;
  if(i & (1<<6)) pri |= LOG_INFO;
  if(i & (1<<6)) pri |= LOG_DEBUG;
  
  syslog(pri, (char *)sp[-1].u.string->str,"%s","%s","%s","%s","%s","%s",
	 "%s","%s","%s","%s");
  pop_n_elems(args);
}

void f_closelog(INT32 args)
{
  closelog();
  pop_n_elems(args);
}
#endif


#ifdef HAVE_STRERROR
void f_strerror(INT32 args)
{
  char *s;
  if(!args) 
    s=NULL;
  else
    s=strerror(sp[-args].u.integer);
  pop_n_elems(args);
  if(s)
    push_text(s);
  else
    push_int(0);
}
#endif

#ifdef HAVE_PERROR
void f_real_perror(INT32 args)
{
  pop_n_elems(args);
  perror(NULL);
}
#else
void f_real_perror(INT32 args)
{
  pop_n_elems(args);
}
#endif

void f_get_all_active_fd(INT32 args)
{
  int i,fds;
  struct stat foo;
  pop_n_elems(args);
  for (i=fds=0; i<MAX_OPEN_FILEDESCRIPTORS; i++)
    if (!fstat(i,&foo))
    {
      push_int(i);
      fds++;
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

struct lpc_string *fd_marks[MAX_OPEN_FILEDESCRIPTORS];

void f_mark_fd(INT32 args)
{
  int fd;
  struct lpc_string *s;
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
    struct sockaddr_in addr;
    char *tmp;
    char buf[20];

    pop_stack();
    if(fd_marks[fd])
    {
      fd_marks[fd]->refs++;
      push_string(fd_marks[fd]);
    } else {
      push_int(0);
    }

    len=sizeof(addr);
    if(! getsockname(fd, (struct sockaddr *) &addr, &len))
    {
      tmp=inet_ntoa(addr.sin_addr);
      push_string(make_shared_string(" Local:"));
      push_string(make_shared_string(tmp));
      sprintf(buf,".%d",(int)(ntohs(addr.sin_port)));
      push_string(make_shared_string(buf));
      f_sum(4);
    }

    if(! getpeername(fd, (struct sockaddr *) &addr, &len))
    {
      push_string(make_shared_string(" Remote:"));
      tmp=inet_ntoa(addr.sin_addr);
      push_string(make_shared_string(tmp));
      sprintf(buf,".%d",(int)(ntohs(addr.sin_port)));
      push_string(make_shared_string(buf));
      f_sum(4);
    }

    return;
  }
  s=sp[-args+1].u.string;
  s->refs++;
  if(fd_marks[fd])
    free_string(fd_marks[fd]);
  fd_marks[fd]=s;
  pop_n_elems(args);
  push_int(0);
}

#define get(X) void f_##X(INT32 args){ pop_n_elems(args); push_int((INT32)X()); }

#ifdef HAVE_GETUID
get(getuid)
#endif
#ifdef HAVE_GETEUID
get(geteuid)
#endif
#ifdef HAVE_GETGID
get(getgid)
#endif
#ifdef HAVE_GETEGID
get(getegid)
#endif
get(getpid)
#ifdef HAVE_GETPPID
get(getppid)
#endif
#ifdef HAVE_GETPGRP
get(getpgrp)
#endif

#undef get


void f_chroot(INT32 args)
{
  int res;
  if(args != 1) 
    error("Wrong number of args to chroot(string|object newroot)\n");
  if(sp[-args].type == T_STRING)
  {
    res = chroot((char *)sp[-args].u.string->str);
    pop_stack();
    push_int(!res);
    return;
  }
#ifdef HAVE_FCHROOT
  else if(sp[-args].type == T_OBJECT) {
    int fd;
    apply(sp[-args].u.object, "query_fd", 0);
    fd=sp[-1].u.integer;
    pop_stack();
    res=fchroot(fd);
    pop_stack();
    push_int(!res);
    return;
  }
  error("Wrong type of argument to chroot(string|object newroot)\n");
#else
  error("Wrong type of argument to chroot(string newroot)\n");
#endif
}

#ifdef HAVE_UNAME
#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif
void f_uname(INT32 args)
{
  struct svalue *old_sp;
  struct utsname foo;

  pop_n_elems(args);
  old_sp = sp;

  if(uname(&foo) < 0)
    error("uname() system call failed.\n");

  push_text("sysname");
  push_text(foo.sysname);

  push_text("nodename");
  push_text(foo.nodename);

  push_text("release");
  push_text(foo.release);

  push_text("version");
  push_text(foo.version);

  push_text("machine");
  push_text(foo.machine);

  f_aggregate_mapping(sp-old_sp);
}
#endif


#if defined(HAVE_GETHOSTNAME) && !defined(SOLARIS)
void f_gethostname(INT32 args)
{
  char name[1024];
  pop_n_elems(args);
  if((gethostname(name, 1024) != -1))
  {
    push_text(name);
    return;
  }
  error("gethostname() system call failed?\n");
}
#else
#ifdef HAVE_UNAME
void f_gethostname(INT32 args)
{
  struct utsname foo;
  pop_n_elems(args);
  if(uname(&foo) < 0)
    error("uname() system call failed.\n");
  push_text(foo.nodename);
}
#endif
#endif

#ifdef HAVE_GETHOSTBYADDR
void f_gethostbyaddr(INT32 args)
{
  u_long addr;
  struct hostent *hp;
  char **p;
  struct svalue *old_sp;
 
  if ((args != 1) || sp[-1].type != T_STRING) 
    error("gethostbyaddr(IP-ADDRESS)\n");

  if ((int)(addr = inet_addr(sp[-1].u.string->str)) == -1) 
    error("IP-address must be of the form a.b.c.d\n");

  pop_n_elems(args);
  old_sp = sp;
  
  hp = gethostbyaddr((char *)&addr, sizeof (addr), AF_INET);
  if(!hp)
  {
    push_int(0);
    return;
  }
  
  for (p = hp->h_addr_list; *p != 0; p++) {
    int nelem = 0;
    struct in_addr in;
    char **q;
 
    memcpy(&in.s_addr, *p, sizeof (in.s_addr));
    push_text(inet_ntoa(in)); nelem++;

    for (q = hp->h_aliases; *q != 0; q++)
    {
      push_text(*q); nelem++;
    }
    f_aggregate(nelem);
  }
  f_aggregate(sp-old_sp);
}  

#endif

#ifdef HAVE_GETHOSTBYNAME
void f_gethostbyname(INT32 args)
{
  struct hostent *hp;
  char **p;
  struct svalue *old_sp;
 
  if ((args != 1) || sp[-1].type != T_STRING) 
    error("gethostbyname(NAME)\n");

  hp = gethostbyname((char *)sp[-1].u.string->str);

  pop_n_elems(args);
  old_sp = sp;
  
  if(!hp)
  {
    push_int(0);
    return;
  }
  
  for (p = hp->h_addr_list; *p != 0; p++) {
    int nelem = 0;
    struct in_addr in;
    char **q;
 
    memcpy(&in.s_addr, *p, sizeof (in.s_addr));
    push_text(inet_ntoa(in)); nelem++;

    for (q = hp->h_aliases; *q != 0; q++)
    {
      push_text(*q); nelem++;
    }
    f_aggregate(nelem);
  }
  f_aggregate(sp-old_sp);
}  
#endif

#ifdef HAVE_PASSWD_H
# include <passwd.h>
#endif

#ifdef HAVE_PWD_H
# include <pwd.h>
#endif

#ifdef HAVE_SHADOW_H
# include <shadow.h>
#endif

#if defined(HAVE_GETPWNAM) || defined(HAVE_GETPWUID) || defined(HAVE_SETPWENT) || defined(HAVE_ENDPWENT) || defined(HAVE_GETPWENT)
static void push_pwent(struct passwd *ent)
{
  push_text(ent->pw_name);

#ifdef HAVE_GETSPNAM
  if(!strcmp(ent->pw_passwd, "x"))
  {
    struct spwd *foo;
    if((foo = getspnam(ent->pw_name)))
    {
      push_text(foo->sp_pwdp);
    } else {
      push_text("x");
    }
  } else 
#endif
  {
    push_text(ent->pw_passwd);
  }
  push_int(ent->pw_uid);
  push_int(ent->pw_gid);
  push_text(ent->pw_gecos);
  push_text(ent->pw_dir);
  push_text(ent->pw_shell);
  f_aggregate(7);
}
#endif

#ifdef HAVE_GETPWNAM
void f_getpwnam(INT32 args)
{
  struct passwd *foo;
  if(args!=1) error("Must pass one string to getpwnam(NAME)\n");
  if(sp[-1].type != T_STRING)  error("Must pass string to getpwnam(NAME)\n");
  foo = getpwnam((const char *)sp[-1].u.string->str);
  pop_stack();
  push_pwent(foo);
}
#endif

#ifdef HAVE_GETPWUID
void f_getpwuid(INT32 args)
{
  struct passwd *foo;
  if(args!=1) error("Must pass one integer to getpwuid(NAME)\n");
  foo = getpwuid(sp[-1].u.integer);
  pop_stack();
  push_pwent(foo);
}
#endif

#ifdef HAVE_SETPWENT
void f_setpwent(INT32 args)
{
  setpwent();
  pop_n_elems(args);
  push_int(0);
}
#endif

#ifdef HAVE_ENDPWENT
void f_endpwent(INT32 args)
{
  setpwent();
  pop_n_elems(args);
  push_int(0);
}
#endif

#ifdef HAVE_GETPWENT
void f_getpwent(INT32 args)
{
  struct passwd *foo;
  pop_n_elems(args);
  foo = getpwent();
  if(!foo)
  {
    push_int(0);
    return;
  }
  push_pwent(foo);
}
#endif



void init_spider_efuns(void) 
{

  /* This _will_ leak some memory. It is supposed to. These
   * make_shared_string's are here to define a few very commonly used
   * strings, to speed spinner up a little.  
   */
  make_shared_string("HTTP/1.0");
  make_shared_string("GET");
  make_shared_string("POST");

#ifdef HAVE_GETPWNAM
  add_efun("getpwnam", f_getpwnam, "function(string:array)", 
	   OPT_EXTERNAL_DEPEND);
#endif
#ifdef HAVE_GETPWUID
  add_efun("getpwuid", f_getpwuid, "function(int:array)", OPT_EXTERNAL_DEPEND);
#endif
#ifdef HAVE_GETPWENT
  add_efun("getpwent", f_getpwent, "function(void:array)",
	   OPT_EXTERNAL_DEPEND);
#endif
#ifdef HAVE_ENDPWENT
  add_efun("endpwent", f_endpwent, "function(void:int)", OPT_EXTERNAL_DEPEND);
#endif
#ifdef HAVE_SETPWENT
  add_efun("setpwent", f_setpwent, "function(void:int)", OPT_EXTERNAL_DEPEND);
#endif


  add_efun("set_start_quote", f_set_start_quote, "function(int:int)",
	   OPT_EXTERNAL_DEPEND);

  add_efun("set_end_quote", f_set_end_quote, "function(int:int)", 
	   OPT_EXTERNAL_DEPEND);

#ifdef HAVE_GETHOSTBYNAME
  add_efun("gethostbyname", f_gethostbyname, "function(string:array)",
	   OPT_TRY_OPTIMIZE);
#endif

#ifdef HAVE_GETHOSTBYADDR
  add_efun("gethostbyaddr", f_gethostbyaddr, "function(string:array)",
	   OPT_TRY_OPTIMIZE);
#endif


#if defined(HAVE_GETHOSTNAME) || defined(HAVE_UNAME)
  add_efun("gethostname", f_gethostname, "function(:string)",OPT_TRY_OPTIMIZE);
#endif

#ifdef HAVE_UNAME
  add_efun("uname", f_uname, "function(:mapping)", OPT_TRY_OPTIMIZE);
#endif

#ifdef SOLARIS
  add_efun("send_fd", f_send_fd, "function(int,int:int)", OPT_EXTERNAL_DEPEND);
#endif

#ifdef HAVE_GETUID
  add_efun("getuid", f_getuid, "function(:int)", OPT_EXTERNAL_DEPEND);
#endif

#ifdef HAVE_GETEUID
  add_efun("geteuid", f_geteuid, "function(:int)", OPT_EXTERNAL_DEPEND);
#endif

#ifdef HAVE_GETGID
  add_efun("getgid", f_getgid, "function(:int)", OPT_EXTERNAL_DEPEND);
#endif

#ifdef HAVE_GETEGID
  add_efun("getegid", f_getegid, "function(:int)", OPT_EXTERNAL_DEPEND);
#endif

  add_efun("getpid", f_getpid, "function(:int)", OPT_EXTERNAL_DEPEND);

#ifdef HAVE_GETPPID
  add_efun("getppid", f_getppid, "function(:int)", OPT_EXTERNAL_DEPEND);
#endif

#ifdef HAVE_GETPGRP
  add_efun("getpgrp", f_getpgrp, "function(:int)", OPT_EXTERNAL_DEPEND);
#endif

  add_efun("chroot", f_chroot, "function(string|object:int)", 
	   OPT_EXTERNAL_DEPEND);

  add_efun("parse_accessed_database", f_parse_accessed_database,
	   "function(string:array)", OPT_TRY_OPTIMIZE);

#define DEBUG
#ifdef DEBUG
  add_efun("_string_debug", f__string_debug, "function(void|mixed:string)", 
	   OPT_EXTERNAL_DEPEND);

  add_efun("_dump_string_table", f__dump_string_table, 
	   "function(:array(array))",  OPT_EXTERNAL_DEPEND);

  add_efun("_num_dest_objects", f__num_dest_objects, "function(:int)", 
	   OPT_EXTERNAL_DEPEND);

  add_efun("_num_arrays", f__num_arrays, "function(:int)", 
	   OPT_EXTERNAL_DEPEND);

  add_efun("_num_objects", f__num_objects, "function(:int)", 
	   OPT_EXTERNAL_DEPEND);

  add_efun("_num_mappings", f__num_mappings, "function(:int)", 
	   OPT_EXTERNAL_DEPEND);

  add_efun("_dump_obj_table", f__dump_obj_table, "function(:array(array))", 
	   OPT_EXTERNAL_DEPEND);
#endif

  add_efun("parse_html",f_parse_html,
	   "function(string,mapping(string:function(string,mapping(string:string),mixed ...:string)),mapping(string:function(string,mapping(string:string),string,mixed ...:string)),mixed ...:string)",
	   0);

  add_efun("do_match",f_do_match,
	   "function(string|array(string),string:string)",OPT_TRY_OPTIMIZE);

  add_efun("localtime",f_localtime,
	   "function(int:mapping(string:mixed))",OPT_EXTERNAL_DEPEND);

#ifdef HAVE_STRERROR
  add_efun("strerror",f_strerror, "function(int:string)",OPT_TRY_OPTIMIZE);
#endif

  add_efun("real_perror",f_real_perror, "function(:void)",OPT_EXTERNAL_DEPEND);

#ifdef HAVE_SYSLOG
  add_efun("openlog", f_openlog, "function(string,int,int:void)", 0);
  add_efun("syslog", f_syslog, "function(int,string:void)", 0);
  add_efun("closelog", f_closelog, "function(:void)", 0);
#endif

  add_efun("discdate", f_discdate, "function(int:array)", 0);
  add_efun("stardate", f_stardate, "function(int,void|int:int)", 0);

  add_efun("parse_tree", f_parse_tree, "function(string:array(string))", 0);

  add_efun("setuid", f_do_setuid, "function(int:void)", 0);
  add_efun("setgid", f_do_setgid, "function(int:void)", 0);
#if defined(HAVE_SETEUID) || defined(HAVE_SETRESUID)
  add_efun("seteuid", f_do_seteuid, "function(int:void)", 0);
#endif
#if defined(HAVE_SETEGID) || defined(HAVE_SETRESGID)
  add_efun("setegid", f_do_setegid, "function(int:void)", 0);
#endif
  add_efun("timezone",f_timezone,"function(:int)",0);
  add_efun("get_all_active_fd",f_get_all_active_fd,"function(:array(int))",0);
  add_efun("fd_info",f_fd_info,"function(int:string)",0);
  add_efun("mark_fd",f_mark_fd,"function(int,void|mixed:mixed)",0);

  /* timezone() needs */
  { 
    time_t foo;
    struct tm *g;
    /* Purify wants */
    foo=(time_t)0; 

    g=localtime(&foo);
#if !HAVE_INT_TIMEZONE
    _tz = g->tm_gmtoff;
#endif
  }
}

void init_spider_programs() {}

void exit_spider(void)
{
  int i;
  for(i=0; i<MAX_OPEN_FILEDESCRIPTORS; i++)
    if(fd_marks[i])
      free_string(fd_marks[i]);
}
