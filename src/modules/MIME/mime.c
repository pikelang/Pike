#include "config.h"

#include "global.h"
RCSID("$Id: mime.c,v 1.1 1997/03/08 21:09:51 marcus Exp $");
#include "stralloc.h"
#include "types.h"
#include "macros.h"
#include "object.h"
#include "program.h"
#include "interpret.h"
#include "builtin_functions.h"
#include "error.h"

static void f_decode_base64(INT32 args);
static void f_encode_base64(INT32 args);
static void f_decode_qp(INT32 args);
static void f_encode_qp(INT32 args);
static void f_decode_uue(INT32 args);
static void f_encode_uue(INT32 args);

static void f_tokenize(INT32 args);
static void f_quote(INT32 args);

static char base64tab[64]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static char base64rtab[0x80-' '];
static char qptab[16]="0123456789ABCDEF";
static char qprtab[0x80-'0'];

#define CT_CTL     0
#define CT_WHITE   1
#define CT_ATOM    2
#define CT_SPECIAL 3
#define CT_LPAR    4
#define CT_RPAR    5
#define CT_LBRACK  6
#define CT_RBRACK  7
#define CT_QUOTE   8
unsigned char rfc822ctype[256];

/* Initialize and start module */
void pike_module_init(void)
{
  int i;
  memset(base64rtab, -1, sizeof(base64rtab));
  for(i=0; i<64; i++) base64rtab[base64tab[i]-' ']=i;
  memset(qprtab, -1, sizeof(qprtab));
  for(i=0; i<16; i++) qprtab[qptab[i]-'0']=i;
  for(i=10; i<16; i++) qprtab[qptab[i]-('0'+'A'-'a')]=i;

  memset(rfc822ctype, CT_ATOM, sizeof(rfc822ctype));
  for(i=0; i<32; i++) rfc822ctype[i]=CT_CTL;
  rfc822ctype[127]=CT_CTL;
  rfc822ctype[' ']=CT_WHITE;
  rfc822ctype['\t']=CT_WHITE;
  rfc822ctype['(']=CT_LPAR;
  rfc822ctype[')']=CT_RPAR;
  rfc822ctype['[']=CT_LBRACK;
  rfc822ctype[']']=CT_LBRACK;
  rfc822ctype['"']=CT_QUOTE;
  for(i=0; i<10; i++) rfc822ctype[(int)"<>@,;:\\/?="[i]]=CT_SPECIAL;

  add_function_constant("decode_base64", f_decode_base64, "function(string:string)", OPT_TRY_OPTIMIZE);
  add_function_constant("encode_base64", f_encode_base64, "function(string:string)", OPT_TRY_OPTIMIZE);
  add_function_constant("decode_qp", f_decode_qp, "function(string:string)", OPT_TRY_OPTIMIZE);
  add_function_constant("encode_qp", f_encode_qp, "function(string:string)", OPT_TRY_OPTIMIZE);
  add_function_constant("decode_uue", f_decode_uue, "function(string:string)", OPT_TRY_OPTIMIZE);
  add_function_constant("encode_uue", f_encode_uue, "function(string,void|string:string)", OPT_TRY_OPTIMIZE);

  add_function_constant("tokenize", f_tokenize, "function(string:array(string|int))", OPT_TRY_OPTIMIZE);
  add_function_constant("quote", f_quote, "function(array(string|int):string)", OPT_TRY_OPTIMIZE);

}

/* Restore and exit module */
void pike_module_exit(void)
{
}

static void f_decode_base64(INT32 args)
{
  if(args != 1)
    error("Wrong number of arguments to MIME.decode_base64()\n");
  else if(sp[-1].type != T_STRING)
    error("Wrong type of argument to MIME.decode_base64()\n");
  else {
    dynamic_buffer buf;
    char *src;
    INT32 cnt, d=1;
    int pads=0;

    buf.s.str=NULL;
    initialize_buf(&buf);

    for(src = sp[-1].u.string->str, cnt = sp[-1].u.string->len; cnt--; src++)
      if(*src>=' ' && base64rtab[*src-' ']>=0)
	if((d=(d<<6)|base64rtab[*src-' '])>=0x1000000) {
	  low_my_putchar( d>>16, &buf );
	  low_my_putchar( d>>8, &buf );
	  low_my_putchar( d, &buf );
	  d=1;
	}
	else ;
      else if(*src=='=') { pads++; d>>=2; }

    switch(pads) {
    case 1:
      low_my_putchar( d>>8, &buf );
    case 2:
      low_my_putchar( d, &buf );
    }

    pop_n_elems(1);
    push_string(low_free_buf(&buf));
  }
}

