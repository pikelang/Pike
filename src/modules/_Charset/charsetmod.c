#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "global.h"
RCSID("$Id: charsetmod.c,v 1.4 1998/11/16 18:59:12 marcus Exp $");
#include "program.h"
#include "interpret.h"
#include "stralloc.h"
#include "object.h"
#include "module_support.h"
#include "error.h"

#include "iso2022.h"

#ifdef __CHAR_UNSIGNED__
#define SIGNED signed
#else
#define SIGNED
#endif

p_wchar1 *misc_charset_lookup(char *name, int *rlo, int *rhi);

static struct program *std_cs_program = NULL, *std_rfc_program = NULL;
static struct program *utf7_program = NULL, *utf8_program = NULL;
static struct program *utf7e_program = NULL, *utf8e_program = NULL;
static struct program *std_94_program = NULL, *std_96_program = NULL;
static struct program *std_9494_program = NULL, *std_9696_program = NULL;
static struct program *std_8bit_program = NULL;

struct std_cs_stor { 
  struct string_builder strbuild;
  struct pike_string *retain, *replace;
};

struct std_rfc_stor {
  UNICHAR *table;
};
static SIZE_T std_rfc_stor_offs = 0;

struct std_misc_stor {
  int lo, hi;
};
static SIZE_T std_misc_stor_offs = 0;

struct utf7_stor {
  INT32 dat, surro;
  int shift, datbit;
};
static SIZE_T utf7_stor_offs = 0;

static SIGNED char rev64t['z'-'+'+1];


static void f_create(INT32 args)
{
  struct std_cs_stor *s = (struct std_cs_stor *)fp->current_storage;

  check_all_args("create()", args, BIT_STRING|BIT_VOID|BIT_INT, 0);

  if(args>0 && sp[-args].type == T_STRING) {
    if(s->replace != NULL)
      free_string(s->replace);
    add_ref(s->replace = sp[-args].u.string);
  }

  pop_n_elems(args);
  push_int(0);
}

static void f_drain(INT32 args)
{
  struct std_cs_stor *s = (struct std_cs_stor *)fp->current_storage;

  pop_n_elems(args);
  push_string(finish_string_builder(&s->strbuild));
  init_string_builder(&s->strbuild, 0);
}

static void f_clear(INT32 args)
{
  struct std_cs_stor *s = (struct std_cs_stor *)fp->current_storage;

  pop_n_elems(args);

  if(s->retain != NULL) {
    free_string(s->retain);
    s->retain = NULL;
  }

  reset_string_builder(&s->strbuild);
  
  push_object(this_object());
}

static void init_stor(struct object *o)
{
  struct std_cs_stor *s = (struct std_cs_stor *)fp->current_storage;

  s->retain = NULL;
  s->replace = NULL;

  init_string_builder(&s->strbuild,0);
}

static void exit_stor(struct object *o)
{
  struct std_cs_stor *s = (struct std_cs_stor *)fp->current_storage;

  if(s->retain != NULL) {
    free_string(s->retain);
    s->retain = NULL;
  }

  if(s->replace != NULL) {
    free_string(s->replace);
    s->replace = NULL;
  }

  reset_string_builder(&s->strbuild);
  free_string(finish_string_builder(&s->strbuild));
}

static void f_std_feed(INT32 args, INT32 (*func)(const p_wchar0 *, INT32 n,
						 struct std_cs_stor *))
{
  struct std_cs_stor *s = (struct std_cs_stor *)fp->current_storage;
  struct pike_string *str, *tmpstr = NULL;
  INT32 l;

  get_all_args("feed()", args, "%W", &str);

  if(str->size_shift>0)
    error("Can't feed on wide strings!\n");

  if(s->retain != NULL) {
    tmpstr = add_shared_strings(s->retain, str);
    free_string(s->retain);
    s->retain = NULL;
    str = tmpstr;
  }

  l = func(STR0(str), str->len, s);

  if(l>0)
    s->retain = make_shared_binary_string(STR0(str)+str->len-l, l);

  if(tmpstr != NULL)
    free_string(tmpstr);

  pop_n_elems(args);
  push_object(this_object());
}


