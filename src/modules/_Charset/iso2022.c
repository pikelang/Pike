#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "global.h"
RCSID("$Id: iso2022.c,v 1.3 1999/03/07 01:37:49 grubba Exp $");
#include "program.h"
#include "interpret.h"
#include "stralloc.h"
#include "object.h"
#include "module_support.h"

#include "iso2022.h"


#define PRGM_NAME "Locale.Charset.ISO2022"


static struct program *iso2022_program = NULL;

struct gdesc {
  UNICHAR *transl;
  int mode, index;
};

struct iso2022_stor {
  struct gdesc g[4], *gl, *gr;
  struct pike_string *retain;

  struct string_builder strbuild;

};

#define EMIT(X) string_builder_putchar(&s->strbuild,(X))


extern struct charset_def charset_map[];
extern int num_charset_def;

extern UNICHAR *iso2022_94[];
extern UNICHAR *iso2022_96[];
extern UNICHAR *iso2022_9494[];
extern UNICHAR *iso2022_9696[];

static UNICHAR **transltab[4] = { iso2022_94, iso2022_96,
				  iso2022_9494, iso2022_9696 };

static INT32 eat_text(unsigned char *src, INT32 srclen, struct iso2022_stor *s,
		     struct gdesc *g)
{
  if(g->transl == NULL) switch(g->mode) {
  case MODE_94:
    while(srclen--) {
      char x = (*src++)&0x7f;
      if(x==0x20 || x==0x7f)
	EMIT((UNICHAR)x);
      else
	EMIT(DEFCHAR);
    }
    return 0;
    break;
  case MODE_96:
    while(srclen--)
      EMIT(DEFCHAR);
    return 0;
    break;
  case MODE_9494:
    while(srclen>1) {
      char hi, lo;
      if ((hi = (*src++)&0x7f)!=0x20 && hi != 0x7f &&
	  (lo = (*src)&0x7f)!=0x20 && lo != 0x7f) {
	EMIT(DEFCHAR);
	src++;
	srclen -= 2;
      } else {
	EMIT(hi);
	srclen--;
      }
    }
    if(srclen==1 && (((*src)&0x7f)==0x20 || ((*src)&0x7f)==0x7f)) {
      EMIT((UNICHAR)((*src++)&0x7f));
      --srclen;
    }
    break;
  case MODE_9696:
    while(srclen>1) {
      EMIT(DEFCHAR);
      srclen -= 2;
    }
    break;
  } else switch(g->mode) {
  case MODE_94:
    while(srclen--) {
      char x = (*src++)&0x7f;
      if(x==0x20 || x==0x7f)
	EMIT((UNICHAR)x);
      else
	EMIT(g->transl[x-0x21]);
    }
    return 0;
    break;
  case MODE_96:
    while(srclen--)
      EMIT(g->transl[((*src++)&0x7f)-0x20]);
    return 0;
    break;
  case MODE_9494:
    while(srclen>1) {
      char hi, lo;
      if ((hi = (*src++)&0x7f)!=0x20 && hi != 0x7f &&
	  (lo = (*src)&0x7f)!=0x20 && lo != 0x7f) {
	EMIT(g->transl[(hi-0x21)*94+(lo-0x21)]);
	src++;
	srclen -= 2;
      } else {
	EMIT(hi);
	srclen--;
      }
    }
    if(srclen==1 && (((*src)&0x7f)==0x20 || ((*src)&0x7f)==0x7f)) {
      EMIT((UNICHAR)((*src++)&0x7f));
      --srclen;
    }
    break;
  case MODE_9696:
    while(srclen>1) {
      char hi = (*src++)&0x7f;
      char lo = (*src++)&0x7f;
      EMIT(g->transl[(hi-0x20)*96+(lo-0x20)]);
      srclen -= 2;
    }
    break;
  }
  return srclen;
}

static INT32 parse_esc(unsigned char *src, INT32 srclen, struct iso2022_stor *s)
{
  int grp=-1, wide=0, final, mode, l=1;
  struct gdesc *g;

  /* Return value:

     0: need more bytes.
    -n: literal copy n bytes
     n: valid esc sequence, remove n bytes
  */

  if(srclen<2)
    return 0;

  if(src[1]>=0x40) 
    switch(src[1]) {
    case 'n':
      /* LS2 */
      s->gl = &s->g[2];
      return 2;
    case 'o':
      /* LS3 */
      s->gl = &s->g[3];
      return 2;
    case '~':
      /* LS1R */
      s->gr = &s->g[1];
      return 2;
    case '}':
      /* LS2R */
      s->gr = &s->g[2];
      return 2;
    case '|':
      /* LS3R */
      s->gr = &s->g[3];
      return 2;
    case 'N':
      /* SS2 */
      if(srclen<3)
	return 0;
      if((src[2]&0x7f)>=0x20) {
	eat_text(src+2, 1, s, &s->g[2]);
	return 3;
      }
      return 2;
    case 'O':
      /* SS3 */
      if(srclen<3)
	return 0;
      if((src[2]&0x7f)>=0x20) {
	eat_text(src+2, 1, s, &s->g[3]);
	return 3;
      }
      return 2;
    default:
      return -1;
    }

  if(src[1]<0x24 || src[1]>=0x30)
    return -1;

  if(srclen<3)
    return 0;

  if(src[l] == '$') {
    wide = 1;
    grp = 0;
    l++;
  }
  if(src[l] >= 0x28 && src[l] < 0x30)
    grp = src[l++]-0x28;
  if(l>=srclen)
    return 0;
  if(grp<0 || src[l]<0x30 || src[l]>0x7e)
    return -1;
  final = src[l++];
  mode = MODE_94;
  if(grp>=4) {
    mode = MODE_96;
    grp-=4;
  }
  if(wide)
    mode += 2;
  g = &s->g[grp];
  g->mode = mode;
  g->index = final-0x30;
  if(final<0x40 || (wide && final>=0x60))
    g->transl = NULL;
  else
    g->transl = transltab[mode][final-0x40];
  return l;
}