static int do_b64_encode(INT32 groups, unsigned char **srcp, char **destp)
{
  unsigned char *src=*srcp;
  char *dest=*destp;
  int g=0;
  while(groups--) {
    INT32 d = *src++<<8;
    d = (*src++|d)<<8;
    d |= *src++;
    *dest++ = base64tab[d>>18];
    *dest++ = base64tab[(d>>12)&63];
    *dest++ = base64tab[(d>>6)&63];
    *dest++ = base64tab[d&63];
    if(++g == 19) {
      *dest++ = 13;
      *dest++ = 10;
      g=0;
    }
  }
  *srcp = src;
  *destp = dest;
  return g;
}

static void f_encode_base64(INT32 args)
{
  if(args != 1)
    error("Wrong number of arguments to MIME.encode_base64()\n");
  else if(sp[-1].type != T_STRING)
    error("Wrong type of argument to MIME.encode_base64()\n");
  else {
    INT32 groups=(sp[-1].u.string->len+2)/3;
    int last=(sp[-1].u.string->len-1)%3+1;
    struct pike_string *str = begin_shared_string(groups*4+(groups/19)*2);
    unsigned char *src=(unsigned char *)sp[-1].u.string->str;
    char *dest=str->str;

    if(groups) {
      unsigned char tmp[3], *tmpp=tmp;
      int i;

      if(do_b64_encode(groups-1, &src, &dest)==18)
	str->len-=2;

      tmp[1]=tmp[2]=0;
      for(i=0; i<last; i++) tmp[i]=*src++;
      do_b64_encode(1, &tmpp, &dest);
      switch(last) {
      case 1:
	*--dest='=';
      case 2:
	*--dest='=';
      }
    }
    pop_n_elems(1);
    push_string(end_shared_string(str));
  }
}

static void f_decode_qp(INT32 args)
{
  if(args != 1)
    error("Wrong number of arguments to MIME.decode_qp()\n");
  else if(sp[-1].type != T_STRING)
    error("Wrong type of argument to MIME.decode_qp()\n");
  else {
    dynamic_buffer buf;
    char *src;
    INT32 cnt;

    buf.s.str=NULL;
    initialize_buf(&buf);

    for(src = sp[-1].u.string->str, cnt = sp[-1].u.string->len; cnt--; src++)
      if(*src == '=')
	if(cnt>0 && (src[1]==10 || src[1]==13)) {
	  if(src[1]==13) { --cnt; src++; }
	  if(cnt>0 && src[1]==10) { --cnt; src++; }
	} else if(cnt>=2 && src[1]>='0' && src[2]>='0' &&
		  qprtab[src[1]-'0']>=0 && qprtab[src[2]-'0']>=0) {
	  low_my_putchar( (qprtab[src[1]-'0']<<4)|qprtab[src[2]-'0'], &buf );
	  cnt -= 2;
	  src += 2;
	} else ;
      else
	low_my_putchar( *src, &buf );

    pop_n_elems(1);
    push_string(low_free_buf(&buf));
  }
}

static void f_encode_qp(INT32 args)
{
  if(args != 1)
    error("Wrong number of arguments to MIME.encode_qp()\n");
  else if(sp[-1].type != T_STRING)
    error("Wrong type of argument to MIME.encode_qp()\n");
  else {
    dynamic_buffer buf;
    unsigned char *src = (unsigned char *)sp[-1].u.string->str;
    INT32 cnt;
    int col=0;

    buf.s.str=NULL;
    initialize_buf(&buf);

    for(cnt = sp[-1].u.string->len; cnt--; src++) {
      if((*src >= 33 && *src <= 60) ||
	 (*src >= 62 && *src <= 126))
	low_my_putchar( *src, &buf );
      else {
	low_my_putchar( '=', &buf );
	low_my_putchar( qptab[(*src)>>4], &buf );
	low_my_putchar( qptab[(*src)&15], &buf );
	col += 2;
      }
      if(++col >= 73) {
	low_my_putchar( '=', &buf );
	low_my_putchar( 13, &buf );
	low_my_putchar( 10, &buf );
	col = 0;
      }
    }
    
    pop_n_elems(1);
    push_string(low_free_buf(&buf));
  }
}