static INT32 feed_utf8(const p_wchar0 *p, INT32 l, struct std_cs_stor *s)
{
  static int utf8len[] = { 0, 0, 0, 0, 0, 0, 0, 0,
			   0, 0, 0, 0, 0, 0, 0, 0,
			   0, 0, 0, 0, 0, 0, 0, 0,
			   0, 0, 0, 0, 0, 0, 0, 0,
			   0, 0, 0, 0, 0, 0, 0, 0,
			   0, 0, 0, 0, 0, 0, 0, 0,
			   1, 1, 1, 1, 1, 1, 1, 1,
			   2, 2, 2, 2, 3, 3, 4, 5 };
  static unsigned INT32 utf8of[] = { 0ul, 0x3080ul, 0xe2080ul,
				     0x3c82080ul, 0xfa082080ul, 0x82082080ul };
  while(l>0) {
    unsigned INT32 ch = 0;
    int cl = utf8len[(*p)>>2];
    if(cl>--l)
      return l+1;
    switch(cl) {
    case 5: ch = *p++<<6;
    case 4: ch += *p++; ch<<=6;
    case 3: ch += *p++; ch<<=6;
    case 2: ch += *p++; ch<<=6;
    case 1: ch += *p++; ch<<=6;
    case 0: ch += *p++;
    }
    l-=cl;
    string_builder_putchar(&s->strbuild, (ch-utf8of[cl])&0x7fffffffl);
  }
  return l;
}

static void f_feed_utf8(INT32 args)
{
  f_std_feed(args, feed_utf8);
}

static INT32 feed_utf7(const p_wchar0 *p, INT32 l, struct std_cs_stor *s)
{
  struct utf7_stor *u7 = (struct utf7_stor *)(((char*)s)+utf7_stor_offs);
  INT32 dat = u7->dat, surro = u7->surro;
  int shift = u7->shift, datbit = u7->datbit;

  if(l<=0)
    return l;

  if(shift==2)
    if(*p=='-') {
      string_builder_putchar(&s->strbuild, '+');
      if(--l==0) {
	u7->shift=0;
	return l;
      }
      p++;
      shift=0;
    } else
      shift=1;

  for(;;)
    if(shift) {
      int c, z;
      while(l-->0 && (c=(*p++)-'+')>=0 && c<=('z'-'+') && (z=rev64t[c])>=0) {
	dat = (dat<<6)|z;
	if((datbit+=6)>=16) {
	  INT32 uc = dat>>(datbit-16);
	  if((uc&0xfc00)==0xd800) {
	    if(surro)
	      string_builder_putchar(&s->strbuild, surro);
	    surro = uc;
	  } else if(surro) {
	    if((uc&0xfc00)==0xdc00)
	      string_builder_putchar(&s->strbuild, 0x00010000+
				     ((surro&0x3ff)<<10)+(uc&0x3ff));
	    else {
	      string_builder_putchar(&s->strbuild, surro);
	      string_builder_putchar(&s->strbuild, uc);
	    }
	    surro = 0;
	  } else
	    string_builder_putchar(&s->strbuild, uc);
	  datbit -= 16;
	  dat &= (1<<datbit)-1;
	}
      }
      if(l<0) {
	l++;
	break;
      }
      if(surro) {
	string_builder_putchar(&s->strbuild, surro);
	surro = 0;
      }
      /* should check that dat is 0 here. */
      shift=0;
      dat=0;
      datbit=0;
      if(c!=('-'-'+')) {
	l++;
	--p;
      } else
	if(l==0)
	  break;	
    } else {
      while(l-->0 && *p!='+')
	string_builder_putchar(&s->strbuild, *p++);
      if(l<0) {
	l++;
	break;
      }
      p++;
      if(l==0) {
	shift=2;
	break;
      }
      if(*p=='-') {
	string_builder_putchar(&s->strbuild, '+');
	if(--l==0)
	  break;
	p++;
      } else
	shift = 1;
    }

  u7->dat = dat;
  u7->surro = surro;
  u7->shift = shift;
  u7->datbit = datbit;
  return l;
}

static void f_clear_utf7(INT32 args)
{
  struct utf7_stor *u7 =
    (struct utf7_stor *)(fp->current_storage+utf7_stor_offs);

  f_clear(args);
  
  u7->dat = 0;
  u7->surro = 0;
  u7->shift = 0;
  u7->datbit = 0;
}

static void utf7_init_stor(struct object *o)
{
  struct utf7_stor *u7 =
    (struct utf7_stor *)(fp->current_storage+utf7_stor_offs);

  u7->dat = 0;
  u7->surro = 0;
  u7->shift = 0;
  u7->datbit = 0;
}

static void f_feed_utf7(INT32 args)
{
  f_std_feed(args, feed_utf7);
}