static INT32 eat_chars(unsigned char *src, INT32 srclen,
		       struct iso2022_stor *s)
{
  INT32 l;
  while(srclen>0)
    if(((*src)&0x7f)<0x20)
      switch(*src++) {
      case 0x0f:
	/* SI */
	s->gl = &s->g[0];
	srclen--;
	break;
      case 0x0e:
	/* SO */
	s->gl = &s->g[1];
	srclen--;
	break;
      case 0x8e:
	/* SS2 */
	if(srclen<2)
	  return srclen;
	srclen--;
	if((src[0]&0x7f)>=0x20) {
	  eat_text(src, 1, s, &s->g[2]);
	  srclen--;
	}
	break;
      case 0x8f:
	/* SS3 */
	if(srclen<2)
	  return srclen;
	srclen--;
	if((src[0]&0x7f)>=0x20) {
	  eat_text(src, 1, s, &s->g[3]);
	  srclen--;
	}
	break;
      case 0x1b:
	l = parse_esc(src-1, srclen, s);
	if(l==0)
	  return srclen;
	else if(l<0) {
	  --src;
	  srclen += l;
	  while(l<0) {
	    EMIT(*src++);
	    l++;
	  }
	} else {
	  src += l-1;
	  srclen -= l;
	}
	break;
      default:
	srclen--;
	EMIT(src[-1]);
	break;
      }
    else if(*src>=0x80) {
      for(l=1; l<srclen && src[l]>=0xa0; l++);
      l -= eat_text(src, l, s, s->gr);
      if(l==0)
	return srclen;
      src += l;
      srclen -= l;
    } else {
      for(l=1; l<srclen && src[l]>=0x20 && src[l]<0x80; l++);
      l -= eat_text(src, l, s, s->gl);
      if(l==0)
	return srclen;
      src += l;
      srclen -= l;
    }
  return srclen;
}

static void eat_string(struct pike_string *str, struct iso2022_stor *s)
{
  struct pike_string *tmpstr = NULL;
  INT32 l;

  if(s->retain != NULL) {
    tmpstr = add_shared_strings(s->retain, str);
    free_string(s->retain);
    s->retain = NULL;
    str = tmpstr;
  }

  l = eat_chars((unsigned char *)str->str, str->len, s);

  if(l>0)
    s->retain = make_shared_binary_string(str->str+str->len-l, l);

  if(tmpstr != NULL)
    free_string(tmpstr);
}


static void f_feed(INT32 args)
{
  struct pike_string *str;

  get_all_args(PRGM_NAME"->feed()", args, "%S", &str);

  eat_string(str, (struct iso2022_stor *)fp->current_storage);

  pop_n_elems(args);
  push_object(this_object());
}

static void f_drain(INT32 args)
{
  struct iso2022_stor *s = (struct iso2022_stor *)fp->current_storage;

  pop_n_elems(args);
  push_string(finish_string_builder(&s->strbuild));
  init_string_builder(&s->strbuild, 0);
}

static void f_clear(INT32 args)
{
  extern UNICHAR map_ANSI_X3_4_1968[];
  extern UNICHAR map_ISO_8859_1_1987[];
  struct iso2022_stor *s = (struct iso2022_stor *)fp->current_storage;
  int i;

  pop_n_elems(args);

  s->gl = &s->g[0];
  s->gr = &s->g[1];
  for(i=0; i<4; i++) {
    s->g[i].transl = NULL;
    s->g[i].mode = MODE_96;
    s->g[i].index = 0;
  }
  s->g[0].transl = map_ANSI_X3_4_1968;
  s->g[0].mode = MODE_94;
  s->g[0].index = 0x12;
  /* Perhaps not strictly correct, but oh so convenient... ;-) */
  s->g[1].transl = map_ISO_8859_1_1987;
  s->g[1].mode = MODE_96;
  s->g[1].index = 0x11;

  if(s->retain != NULL) {
    free_string(s->retain);
    s->retain = NULL;
  }

  reset_string_builder(&s->strbuild);
  
  push_object(this_object());
}

static void init_stor(struct object *o)
{
  struct iso2022_stor *s = (struct iso2022_stor *)fp->current_storage;

  s->retain = NULL;

  init_string_builder(&s->strbuild,0);

  f_clear(0);
  pop_n_elems(1);
}

static void exit_stor(struct object *o)
{
  struct iso2022_stor *s = (struct iso2022_stor *)fp->current_storage;

  if(s->retain != NULL) {
    free_string(s->retain);
    s->retain = NULL;
  }

  reset_string_builder(&s->strbuild);
  free_string(finish_string_builder(&s->strbuild));
}

struct program *iso2022_init(void)
{
  start_new_program();
  add_storage(sizeof(struct iso2022_stor));
  add_function("feed", f_feed, "function(string:object)", 0);
  add_function("drain", f_drain, "function(:string)", 0);
  add_function("clear", f_clear, "function(:object)", 0);
  set_init_callback(init_stor);
  set_exit_callback(exit_stor);
  return iso2022_program = end_program();
}

void iso2022_exit(void)
{
  if(iso2022_program) {
    free_program(iso2022_program);
    iso2022_program = NULL;
  }
}