static void f_decode_uue(INT32 args)
{
  if(args != 1)
    error("Wrong number of arguments to MIME.decode_uue()\n");
  else if(sp[-1].type != T_STRING)
    error("Wrong type of argument to MIME.decode_uue()\n");
  else {
    dynamic_buffer buf;
    char *src;
    INT32 cnt;

    buf.s.str=NULL;
    initialize_buf(&buf);

    src = sp[-1].u.string->str;
    cnt = sp[-1].u.string->len;

    while(cnt--)
      if(*src++=='b' && cnt>5 && !memcmp(src, "egin ", 5))
	break;
    if(cnt>=0)
      while(cnt--)
	if(*src++=='\n')
	  break;
    if(cnt<0) {
      pop_n_elems(1);
      push_int(0);
      return;
    }
    for(;;) {
      int l, g;
      if(cnt<=0 || (l=(*src++)-' ')>=64)
	break;
      --cnt;
      g=(l+2)/3;
      l-=g*3;
      if((cnt-=g*4)<0)
	break;
      while(g--) {
	INT32 d = ((*src++-' ')&63)<<18;
	d |= ((*src++-' ')&63)<<12;
	d |= ((*src++-' ')&63)<<6;
	d |= ((*src++-' ')&63);
	low_my_putchar( d>>16, &buf );
	low_my_putchar( d>>8, &buf );
	low_my_putchar( d, &buf );
      }
      while(l++)
	low_make_buf_space( -1, &buf );
      while(cnt-- && *src++!=10);
    }
    pop_n_elems(1);
    push_string(low_free_buf(&buf));
  }
}

static void do_uue_encode(INT32 groups, unsigned char **srcp, char **destp,
			  INT32 last)
{
  unsigned char *src=*srcp;
  char *dest=*destp;

  while(groups || last) {
    int g=(groups>=15? 15:groups);
    if(g<15) {
      *dest++ = ' '+(3*g+last);
      last = 0;
    } else
      *dest++ = ' '+(3*g);
    groups -= g;

    while(g--) {
      INT32 d = *src++<<8;
      d = (*src++|d)<<8;
      d |= *src++;
      *dest++ = ' '+(d>>18);
      *dest++ = ' '+((d>>12)&63);
      *dest++ = ' '+((d>>6)&63);
      *dest++ = ' '+(d&63);
    }

    if(groups || last) {
      *dest++ = 13;
      *dest++ = 10;
    }
  }
  *srcp = src;
  *destp = dest;
}

static void f_encode_uue(INT32 args)
{
  if(args != 1 && args != 2)
    error("Wrong number of arguments to MIME.encode_uue()\n");
  else if(sp[-args].type != T_STRING ||
	  (args == 2 && sp[-1].type != T_VOID && sp[-1].type != T_STRING &&
	   sp[-1].type != T_INT))
    error("Wrong type of argument to MIME.encode_uue()\n");
  else {
    struct pike_string *str;
    unsigned char *src = (unsigned char *) sp[-args].u.string->str;
    INT32 groups=(sp[-args].u.string->len+2)/3;
    int last=(sp[-args].u.string->len-1)%3+1;
    char *dest, *filename = "attachment";
    if(args == 2 && sp[-1].type == T_STRING)
      filename = sp[-1].u.string->str;
    str = begin_shared_string(groups*4+((groups+14)/15)*3+strlen(filename)+20);
    dest = str->str;
    sprintf(dest, "begin 644 %s\r\n", filename);
    dest += 12 + strlen(filename);
    if(groups) {
      unsigned char tmp[3], *tmpp=tmp;
      char *kp, k;
      int i;

      do_uue_encode(groups-1, &src, &dest, last);

      tmp[1]=tmp[2]=0;
      for(i=0; i<last; i++) tmp[i]=*src++;
      k = *--dest;
      kp = dest;
      do_uue_encode(1, &tmpp, &dest, 0);
      *kp = k;
      switch(last) {
      case 1:
	dest[-2]='`';
      case 2:
	dest[-1]='`';
      }
      *dest++ = 13;
      *dest++ = 10;
    }
    memcpy(dest, "`\r\nend\r\n", 8);
    pop_n_elems(args);
    push_string(end_shared_string(str));
  }
}