static void f_rfc1345(INT32 args)
{
  extern struct charset_def charset_map[];
  extern int num_charset_def;
  struct pike_string *str;
  int lo=0, hi=num_charset_def-1;
  p_wchar1 *tabl;

  check_all_args("rfc1345()", args, BIT_STRING, BIT_INT|BIT_VOID,
		 BIT_STRING|BIT_VOID|BIT_INT, 0);

  str = sp[-args].u.string;

  if(str->size_shift>0)
    hi = -1;

  while(lo<=hi) {
    int c, mid = (lo+hi)>>1;
    if((c = strcmp(STR0(str), charset_map[mid].name))==0) {
      struct program *p;

      if(args>1 && sp[1-args].type == T_INT && sp[1-args].u.integer != 0)
	error("No '%s' encoding today, sorry.\n", STR0(str));

      pop_n_elems(args);
      switch(charset_map[mid].mode) {
      case MODE_94: p = std_94_program; break;
      case MODE_96: p = std_96_program; break;
      case MODE_9494: p = std_9494_program; break;
      case MODE_9696: p = std_9696_program; break;
      default:
	fatal("Internal error in rfc1345\n");
      }
      push_object(clone_object(p, 0));
      ((struct std_rfc_stor *)(sp[-1].u.object->storage+std_rfc_stor_offs))
	->table = charset_map[mid].table;
      return;
    }
    if(c<0)
      hi=mid-1;
    else
      lo=mid+1;
  }

  if(str->size_shift==0 &&
     (tabl = misc_charset_lookup(STR0(str), &lo, &hi))!=NULL) {

    if(args>1 && sp[1-args].type == T_INT && sp[1-args].u.integer != 0)
      error("No '%s' encoding today, sorry.\n", STR0(str));

    pop_n_elems(args);
    push_object(clone_object(std_8bit_program, 0));
    ((struct std_rfc_stor *)(sp[-1].u.object->storage+std_rfc_stor_offs))
      ->table = (UNICHAR *)tabl;
    ((struct std_misc_stor *)(sp[-1].u.object->storage+std_misc_stor_offs))
      ->lo = lo;
    ((struct std_misc_stor *)(sp[-1].u.object->storage+std_misc_stor_offs))
      ->hi = hi;
    return;    
  }

  pop_n_elems(args);
  push_int(0);
}

static INT32 feed_94(const p_wchar0 *p, INT32 l, struct std_cs_stor *s)
{
  UNICHAR *table =
    ((struct std_rfc_stor *)(((char*)s)+std_rfc_stor_offs))->table;
  while(l--) {
    p_wchar0 x = *p++;
    if(x<=0x20 || x>=0x7f)
      string_builder_putchar(&s->strbuild, x);
    else
      string_builder_putchar(&s->strbuild, table[x-0x21]);
  }
  return 0;
}

static void f_feed_94(INT32 args)
{
  f_std_feed(args, feed_94);
}

static INT32 feed_96(const p_wchar0 *p, INT32 l, struct std_cs_stor *s)
{
  UNICHAR *table =
    ((struct std_rfc_stor *)(((char*)s)+std_rfc_stor_offs))->table;
  while(l--) {
    p_wchar0 x = *p++;
    if(x<0xa0)
      string_builder_putchar(&s->strbuild, x);
    else
      string_builder_putchar(&s->strbuild, table[x-0xa0]);
  }
  return 0;
}

static void f_feed_96(INT32 args)
{
  f_std_feed(args, feed_96);
}

static INT32 feed_9494(const p_wchar0 *p, INT32 l, struct std_cs_stor *s)
{
  UNICHAR *table =
    ((struct std_rfc_stor *)(((char*)s)+std_rfc_stor_offs))->table;
  while(l--) {
    p_wchar0 y, x = (*p++)&0x7f;
    if(x<=0x20 || x>=0x7f)
      string_builder_putchar(&s->strbuild, x);
    else if(l==0)
      return 1;
    else if((y=(*p)&0x7f)>0x20 && y<0x7f) {
      --l;
      p++;
      string_builder_putchar(&s->strbuild, table[(x-0x21)*94+(y-0x21)]);
    } else {
      string_builder_putchar(&s->strbuild, x);
    }
  }
  return 0;
}

static void f_feed_9494(INT32 args)
{
  f_std_feed(args, feed_9494);
}

static INT32 feed_9696(const p_wchar0 *p, INT32 l, struct std_cs_stor *s)
{
  UNICHAR *table =
    ((struct std_rfc_stor *)(((char*)s)+std_rfc_stor_offs))->table;
  while(l--) {
    p_wchar0 y, x = (*p++)&0x7f;
    if(x<0x20)
      string_builder_putchar(&s->strbuild, x);
    else if(l==0)
      return 1;
    else if((y=(*p)&0x7f)>=0x20) {
      --l;
      p++;
      string_builder_putchar(&s->strbuild, table[(x-0x20)*96+(y-0x20)]);
    } else {
      string_builder_putchar(&s->strbuild, x);
    }
  }
  return 0;
}

static void f_feed_9696(INT32 args)
{
  f_std_feed(args, feed_9696);
}

static INT32 feed_8bit(const p_wchar0 *p, INT32 l, struct std_cs_stor *s)
{
  UNICHAR *table =
    ((struct std_rfc_stor *)(((char*)s)+std_rfc_stor_offs))->table;
  struct std_misc_stor *misc =
    ((struct std_misc_stor *)(((char*)s)+std_misc_stor_offs));
  int lo = misc->lo, hi = misc->hi;

  while(l--) {
    p_wchar0 x = *p++;
    if(x<lo || (x>0x7f && hi<=0x7f))
      string_builder_putchar(&s->strbuild, x);
    else if(x>hi)
      string_builder_putchar(&s->strbuild, DEFCHAR);
    else
      string_builder_putchar(&s->strbuild, table[x-lo]);
  }
  return 0;
}

static void f_feed_8bit(INT32 args)
{
  f_std_feed(args, feed_8bit);
}


static void feed_utf8e(struct string_builder *sb,
		       struct pike_string *str, struct pike_string *rep)
{
  INT32 l = str->len;

  switch(str->size_shift) {
  case 0:
    {
      p_wchar0 c, *p = STR0(str);
      while(l--)
	if((c=*p++)<=0x7f)
	  string_builder_putchar(sb, c);
        else {
	  string_builder_putchar(sb, 0xc0|(c>>6));
	  string_builder_putchar(sb, 0x80|(c&0x3f));	
	}
    }
    break;
  case 1:
    {
      p_wchar1 c, *p = STR1(str);
      while(l--)
	if((c=*p++)<=0x7f)
	  string_builder_putchar(sb, c);
	else if(c<=0x7ff) {
	  string_builder_putchar(sb, 0xc0|(c>>6));
	  string_builder_putchar(sb, 0x80|(c&0x3f));	
	} else {
      	  string_builder_putchar(sb, 0xe0|(c>>12));
	  string_builder_putchar(sb, 0x80|((c>>6)&0x3f));
	  string_builder_putchar(sb, 0x80|(c&0x3f));	
	}
    }
    break;
  case 2:
    {
      p_wchar2 c, *p = STR2(str);
      while(l--)
	if((c=*p++)<=0x7f)
	  string_builder_putchar(sb, c);
	else if(c<=0x7ff) {
	  string_builder_putchar(sb, 0xc0|(c>>6));
	  string_builder_putchar(sb, 0x80|(c&0x3f));	
	} else if(c<=0xffff) {
	  string_builder_putchar(sb, 0xe0|(c>>12));
	  string_builder_putchar(sb, 0x80|((c>>6)&0x3f));
	  string_builder_putchar(sb, 0x80|(c&0x3f));	
	} else if(c<=0x1fffff) {
	  string_builder_putchar(sb, 0xf0|(c>>18));
	  string_builder_putchar(sb, 0x80|((c>>12)&0x3f));
	  string_builder_putchar(sb, 0x80|((c>>6)&0x3f));
	  string_builder_putchar(sb, 0x80|(c&0x3f));	
	} else if(c<=0x3ffffff) {
	  string_builder_putchar(sb, 0xf8|(c>>24));
	  string_builder_putchar(sb, 0x80|((c>>18)&0x3f));
	  string_builder_putchar(sb, 0x80|((c>>12)&0x3f));
	  string_builder_putchar(sb, 0x80|((c>>6)&0x3f));
	  string_builder_putchar(sb, 0x80|(c&0x3f));	
	} else if(c<=0x7fffffff) {
	  string_builder_putchar(sb, 0xfc|(c>>30));
	  string_builder_putchar(sb, 0x80|((c>>24)&0x3f));
	  string_builder_putchar(sb, 0x80|((c>>18)&0x3f));
	  string_builder_putchar(sb, 0x80|((c>>12)&0x3f));
	  string_builder_putchar(sb, 0x80|((c>>6)&0x3f));
	  string_builder_putchar(sb, 0x80|(c&0x3f));	
	} else if(rep != NULL)
	  feed_utf8e(sb, rep, NULL);
	else
	  error("Character unsupported by encoding.\n");
    }
    break;
  default:
    fatal("Illegal shift size!\n");
  }
}