static void f_tokenize(INT32 args)
{
  if(args != 1)
    error("Wrong number of arguments to MIME.tokenize()\n");
  else if(sp[-1].type != T_STRING)
    error("Wrong type of argument to MIME.tokenize()\n");
  else {
    unsigned char *src = (unsigned char *)sp[-1].u.string->str;
    struct array *arr;
    struct pike_string *str;
    INT32 cnt = sp[-1].u.string->len, n = 0, l, e;
    char *p;

    while(cnt>0)
      switch(rfc822ctype[*src]) {
      case CT_SPECIAL:
      case CT_RBRACK:
      case CT_RPAR:
	/* individual special character */
	push_int(*src++);
	n++;
	--cnt;
	break;
      case CT_ATOM:
	/* atom */
	for(l=1; l<cnt; l++)
	  if(rfc822ctype[src[l]] != CT_ATOM)
	    break;
	push_string(make_shared_binary_string(src, l));
	n++;
	src += l;
	cnt -= l;
	break;
      case CT_QUOTE:
	/* quoted-string */
	for(e=0, l=1; l<cnt; l++)
	  if(src[l] == '"')
	    break;
	  else if(src[l] == '\\') { e++; l++; }
	str = begin_shared_string( l-e-1 );
	for(p=str->str, e=1; e<l; e++)
	  *p++=(src[e]=='\\'? src[++e]:src[e]);
	push_string(end_shared_string(str));
	n++;
	src += l+1;
	cnt -= l+1;
	break;
      case CT_LBRACK:
	/* domain literal */
	for(e=0, l=1; l<cnt; l++)
	  if(src[l] == ']')
	    break;
	  else if(src[l] == '\\') { e++; l++; }
	str = begin_shared_string( l-e+1 );
	for(p=str->str, e=0; e<=l; e++)
	  *p++=(src[e]=='\\'? src[++e]:src[e]);
	push_string(end_shared_string(str));
	n++;
	src += l+1;
	cnt -= l+1;
	break;
      case CT_LPAR:
	/* comment */
	for(e=1, l=1; l<cnt; l++)
	  if(src[l] == '(')
	    e++;
	  else if(src[l] == ')')
	    if(!--e)
	      break;
	    else ;
	  else if(src[l] == '\\') l++;
	src += l+1;
	cnt -= l+1;
	break;
      case CT_WHITE:
	/* whitespace */
	src++;
	--cnt;
	break;
      default:
	error("invalid character in header field\n");
      }

    arr = aggregate_array(n);
    pop_n_elems(1);
    push_array(arr);
  }
}

static int check_atom_chars(unsigned char *str, INT32 len)
{
  /* Atoms must contain at least 1 character... */
  if(len<1) return 0;

  while(len--)
    if(*str>=0x80 || rfc822ctype[*str]!=CT_ATOM)
      return 0;
    else
      str++;

  return 1;
}

static void f_quote(INT32 args)
{
  struct svalue *item;
  INT32 cnt;
  dynamic_buffer buf;
  int prev_atom = 0;

  if(args != 1)
    error("Wrong number of arguments to MIME.quote()\n");
  else if(sp[-1].type != T_ARRAY)
    error("Wrong type of argument to MIME.quote()\n");

  buf.s.str=NULL;
  initialize_buf(&buf);
  for(cnt=sp[-1].u.array->size, item=sp[-1].u.array->item; cnt--; item++) {
    if(item->type == T_INT) {
      /* single special character */
      low_my_putchar( item->u.integer, &buf );
      prev_atom=0;
    } else if(item->type != T_STRING) {
      toss_buffer(&buf);
      error("Wrong type of argument to MIME.quote()\n");
    } else {
      struct pike_string *str=item->u.string;
      if(prev_atom)
	low_my_putchar( ' ', &buf );
      if(check_atom_chars((unsigned char *)str->str, str->len)) {
	/* valid atom without quotes... */
	low_my_binary_strcat(str->str, str->len, &buf);
      } else {
	/* quote string */
	INT32 len=str->len;
	char *src=str->str;
	low_my_putchar( '"', &buf );
	while(len--) {
	  if(*src=='"' || *src=='\\' || *src=='\r')
	    low_my_putchar( '\\', &buf );
	  low_my_putchar( *src++, &buf );
	}
	low_my_putchar( '"', &buf );
      }
      prev_atom=1;
    }
  }
  pop_n_elems(1);
  push_string(low_free_buf(&buf));  
}