static void f_feed_utf8e(INT32 args)
{
  struct pike_string *str;
  struct std_cs_stor *cs = (struct std_cs_stor *)fp->current_storage;

  get_all_args("feed()", args, "%W", &str);

  feed_utf8e(&cs->strbuild, str, cs->replace);

  pop_n_elems(args);
  push_object(this_object());
}

void pike_module_init(void)
{
  int i;
  struct svalue prog;
  extern struct program *iso2022_init();
  struct program *iso2022_program = iso2022_init();
  if(iso2022_program != NULL)
    add_program_constant("ISO2022", iso2022_program, ID_STATIC|ID_NOMASK);

  start_new_program();
  add_storage(sizeof(struct std_cs_stor));
  add_function("drain", f_drain, "function(:string)", 0);
  add_function("clear", f_clear, "function(:object)", 0);
  add_function("create", f_create, "function(string|void:void)", 0);
  set_init_callback(init_stor);
  set_exit_callback(exit_stor);
  std_cs_program = end_program();

  prog.type = T_PROGRAM;
  prog.subtype = 0;
  prog.u.program = std_cs_program;

  memset(rev64t, -1, sizeof(rev64t));
  for(i=0; i<64; i++)
    rev64t["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
	  "0123456789+/"[i]-'+']=i;

  start_new_program();
  do_inherit(&prog, 0, NULL);
  utf7_stor_offs = add_storage(sizeof(struct utf7_stor));
  add_function("feed", f_feed_utf7, "function(string:object)", 0);
  add_function("clear", f_clear_utf7, "function(:object)", 0);
  set_init_callback(utf7_init_stor);
  add_program_constant("UTF7dec", utf7_program = end_program(), ID_STATIC|ID_NOMASK);

  start_new_program();
  do_inherit(&prog, 0, NULL);
  add_function("feed", f_feed_utf8, "function(string:object)", 0);
  add_program_constant("UTF8dec", utf8_program = end_program(), ID_STATIC|ID_NOMASK);

  start_new_program();
  do_inherit(&prog, 0, NULL);
  add_program_constant("UTF7enc", utf7e_program = end_program(), ID_STATIC|ID_NOMASK);

  start_new_program();
  do_inherit(&prog, 0, NULL);
  add_function("feed", f_feed_utf8e, "function(string:object)", 0);
  add_program_constant("UTF8enc", utf8e_program = end_program(), ID_STATIC|ID_NOMASK);

  start_new_program();
  do_inherit(&prog, 0, NULL);
  std_rfc_stor_offs = add_storage(sizeof(struct std_rfc_stor));
  std_rfc_program = end_program();

  prog.u.program = std_rfc_program;

  start_new_program();
  do_inherit(&prog, 0, NULL);
  add_function("feed", f_feed_94, "function(string:object)", 0);
  std_94_program = end_program();

  start_new_program();
  do_inherit(&prog, 0, NULL);
  add_function("feed", f_feed_96, "function(string:object)", 0);
  std_96_program = end_program();

  start_new_program();
  do_inherit(&prog, 0, NULL);
  add_function("feed", f_feed_9494, "function(string:object)", 0);
  std_9494_program = end_program();

  start_new_program();
  do_inherit(&prog, 0, NULL);
  add_function("feed", f_feed_9696, "function(string:object)", 0);
  std_9696_program = end_program();

  start_new_program();
  do_inherit(&prog, 0, NULL);
  std_misc_stor_offs = add_storage(sizeof(struct std_misc_stor));
  add_function("feed", f_feed_8bit, "function(string:object)", 0);
  std_8bit_program = end_program();

  add_function_constant("rfc1345", f_rfc1345,
			"function(string,int|void,string|void:object)", 0);
}

void pike_module_exit(void)
{
  extern void iso2022_exit();

  if(utf7e_program != NULL)
    free_program(utf7e_program);

  if(utf8e_program != NULL)
    free_program(utf8e_program);

  if(utf7_program != NULL)
    free_program(utf7_program);

  if(utf8_program != NULL)
    free_program(utf8_program);

  if(std_94_program != NULL)
    free_program(std_94_program);

  if(std_96_program != NULL)
    free_program(std_96_program);

  if(std_9494_program != NULL)
    free_program(std_9494_program);

  if(std_9696_program != NULL)
    free_program(std_9696_program);

  if(std_8bit_program != NULL)
    free_program(std_8bit_program);

  if(std_rfc_program != NULL)
    free_program(std_rfc_program);

  if(std_cs_program != NULL)
    free_program(std_cs_program);

  iso2022_exit();
}
