/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: iso2022.c,v 1.36 2005/02/17 14:43:50 grubba Exp $
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "global.h"
#include "program.h"
#include "interpret.h"
#include "stralloc.h"
#include "object.h"
#include "module_support.h"
#include "pike_error.h"

#include "iso2022.h"


#define sp Pike_sp
#define fp Pike_fp

#define PRGM_NAME "Locale.Charset.ISO2022"


static struct program *iso2022dec_program = NULL;
static struct program *iso2022enc_program = NULL;

struct gdesc {
  const UNICHAR *transl;
  int mode, index;
};

struct iso2022_stor {
  struct gdesc g[4], *gl, *gr;
  struct pike_string *retain;

  struct string_builder strbuild;

};

struct iso2022enc_stor {
  struct gdesc g[2];
  struct {
    p_wchar1 *map;
    unsigned lo, hi;
  } r[2];
  int variant;
  struct pike_string *replace;
  struct string_builder strbuild;
  struct svalue repcb;
};

#define EMIT(X) string_builder_putchar(&s->strbuild,(X))


extern struct charset_def charset_map[];
extern int num_charset_def;

extern const UNICHAR *iso2022_94[];
extern const UNICHAR *iso2022_96[];
extern const UNICHAR *iso2022_9494[];
extern const UNICHAR *iso2022_9696[];

static const UNICHAR **transltab[4] = { iso2022_94, iso2022_96,
					iso2022_9494, iso2022_9696 };

static ptrdiff_t eat_text(unsigned char *src, ptrdiff_t srclen,
			  struct iso2022_stor *s, struct gdesc *g)
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

static INT32 parse_esc(unsigned char *src, ptrdiff_t srclen,
		       struct iso2022_stor *s)
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

static ptrdiff_t eat_chars(unsigned char *src, ptrdiff_t srclen,
			   struct iso2022_stor *s)
{
  ptrdiff_t l;
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
  ptrdiff_t l;

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

static int call_repcb(struct svalue *repcb, p_wchar2 ch)
{
  push_string(make_shared_binary_string2(&ch, 1));
  apply_svalue(repcb, 1);
  if(sp[-1].type == T_STRING)
    return 1;
  pop_stack();
  return 0;
}

#define REPLACE_CHAR(ch, pos)			       \
          if(repcb != NULL && call_repcb(repcb, ch)) { \
	    eat_enc_string(sp[-1].u.string, s, rep, NULL); \
            pop_stack(); \
	  } else if(rep != NULL) \
            eat_enc_string(rep, s, NULL, NULL); \
	  else \
	    Pike_error("Character %lu at position %"PRINTPTRDIFFT"d "	\
		       "unsupported by encoding.\n",			\
		       (unsigned long) ch, (pos));

static const unsigned INT32 jp2_tab[] = {
  0x000003c0,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0xffffff00,0x000003ff,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0xfffc0000,0x000010ff,0x0000301c,
  0x00000000,0x00000000,0x3fffffc0,0x00aaaaaa,0x00ffffff,0x00000000,
  0x000fff00,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x80000000,0x0000000a,0x20000800,0x0000a000,0x02aa0000,
  0x00000000,0xa0000a00,0x00000000,0x00000000,0x00080000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x3ffaaaaa,0xaaaaaa00,0xaaaaaaaa,
  0xffaaaaaa,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0x000fffff,
  0x00000000,0x28aaaa00,0xa0282828,0x2828a028,0x28282828,0x002aaa8a,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000030,0x000fffc0,
  0x0000f000,0x000f000f,0x0000000f,0x00000000,0x00000000,0xf0000000,
  0x3fffffff,0x00000000,0x00000000,0x00000000,0x00000000,0x030fffff,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x0000a000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0xaaaaa800,0xaaaaaaaa,0x000aaaaa,0xfffffffc,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
  0x03ffffff,0x000aaaaa,0x00000000,0x00000000,0x00000000,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
  0xffffffff,0xffffffff,0xffffffff,0x0fffffff,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
  0x00000000,0x00000000,0x00000000,0x01000510,0x4aa00090,0x52418960,
  0x28200601,0x020a4105,0x011002a6,0x0fc5204a,0x4004f502,0x84000000,
  0x04205083,0x40088000,0xaa840820,0x00080840,0x005000a9,0x42a10015,
  0x54000544,0x01415551,0xa0865550,0x10aaaaaa,0x50444464,0x01550015,
  0x40011110,0x1110aa55,0x14154204,0x11140514,0x50144151,0x022aaa80,
  0x15554010,0x45455405,0x210000d0,0x0aaaa011,0x45000019,0x11104135,
  0x15414041,0x10004010,0x2a600555,0x15101415,0x40054054,0x54155055,
  0xa0808005,0x50121154,0x55401551,0x401aaa95,0x04940001,0x11004141,
  0x45010141,0x42051115,0xd0101095,0x55415040,0x15444001,0x55015540,
  0x00050545,0x40100010,0x40016448,0x0000d100,0x18a80282,0x10420148,
  0x06810000,0x84000044,0x51950000,0x40018012,0x10010555,0x90800610,
  0x82804150,0x08000414,0x00a91450,0x0a810012,0xa9000414,0x0004042a,
  0x4d15440a,0x10009005,0x00054000,0x01500419,0x29155442,0x40025528,
  0x845150a1,0x41811101,0x01401106,0x01550404,0x00c05105,0x01500041,
  0x51410054,0x25456400,0x15000411,0xa1400310,0x80442800,0x05838664,
  0x1c240010,0x1aa96840,0x04040940,0x405001a4,0x84154510,0x40000409,
  0x14120458,0x00005001,0x0a880400,0x00002010,0x2010a860,0x40114084,
  0x000005e5,0x44804140,0x12aaaaa2,0x15445550,0x40020120,0x04050464,
  0x49a81604,0x14145085,0x40544204,0x96101000,0xaaa8aaaa,0x41108001,
  0x51001080,0x15050405,0x03800408,0x0250aaaa,0x85409412,0x04144084,
  0x10550805,0xaaaa9905,0x4002aaaa,0x50011014,0x00004155,0x04000140,
  0x6aaa8a98,0x19441055,0x18416054,0xaaa6a818,0x40001aaa,0x5611040a,
  0xa0040015,0x0900aaaa,0x45544441,0x44555545,0x520a81c0,0x54041055,
  0x02802414,0x21410500,0x55555480,0x85555080,0x10004054,0x45540400,
  0x45040201,0x05415524,0x20005a04,0x01106014,0x04500404,0x51540555,
  0x54280140,0x05001400,0xaaaa8010,0x55414412,0x11546545,0x01002a44,
  0x55955520,0x29856004,0x04010220,0x41014020,0x142a0400,0x84055104,
  0x54011401,0x05058140,0x50400008,0x04005001,0x40040010,0x45040200,
  0x40000401,0x02014904,0x41010000,0x55554005,0x24281521,0x00041555,
  0x01000014,0x14010150,0x04101111,0x00040014,0x00002040,0x09409600,
  0x84000150,0x40010144,0x42a81641,0x00801068,0x55442050,0x44400044,
  0x01441415,0x1142a000,0x60004410,0x54a84100,0x01105541,0x04110144,
  0x54028010,0x52004140,0x000b4155,0x1502a901,0x10c00050,0x05514140,
  0x000002a9,0x55214500,0x10000401,0x04004540,0x15556a00,0x55400000,
  0x55055541,0x55204315,0x40515555,0x40015100,0x551202a0,0x10900401,
  0x54001408,0x00010c05,0x55544000,0x04000405,0x00018100,0x01400004,
  0x10081090,0x52210020,0x55000000,0x10101104,0x20001145,0x45a5000a,
  0x29504014,0x54000005,0x00155000,0x43000001,0x06880104,0x01000000,
  0x5a0e4a00,0x51549840,0x00001611,0x00080020,0x01d40000,0x04056050,
  0x85500501,0x4811402a,0x56aaa010,0x08101051,0x88100510,0x554402ab,
  0x04004015,0x10002a00,0x00000114,0x040010a4,0x20000010,0x28405540,
  0x64040104,0x00119555,0x48860404,0x04900155,0x55540201,0x00150045,
  0x40440550,0x00704001,0x00050155,0x50404840,0x00050001,0x80040121,
  0x00050000,0x84024809,0x42010512,0x1001a001,0xd2800418,0x00540020,
  0x11411005,0x55550000,0x0000c001,0x10402001,0xa00802aa,0x01150550,
  0x54080044,0x15401000,0x11000514,0x00600400,0x00000000,0x10015012,
  0x05040000,0x11610156,0x44681101,0x04005501,0x00410921,0x40004141,
  0x14910510,0x54040000,0x05001800,0x41405550,0x0001a805,0x80456100,
  0x10000155,0x55158014,0x21101050,0x05506aaa,0x40114140,0x04000100,
  0xaa800440,0x01054001,0x04005104,0x00004004,0x5aaa2084,0x50115c10,
  0x14010445,0xaa810510,0x50404005,0x50550155,0x14440500,0xaaa82410,
  0x05010410,0x01504010,0x15055155,0x00502a92,0x00045405,0x00041404,
  0x10000008,0x01000301,0x4d550010,0x04400145,0x45014050,0x04110401,
  0x01058000,0x50000015,0x555556a8,0x55554415,0x00000800,0x80806154,
  0x01028440,0x02454a51,0x2404b00c,0x01001550,0x01101115,0x46a8a100,
  0x04100042,0x54000000,0x80200001,0x00011aaa,0x50105040,0x10000100,
  0x80101400,0x001aaaa0,0x05000110,0x24500140,0x2aa11041,0x04140aaa,
  0x01c41420,0x10501954,0xa0550150,0x050014aa,0x45010504,0x00104110,
  0x10140501,0x54001100,0x062a8082,0x18010404,0x21000544,0x09100401,
  0xa805c450,0x02940aaa,0x50810111,0x40140519,0x44000401,0x406aaa08,
  0x65500155,0x04000055,0x05480140,0x40005620,0x45452005,0x01015055,
  0x5422a840,0x11540102,0x20004405,0x04140500,0x40404401,0x55550104,
  0x50541560,0x10055815,0x05100004,0x32044040,0x40800151,0x0c404001,
  0x35500000,0x00900404,0x04010490,0x10081041,0x40000150,0x10050010,
  0x40011101,0xa00000c0,0x5f04a511,0x04050101,0x14081417,0x11541154,
  0x20f54045,0x11101401,0x01656a84,0x01500c05,0x51454440,0x05400021,
  0x41540011,0x04028101,0x404004d4,0x55013051,0x40400155,0x15410000,
  0x00500000,0x10205400,0x01040042,0x80000015,0x10000840,0x010144a2,
  0x04000500,0x110a1000,0x00401100,0x50000154,0x20451044,0x0aa28a80,
  0x35400415,0x00050451,0x01044010,0x00001806,0x08055011,0xaaaaa802,
  0x4404013a,0x45014000,0x60408045,0x01580500,0x00414410,0x5c4a2a2a,
  0x01540415,0x00011110,0x01151000,0x10140544,0x00400010,0x4101042a,
  0x05041045,0x05510400,0x05401405,0x00141000,0x80005545,0x0800c202,
  0x00410454,0x10055410,0x00404000,0x01051540,0x40500015,0x040aaa01,
  0x00011010,0x45551404,0x01154000,0x00010400,0x80805204,0x50300552,
  0x10000545,0x40000045,0x04120404,0x80010101,0x50404159,0x10141400,
  0x004014d5,0x00d54900,0x12a80044,0x00445054,0x54400004,0x40480400,
  0x54000105,0x55555000,0x05001501,0x55005500,0x55003015,0x43055055,
  0x10554005,0x55550120,0x40401400,0x04101000,0x04015010,0x15404801,
  0x1600c431,0x00008418,0x406040a0,0x15050059,0x14050001,0x81040168,
  0x00189800,0x10205544,0x48044804,0x1404a555,0x40262811,0x91295028,
  0x4850146a,0x455aa000,0x01440110,0x44484208,0x00451100,0x80045400,
  0x41014500,0x082aaa80,0x00000111,0x54001010,0x21101115,0x11444001,
  0xaaaaa910,0x5010550a,0x00100044,0x50015000,0x41040004,0xaaaaa080,
  0x500042aa,0x40004406,0xc1000001,0x0c084010,0xa8015114,0x219aaaaa,
  0x44044100,0x501501c0,0x44701000,0x40040411,0x01031101,0xaa000155,
  0x1000a20a,0x00004010,0x0d441210,0x40455500,0x15004155,0x00010050,
  0x05700040,0x040aaa80,0x00040040,0x14c04001,0x40400244,0x54114045,
  0x802a8000,0x144aaaaa,0x05440000,0x05510100,0x00555000,0x54141200,
  0x01401541,0x4aaaa000,0x14144054,0x01550014,0x14000020,0x05494040,
  0x42101151,0x50015401,0x31001401,0x00194400,0x10100028,0x00050000,
  0x05040401,0x04005401,0x03004000,0x40030041,0x01044900,0x90555540,
  0x14000503,0x19000500,0x90001400,0x00001406,0x0a715500,0x0040010c,
  0xaa804105,0x001000aa,0x01811554,0xaaaaa100,0x00108000,0x41605540,
  0x1002a8aa,0x45400001,0x00010005,0x00505404,0x15104011,0x00010001,
  0x0026aaa0,0x010c5401,0x00520010,0x40000011,0x40100851,0x01400105,
  0x01541040,0x00000000,0x10010000,0x00054001,0x44155540,0x40400155,
  0x00058108,0x58000554,0x11101541,0x50412041,0x41614804,0x94a40100,
  0x80400040,0x14010000,0x10028106,0x09564007,0x60011400,0xa0020400,
  0x1050016a,0x25405598,0x10050850,0x01014118,0x055a0414,0x04000040,
  0x55551015,0x59445001,0x21044004,0x10001455,0x55814749,0xa450d411,
  0x55244402,0x41441400,0x0404105a,0x00045344,0x44544520,0x80104105,
  0x005000c6,0x5555510f,0x02070005,0x54405141,0x04004015,0x14415c10,
  0x00008014,0x60045435,0x41150015,0x55551c45,0x40655554,0x01053000,
  0x50550061,0x40510100,0x41500100,0x8000100c,0x00c85010,0x48144105,
  0x54041800,0x10014841,0x500040d1,0x40000004,0x15540c24,0x00000484,
  0x8104a451,0xaa00011a,0x40100102,0x510a1105,0x41006000,0x55a26105,
  0x00040004,0x41114951,0x15469452,0x04a41440,0xa1155450,0x38540545,
  0x60040600,0x54800580,0x00000104,0x05000424,0x55400408,0x05555555,
  0x04000028,0x91440404,0x0006010a,0x81011001,0x01041005,0x58101001,
  0x01145555,0x54010010,0x14041500,0x54101498,0x5500555a,0x01104111,
  0x04100155,0x05000955,0x40155520,0x50184010,0x10000404,0x00040040,
  0x50040554,0x01901101,0xa8412105,0x1404140a,0x86aaa004,0x16080054,
  0x2a8a4401,0xd5014142,0x5100a812,0x18115555,0x10108000,0x40180000,
  0x56810100,0x05010905,0x44480255,0x11054000,0x55481450,0x00101555,
  0x40555c50,0x01001801,0x51503081,0x54054601,0x44400505,0x55501500,
  0x01410104,0x80544000,0x15460010,0x01400000,0x01615544,0x40450004,
  0x00415504,0x48116190,0x15501100,0x84500411,0x09010004,0x01146c50,
  0x10400404,0x45554084,0x00153581,0x01014500,0x55541408,0x04105401,
  0x10018400,0x08501411,0x1a000105,0x0902a841,0x01440440,0x11041000,
  0x2040e004,0x01450000,0x14400154,0x00004080,0x40000900,0x54544240,
  0x66a20105,0x10004005,0x00aa0400,0x44141121,0xac884050,0x10040902,
  0x10401005,0x418aa421,0x05501411,0x00110410,0x005455b8,0xa0110008,
  0x1515051a,0x51441554,0x30542000,0x55011401,0x00141555,0x00155008,
  0x15540454,0x54054054,0x05840215,0x40005000,0x1a000108,0x00201001,
  0x41505001,0x46054108,0x04144540,0x00561000,0x00000400,0x40054040,
  0x14001054,0x04109540,0x15000004,0x04014005,0x05154054,0x00104400,
  0x40518001,0x35415554,0x14005004,0x00155003,0x00045040,0x15001550,
  0x00047000,0x045014d0,0x00155404,0x00010001,0x05004405,0x45014001,
  0x40001040,0x50004500,0x40010011,0x50400554,0x10015540,0x04105004,
  0x94110404,0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,
  0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,0x55400aaa,0x40004160,
  0x05608050,0x04001564,0x54000204,0x44400009,0x81005005,0x05509014,
  0x41000102,0x50010041,0x40420000,0x51010440,0x54100018,0x541d4000,
  0x54001144,0x5d289921,0x11164509,0x0a8001a1,0x00000200,0x04281501,
  0x00000404,0x10010086,0x91000000,0x20405202,0xaa058000,0x4401002a,
  0x02014555,0x16aa8001,0x10141051,0xa8400040,0x110152aa,0x01c05501,
  0x0152a421,0x01124400,0x00284010,0x44054005,0x2a804000,0x15401504,
  0x10411009,0x10202010,0x00400110,0x52500448,0x15405400,0x01400104,
  0x44055415,0x40002241,0x04140140,0x88010094,0x2a90011a,0x50481540,
  0x01001554,0x04004041,0x44680182,0x90164165,0x02519504,0x10154145,
  0x51104111,0xaaaa92a8,0x00120401,0x44150111,0x14005000,0x82514005,
  0x54600aaa,0x15008015,0x05504041,0x14006150,0xaaa05544,0xaaaaaaaa,
  0x44010001,0x04001155,0x05450510,0x05045000,0x52aaa2a2,0x40044001,
  0x04000105,0x00110aa8,0x01051000,0x40101004,0x0a060405,0x4002aa00,
  0x00040000,0x45514400,0x01151554,0x00104501,0x55011141,0x2a0a8405,
  0x10008450,0x40155000,0x14100104,0x41014011,0xa9000040,0x40156aaa,
  0x04554015,0x51004050,0x11000001,0x40554550,0x42aa9150,0x40001000,
  0x55041055,0x40400115,0x445402a0,0x41441404,0x54010040,0x04004415,
  0x55015502,0x10505550,0x40150080,0x54015001,0x41001555,0x54000501,
  0x55450015,0x00054005,0x05111000,0x80001555,0x04c55058,0x61040155,
  0xaa105400,0x0401002a,0x09155004,0x06015004,0x40254544,0x80200455,
  0x01125416,0x00005455,0x051002aa,0x50404500,0x5004a941,0x50010064,
  0x15140014,0x04028555,0x04010000,0x40000504,0x2a400016,0x01455540,
  0x15554441,0x15025541,0x50000404,0x10150004,0x55955488,0x27500950,
  0x40110044,0x00555045,0x55550151,0x55550000,0x01100411,0x50550444,
  0x41111605,0x2a040901,0x00540004,0x00000a01,0x41510000,0x08150001,
  0x16100000,0x155aa001,0x10400054,0x40004aa0,0x00004405,0x54001004,
  0x01951400,0x00015009,0x50140615,0x40055411,0x01554015,0x10400054,
  0x05500441,0x04040000,0x55400501,0x00000401,0x01005501,0xaaaaaaa8,
  0x2001550a,0x84440400,0x55541044,0x40004500,0x10000514,0x41401115,
  0x14004400,0x15544541,0x05015040,0x00004404,0x50545400,0x40601041,
  0x40045011,0x50344000,0x10405540,0x00151040,0x44004544,0x01300000,
  0x41001501,0x00501400,0x51000004,0x54000000,0x01004001,0x04015550,
  0x10040150,0x04000040,0x11551000,0x44400504,0x55000500,0xaaaaaaaa,
  0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,
  0xaaaaaaaa,0xaaaaaaaa,0x14052aaa,0x40444400,0x05454154,0x04055540,
  0x10401440,0x00405000,0x01540055,0x00000100,0x00140000,0x40441400,
  0x04055400,0x41010004,0x50015715,0x00040055,0xa8554010,0xaaaaaaaa,
  0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,0x110404aa,0x40002800,0x54810110,
  0x50000605,0x55401001,0x81464618,0x10155681,0x20044504,0x01005500,
  0x04aa8115,0x02401554,0x00001104,0xaa085055,0x54255044,0x54400005,
  0x15400128,0x55542014,0x80540005,0x80040050,0x16500411,0x14055550,
  0x00500560,0x60004155,0x01000105,0x54010000,0x00145555,0x40014015,
  0x10015001,0x15014555,0x00105500,0x40004045,0x14010555,0xaaaaa400,
  0xaaaaaaaa,0xaaaaaaaa,0x042aaaaa,0x00a81505,0x18091400,0x05529005,
  0xaa8a0451,0x10010141,0x104014b4,0x01290110,0x00410020,0x45552000,
  0x40804140,0x01000140,0x04408000,0x01005010,0x00040101,0x15410400,
  0x08454481,0x2a000415,0x14a81151,0x81015540,0x7505449a,0x4040a400,
  0x50021101,0x55555501,0x11515d01,0x50044451,0x05545504,0x40016040,
  0x28544042,0xa4004104,0xa854210a,0x12101444,0x50a10544,0x15015085,
  0x55401055,0x00200550,0x10144140,0x55140500,0x04554005,0x01555555,
  0x0555d051,0x50015540,0x15151455,0x54105051,0x05050004,0x55554405,
  0x45555601,0x15550000,0x40044010,0x60555555,0x555110d5,0x15415555,
  0x55555541,0x51015455,0x40440001,0x05550315,0x55000554,0x00144504,
  0x20005140,0x00354000,0x10154415,0x44155500,0x44155155,0x44555555,
  0x55410410,0x00551545,0x55501500,0x05545505,0x51540040,0x00005010,
  0x55557000,0x55550115,0x40005400,0x50000c00,0x65400155,0x01555440,
  0x41055400,0x54415544,0x00000014,0xaaaaa940,0xaaaaaaaa,0xaaaaaaaa,
  0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,
  0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,
  0x95552aaa,0x15010100,0x54000030,0x01541004,0x4410150c,0x00451000,
  0x54140055,0xaaaa1101,0xaaaaaaaa,0xaaaaaaaa,0x942aaaaa,0x01001510,
  0x05156a44,0x000aa804,0x00015910,0x100a8004,0x51400100,0x00040155,
  0x54101006,0x50041500,0x28100040,0x80100000,0x54110010,0x40040002,
  0x20100084,0x00040458,0x04500000,0x08010054,0x00000044,0x55500044,
  0x0051448c,0x10104840,0x54415040,0x00040001,0x501456a8,0x91000010,
  0x55555414,0x55144505,0x54555505,0x42aaa155,0x00154105,0x14104003,
  0x55041500,0x50555441,0x500c0451,0x00154155,0x00145054,0x01101450,
  0xaaaaa800,0xaaaaaaaa,0xaaaaaaaa,0x1540aaaa,0x55554101,0xa155c415,
  0x202aaaaa,0x00125544,0x00001040,0x08014010,0x41505404,0x00405010,
  0x01555554,0x14015005,0x55550100,0xaaaaaa81,0xaaaaaaaa,0xaaaaaaaa,
  0x44400aaa,0x00001000,0x54140415,0x00040040,0x11140140,0x01554000,
  0x14040501,0x01430150,0x50001005,0x14005555,0x00011540,0x51554104,
  0x04001155,0xaa000000,0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,0x5150aaaa,
  0x50649558,0x12815864,0x01405411,0x01054509,0x54140450,0x95550555,
  0x01441095,0x05401001,0x04155540,0x01498000,0x55405400,0x15555555,
  0x55555440,0x45555501,0x15500101,0x01045000,0x55414015,0x10014004,
  0x54014001,0x55505400,0x55554040,0x40400015,0x15155004,0x00155050,
  0x04005404,0x55540554,0x55411541,0x55550040,0xaa404555,0xaaaaaaaa,
  0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,0x0000102a,
  0x55554010,0x00400015,0x15014004,0x40000000,0x05155555,0x00104450,
  0x40000140,0x00555450,0x55405001,0x15501555,0x00015550,0x15100004,
  0x55040404,0x00154040,0x55505500,0x10554555,0x50000141,0x55555555,
  0x85055555,0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,
  0x215000aa,0x50004551,0x10551000,0x51010100,0x08000555,0x004950c0,
  0x40000408,0x15284120,0x60010415,0x4a840055,0x55555115,0x40555010,
  0x45154154,0x00005555,0x1555544a,0x54050000,0x85500545,0x42aaaaaa,
  0x90a85155,0x00000000,
};

static void eat_enc_string(struct pike_string *str, struct iso2022enc_stor *s,
			   struct pike_string *rep, struct svalue *repcb)
{
  extern UNICHAR map_ANSI_X3_4_1968[];
  extern UNICHAR map_ISO_8859_1_1998[];
  ptrdiff_t l = str->len;
  int s1 = 0;

  switch(str->size_shift) {
  case 0:
    /* Simple case, just ASCII and latin1... */
    {
      int asc, lat;
      p_wchar0 c, *p = STR0(str);
      if(s->g[0].mode == MODE_94 && s->g[0].index == 0x12)
	asc = 1;
      else
	asc = 0;
      if(s->g[1].mode == MODE_96 && s->g[1].index == 0x11)
	lat = 1;
      else
	lat = 0;
      while(l--) {
	if((c=*p++)<0x80) {
#ifdef OPTIMIZE_ISO2022
	  /* See comment below... */
	  if(c=='\r' || c=='\n') {
	    if(s->g[0].mode != MODE_94 || s->g[0].index != 0x12) {
	      s->g[0].transl = NULL;
	      s->g[0].index = 0;
	      if(s->r[0].map != NULL) {
		free(s->r[0].map);
		s->r[0].map = NULL;
	      }
	    }
	    s->g[1].transl = NULL;
	    s->g[1].index = 0;
	    if(s->r[1].map != NULL) {
	      free(s->r[1].map);
	      s->r[1].map = NULL;
	    }
	    lat = 0;
	  } else if(!asc && c>' ' && c<0x7f) {
#else
	  if(!asc) {
#endif
	    string_builder_strcat(&s->strbuild, "\033(B");
	    s->g[0].transl = map_ANSI_X3_4_1968;
	    s->g[0].mode = MODE_94;
	    s->g[0].index = 0x12;
	    if(s->r[0].map != NULL) {
	      free(s->r[0].map);
	      s->r[0].map = NULL;
	    }
	    asc = 1;
	  }
	} else if(!lat) {
	  string_builder_strcat(&s->strbuild, "\033-A");
	  s->g[1].transl = map_ISO_8859_1_1998;
	  s->g[1].mode = MODE_96;
	  s->g[1].index = 0x11;
	  if(s->r[1].map != NULL) {
	    free(s->r[1].map);
	    s->r[1].map = NULL;
	  }
	  lat = 1;
	}
	string_builder_putchar(&s->strbuild,c);
      }
    }
    break;
  case 1:
    s1++;
  case 2:
    {
      char *p = str->str;
      p_wchar2 c;

      while(l--) {
	if(s1) {
	  c = *(p_wchar1 *)p;
	  p += sizeof(p_wchar1);
	} else {
	  c = *(p_wchar2 *)p;
	  p += sizeof(p_wchar2);
	}
	if (c < 0) {
	  /* User reserved character. */
	} else
#ifdef OPTIMIZE_ISO2022
	/* This optimization breaks on some 2022 decoders,
	   such as the one in Netscape...  :-P */
	if(c<0x21 || c==0x7f) {
	  if(c=='\r' || c=='\n') {
	    if(s->g[0].mode != MODE_94 || s->g[0].index != 0x12) {
	      s->g[0].transl = NULL;
	      s->g[0].index = 0;
	      if(s->r[0].map != NULL) {
		free(s->r[0].map);
		s->r[0].map = NULL;
	      }
	    }
	    s->g[1].transl = NULL;
	    s->g[1].index = 0;
	    if(s->r[1].map != NULL) {
	      free(s->r[1].map);
	      s->r[1].map = NULL;
	    }
	  }
	  string_builder_putchar(&s->strbuild,c);
	} else if(c<0x7f)
#else
	if(c<0x80)
#endif
	{
	  /* USASCII */
	  if(s->g[0].mode != MODE_94 || s->g[0].index != 0x12) {
	    string_builder_strcat(&s->strbuild, "\033(B");
	    s->g[0].transl = map_ANSI_X3_4_1968;
	    s->g[0].mode = MODE_94;
	    s->g[0].index = 0x12;
	    if(s->r[0].map != NULL) {
	      free(s->r[0].map);
	      s->r[0].map = NULL;
	    }
	  }
	  string_builder_putchar(&s->strbuild,c);
	} else if(c<0x100) {
	  /* ISO-8859-1 */
	  if(s->g[1].mode != MODE_96 || s->g[1].index != 0x11) {
	    string_builder_strcat(&s->strbuild, "\033-A");
	    s->g[1].transl = map_ISO_8859_1_1998;
	    s->g[1].mode = MODE_96;
	    s->g[1].index = 0x11;
	    if(s->r[1].map != NULL) {
	      free(s->r[1].map);
	      s->r[1].map = NULL;
	    }
	  }
	  string_builder_putchar(&s->strbuild,c);
	} else if(c==0xfffd) {
	  /* Substitution character... */
	  REPLACE_CHAR(0xfffd,
		       s1 ? (p_wchar1 *) p - STR1(str) - 1 :
		       (p_wchar2 *) p - STR2(str) - 1);
	} else if(s->r[0].map != NULL && c >= s->r[0].lo && c < s->r[0].hi &&
		  s->r[0].map[c-s->r[0].lo]) {
	  /* Char contained in current G0 set */
	  if((c = s->r[0].map[c-s->r[0].lo])>=0x100)
	    string_builder_putchar(&s->strbuild,(c&0x7f00)>>8);
	  string_builder_putchar(&s->strbuild,c&0x7f);
	} else if(s->r[1].map != NULL && c >= s->r[1].lo && c < s->r[1].hi &&
		  s->r[1].map[c-s->r[1].lo]) {
	  /* Char contained in current G1 set */
	  if((c = s->r[1].map[c-s->r[1].lo])>=0x100)
	    string_builder_putchar(&s->strbuild,((c&0x7f00)>>8)|0x80);
	  string_builder_putchar(&s->strbuild,(c&0x7f)|0x80);
	} else {
	  /* Need to switch to another map */

	  int mode=0, index=0, ch, ch2;
	  const UNICHAR *ttab = NULL, *ttt;
	  p_wchar1 *rmap = NULL;

	  if(s->variant)
	    switch(s->variant) {
	    case VARIANT_JP2:
	      if(c >= 0xf900 || c == 0x02dc) {
		mode = MODE_9494;
		index = 0x13;
	      } else if(c < 0x200 || c >= 0x9fa1) {
		mode = MODE_9494;
		index = 0x14;		
	      } else if((c >= 0x386 && c < 0x400) || c == 0x201b) {
		mode = MODE_96;
		index = 0x16;
	      } else if(c < 0x2074) {
		if(c == 0x02c9) {
		  mode = MODE_9494;
		  index = 0x11;
		} else if(c == 0x0401 || (c>=0x0410 && c<=0x0451)) {
		  mode = MODE_9494;
		  index = 0x12;
		} else {
		  mode = MODE_9494;
		  index = 0x14;
		}
	      } else switch((jp2_tab[(c-0x2010)>>4]>>((c&15)<<1))&3) {
	      case 0:
		mode = MODE_9494;
		switch(c) {
		case 0x2039: case 0x203a: case 0x203e: case 0x223c:
		case 0x22ef: case 0x2329: case 0x232a: case 0x2571:
		case 0x2572: case 0x25ca: case 0x3004: case 0x30f8:	
		  index = 0x10;
		  break;
		default:
		  index = 0x12;
		}
		break;
	      case 1:
		mode = MODE_9494;
		index = 0x14;
		break;
	      case 2:
		mode = MODE_9494;
		index = 0x11;
		break;
	      case 3:
		mode = MODE_9494;
		index = 0x13;
		break;
	      }
	      break;		
	    case VARIANT_JP:
	      switch(c) {
	      case 0x2013: case 0x2039: case 0x203a: case 0x203e:
	      case 0x2225: case 0x223c: case 0x2264: case 0x2265:
	      case 0x22ef: case 0x2329: case 0x232a: case 0x2571:
	      case 0x2572: case 0x25ca: case 0x3004: case 0x30f8:
		mode = MODE_9494;
		index = 0x10;
		break;
	      default:
		mode = MODE_9494;
		index = 0x12;
	      }
	      break;
	    case VARIANT_KR:
	      mode = MODE_9494;
	      index = 0x13;
	      break;
	    case VARIANT_CN:
	      mode = MODE_9494;
	      index = 0x11;
	    }
	  else if(c>=0x3000) {

	    /* CJK */

	    if(c < 0x9fa1)
	      switch((jp2_tab[(c-0x2010)>>4]>>((c&15)<<1))&3) {
	      case 0:
		mode = MODE_9494;
		index = (c==0x3004 || c==0x30f8? 0x10 : 0x12);
		break;
	      case 1:
		mode = MODE_9494;
		index = 0x14;
		break;
	      case 2:
		mode = MODE_9494;
		index = 0x11;
		break;
	      case 3:
		mode = MODE_9494;
		index = 0x13;
		break;
	      }
	    else if(c >= 0xf900) {
	      mode = MODE_9494;
	      index = 0x13;
	    } else {
	      mode = MODE_9494;
	      index = 0x14;		
	    }

	  } else if(c<0x180) {
	    /* Latin-Extended-A */
	    /* Bitmap from character to 2 bit index in map2. */
	    unsigned char map1[] = {
	      0x02, 0x00, 0x15, 0x00, 0xa0, 0xa0, 0x02, 0xff,
	      0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 0x80, 0xc0,
	      0x00, 0x08, 0xfc, 0x03, 0x30, 0x20, 0x00, 0x15,
	      0x00, 0xf0, 0xff, 0x03, 0xf0, 0xff, 0x00, 0x00 };
	    unsigned char map2[] = {
	      0x12, /* iso-8859-2:1999 */
	      0x13, /* iso-8859-3:1999 */
	      0x14, /* iso-8859-4:1999 */
	      0x20, /* iso-8859-sup */
	    };
	    mode = MODE_96;
	    index = map2[(map1[(c-0x100)>>2]>>((c&3)<<1))&3];
	  } else if (c < 0x250) {
	    /* Latin-extended-B */
	    if ((c >= 0x01de && c <= 0x01ef) || (c == 0x01b7)) {
	      /* Latin-lap
	       *   0x01b7
	       *   0x01de - 0x01ef
	       */
	      mode = MODE_96;
	      index = 0x40;	/* latin_lap */
	    } else {
	      /* JIS_X0212_1990
	       *   0x01cd - 0x01d6
	       *   0x01d8 - 0x01dc
	       *   0x01f5
	       */
	      mode = MODE_9494;
	      index = 0x14;	/* JIS_X0212_1990 */
	    }
	  } else if (c < 0x2b0) {
	    /* IPA extensions. */
	  } else if (c < 0x300) {
	    /* Spacing Modifier Letters */
	    if ((c >= 0x02d8 && c <= 0x02dd) || c == 0x02c7) {
	      /* KS_C_5601_1987
	       *   0x02c7 0x02d8 - 0x02dd
	       */
	      mode = MODE_9494;
	      index = 0x13;	/* KS_C_5601_1987 */
	      /* Alternatives:
	       *
	       * ISO-8859-2 & CSN_369103
	       *   0x02c7 0x02d8 0x02d9 0x02db 0x02dd
	       * JIS_X0112_1990
	       *   0x02c7 0x02d8 - 0x02db 0x02dd
	       */
	    } else if (c == 0x02bf) {
	      /* Latin-lap
	       *   0x02bf
	       */
	      mode = MODE_96;
	      index = 0x28;	/* latin-lap */
	    } else {
	      /* GB_2312_80
	       *   0x02c7 0x02c9
	       */
	      mode = MODE_9494;
	      index = 0x11;	/* GB_2312_80 */
	    }
	  } else if (c < 0x370) {
	    /* Combining Diacritical Marks */
	  } else if (c < 0x400) {
	    /* Greek */
	    mode = MODE_9494;
	    index = 0x12;	/* JIS_C6226_1983 */
	  } else if (c < 0x500) {
	    /* Cyrillic */
	    if ((c >= 0x0402 && c <= 0x040f) ||
		(c >= 0x0452 && c <= 0x045f)) {
	      /* JIS_X0212_1990
	       *   0x0402 - 0x040f
	       *   0x0452 - 0x045f
	       */
	      mode = MODE_9494;
	      index = 0x14;	/* JIS_X0212_1990 */
	    } else {
	      /* JIS_C6226_1983
	       *   0x0401
	       *   0x0410 - 0x044f
	       *   0x0451
	       */
	      mode = MODE_9494;
	      index = 0x12;	/* JIS_C6226_1983 */
	    }
	  } else if (c < 0x590) {
	    /* Armenian */
	    /* ISO_8859_8_1999
	     *   0x05d0 - 0x05ea
	     */
	    mode = MODE_96;
	    index = 0x18;	/* ISO_8859_8_1999 */
	  } else if((c>=0x2010 && c<=0x22a5) ||
		    c==0x2312 ||
		    (c>=0x2500 && c<=0x266f)) {
	    /* FIXME:  Some of these chars might need a different set */
	    mode = MODE_9494;
	    index = 0x12;	/* JIS_C6226_1983 */
          } else {
	    /* Pike_error("Not implemented.\n"); */
	  }

	  if(index!=0 && (ttab = transltab[mode][index-0x10])!=NULL) {
	    switch(mode) {
	    case MODE_94:
	      rmap = (p_wchar1 *)xalloc((0x10000-0x100)*sizeof(p_wchar1));
	      memset(rmap, 0, (0x10000-0x100)*sizeof(p_wchar1));
	      for(ch=0; ch<94; ch++)
		if(ttab[ch]>=0x100 && ttab[ch]!=0xfffd)
		  rmap[ttab[ch]-0x100]=ch+33;
	      if(rmap[c-0x100]) {
		string_builder_strcat(&s->strbuild, "\033(");
		string_builder_putchar(&s->strbuild, 48+index);
		string_builder_putchar(&s->strbuild, rmap[c-0x100]);
		s->g[0].transl = ttab;
		s->g[0].mode = MODE_94;
		s->g[0].index = index;
		if(s->r[0].map != NULL)
		  free(s->r[0].map);
		s->r[0].map = rmap;
		s->r[0].lo = 0x100;
		s->r[0].hi = 0x10000;
	      } else
		ttab = NULL;
	      break;
	    case MODE_96:
	      rmap = (p_wchar1 *)xalloc((0x10000-0x100)*sizeof(p_wchar1));
	      memset(rmap, 0, (0x10000-0x100)*sizeof(p_wchar1));
	      for(ch=0; ch<96; ch++)
		if(ttab[ch]>=0x100 && ttab[ch]!=0xfffd)
		  rmap[ttab[ch]-0x100]=ch+32;
	      if(rmap[c-0x100]) {
		string_builder_strcat(&s->strbuild, "\033-");
		string_builder_putchar(&s->strbuild, 48+index);
		string_builder_putchar(&s->strbuild, rmap[c-0x100]|0x80);
		s->g[1].transl = ttab;
		s->g[1].mode = MODE_96;
		s->g[1].index = index;
		if(s->r[1].map != NULL)
		  free(s->r[1].map);
		s->r[1].map = rmap;
		s->r[1].lo = 0x100;
		s->r[1].hi = 0x10000;
	      } else
		ttab = NULL;
	      break;
	    case MODE_9494:
	      rmap = (p_wchar1 *)xalloc((0x10000-0x100)*sizeof(p_wchar1));
	      memset(rmap, 0, (0x10000-0x100)*sizeof(p_wchar1));
	      for(ttt=ttab, ch=0; ch<94; ch++)
		for(ch2=0; ch2<94; ch2++, ttt++)
		if(*ttt>=0x100 && *ttt!=0xfffd)
		  rmap[*ttt-0x100]=((ch+33)<<8)|(ch2+33);
	      if(rmap[c-0x100]) {
		/* Argh.  This should really be `\033$(', but that won't work with
		   Netscape (yet again)... */
		string_builder_strcat(&s->strbuild, "\033$");
		string_builder_putchar(&s->strbuild, 48+index);
		string_builder_putchar(&s->strbuild, rmap[c-0x100]>>8);
		string_builder_putchar(&s->strbuild, rmap[c-0x100]&0xff);
		s->g[0].transl = ttab;
		s->g[0].mode = MODE_9494;
		s->g[0].index = index;
		if(s->r[0].map != NULL)
		  free(s->r[0].map);
		s->r[0].map = rmap;
		s->r[0].lo = 0x100;
		s->r[0].hi = 0x10000;
	      } else
		ttab = NULL;
	      break;
	    case MODE_9696:
	      rmap = (p_wchar1 *)xalloc((0x10000-0x100)*sizeof(p_wchar1));
	      memset(rmap, 0, (0x10000-0x100)*sizeof(p_wchar1));
	      for(ttt=ttab, ch=0; ch<96; ch++)
		for(ch2=0; ch2<96; ch2++, ttt++)
		if(*ttt>=0x100 && *ttt!=0xfffd)
		  rmap[*ttt-0x100]=((ch+32)<<8)|(ch2+32);
	      if(rmap[c-0x100]) {
		string_builder_strcat(&s->strbuild, "\033$-");
		string_builder_putchar(&s->strbuild, 48+index);
		string_builder_putchar(&s->strbuild, (rmap[c-0x100]>>8)|0x80);
		string_builder_putchar(&s->strbuild, (rmap[c-0x100]&0xff)|0x80);
		s->g[1].transl = ttab;
		s->g[1].mode = MODE_9696;
		s->g[1].index = index;
		if(s->r[1].map != NULL)
		  free(s->r[1].map);
		s->r[1].map = rmap;
		s->r[1].lo = 0x100;
		s->r[1].hi = 0x10000;
	      } else
		ttab = NULL;
	      break;
	    }
	  }
	  if(ttab == NULL) {
	    if(rmap != NULL)
	      free(rmap);
	    REPLACE_CHAR(c,
			 s1 ? (p_wchar1 *) p - STR1(str) - 1 :
			 (p_wchar2 *) p - STR2(str) - 1);
	  }
	}
      }
    }
    break;
  default:
    Pike_fatal("Illegal shift size!\n");
  }
}

static void f_feed(INT32 args)
{
  struct pike_string *str;

  get_all_args(PRGM_NAME"Dec->feed()", args, "%S", &str);

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
  extern UNICHAR map_ISO_8859_1_1998[];
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
  s->g[1].transl = map_ISO_8859_1_1998;
  s->g[1].mode = MODE_96;
  s->g[1].index = 0x11;

  if(s->retain != NULL) {
    free_string(s->retain);
    s->retain = NULL;
  }

  reset_string_builder(&s->strbuild);
  
  push_object(this_object());
}


static void f_enc_feed(INT32 args)
{
  struct pike_string *str;

  get_all_args(PRGM_NAME"Enc->feed()", args, "%W", &str);
  if( str->len )
    eat_enc_string(str, (struct iso2022enc_stor *)fp->current_storage,
		   ((struct iso2022enc_stor *)fp->current_storage)->replace,
		   (((struct iso2022enc_stor *)fp->current_storage)->repcb.type
		    == T_FUNCTION?
		    &((struct iso2022enc_stor *)fp->current_storage)->repcb :
		    NULL));
  pop_n_elems(args);
  push_object(this_object());
}

static void f_enc_drain(INT32 args)
{
  struct iso2022enc_stor *s = (struct iso2022enc_stor *)fp->current_storage;

  pop_n_elems(args);
  push_string(finish_string_builder(&s->strbuild));
  init_string_builder(&s->strbuild, 0);
}

static void f_enc_clear(INT32 args)
{
  extern UNICHAR map_ANSI_X3_4_1968[];
  struct iso2022enc_stor *s = (struct iso2022enc_stor *)fp->current_storage;
  int i;

  pop_n_elems(args);

  for(i=0; i<2; i++) {
    s->g[i].transl = NULL;
    s->g[i].mode = MODE_96;
    s->g[i].index = 0;
    if(s->r[i].map)
      free(s->r[i].map);
    s->r[i].map = NULL;
    s->r[i].lo = 0;
    s->r[i].hi = 0;
  }
  s->g[0].transl = map_ANSI_X3_4_1968;
  s->g[0].mode = MODE_94;
  s->g[0].index = 0x12;

  reset_string_builder(&s->strbuild);
  
  push_object(this_object());
}

static void select_encoding_parameters(struct iso2022enc_stor *s,
				       struct pike_string *str)
{
  char *var;
  if(str == NULL || str->size_shift)
    Pike_error("Invalid ISO2022 encoding variant\n");
  var = (char *)STR0(str);
  if(!*var)
    s->variant = 0;
  else if(!strcmp(var, "jp"))
    s->variant = VARIANT_JP;
  else if(!strcmp(var, "cn") || !strcmp(var, "cnext"))
    s->variant = VARIANT_CN;
  else if(!strcmp(var, "kr"))
    s->variant = VARIANT_KR;
  else if(!strcmp(var, "jp2"))
    s->variant = VARIANT_JP2;
  else
    Pike_error("Invalid ISO2022 encoding variant\n");
}

static void f_create(INT32 args)
{
  struct iso2022enc_stor *s = (struct iso2022enc_stor *)fp->current_storage;

  check_all_args("create()", args, BIT_STRING, BIT_STRING|BIT_VOID|BIT_INT,
		 BIT_FUNCTION|BIT_VOID|BIT_INT, 0);

  select_encoding_parameters(s, sp[-args].u.string);

  if(args>1 && sp[1-args].type == T_STRING) {
    if(s->replace != NULL)
      free_string(s->replace);
    add_ref(s->replace = sp[1-args].u.string);
  }

  if(args>2 && sp[2-args].type == T_FUNCTION)
    assign_svalue(&s->repcb, &sp[2-args]);

  pop_n_elems(args);
  push_int(0);
}

static void f_set_repcb(INT32 args)
{
  struct iso2022enc_stor *s = (struct iso2022enc_stor *)fp->current_storage;

  check_all_args("set_replacement_callback()", args,
		 BIT_FUNCTION|BIT_INT, 0);

  if(args>0)
    assign_svalue(&s->repcb, &sp[-args]);

  pop_n_elems(args);
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

static void init_enc_stor(struct object *o)
{
  struct iso2022enc_stor *s = (struct iso2022enc_stor *)fp->current_storage;
  int i;

  s->replace = NULL;

  init_string_builder(&s->strbuild,0);

  for(i=0; i<2; i++)
    s->r[i].map = NULL;
  f_enc_clear(0);
  pop_n_elems(1);
}

static void exit_enc_stor(struct object *o)
{
  struct iso2022enc_stor *s = (struct iso2022enc_stor *)fp->current_storage;
  int i;

  for(i=0; i<2; i++) {
    if(s->r[i].map)
      free(s->r[i].map);
  }

  if(s->replace != NULL) {
    free_string(s->replace);
    s->replace = NULL;
  }

  reset_string_builder(&s->strbuild);
  free_string(finish_string_builder(&s->strbuild));
}

void iso2022_init(void)
{
  start_new_program();
  ADD_STORAGE(struct iso2022_stor);
  /* function(string:object) */
  ADD_FUNCTION("feed", f_feed,tFunc(tStr,tObj), 0);
  /* function(:string) */
  ADD_FUNCTION("drain", f_drain,tFunc(tNone,tStr), 0);
  /* function(:object) */
  ADD_FUNCTION("clear", f_clear,tFunc(tNone,tObj), 0);
  set_init_callback(init_stor);
  set_exit_callback(exit_stor);
  add_program_constant("ISO2022Dec", iso2022dec_program = end_program(),
		       ID_STATIC|ID_NOMASK);

  start_new_program();
  ADD_STORAGE(struct iso2022enc_stor);
  /* function(string:object) */
  ADD_FUNCTION("feed", f_enc_feed,tFunc(tStr,tObj), 0);
  /* function(:string) */
  ADD_FUNCTION("drain", f_enc_drain,tFunc(tNone,tStr), 0);
  /* function(:object) */
  ADD_FUNCTION("clear", f_enc_clear,tFunc(tNone,tObj), 0);
  /* function(string|void,function(string:string)|void:void) */
  ADD_FUNCTION("create", f_create,tFunc(tStr tOr(tStr,tVoid) tOr(tFunc(tStr,tStr),tVoid),tVoid), 0);
  /* function(function(string:string):void) */
  ADD_FUNCTION("set_replacement_callback", f_set_repcb,tFunc(tFunc(tStr,tStr),tVoid), 0);
  map_variable("_repcb", "function(string:string)", ID_STATIC,
	       OFFSETOF(iso2022enc_stor, repcb), T_MIXED);
  set_init_callback(init_enc_stor);
  set_exit_callback(exit_enc_stor);
  add_program_constant("ISO2022Enc", iso2022enc_program = end_program(),
		       ID_STATIC|ID_NOMASK);
}

void iso2022_exit(void)
{
  if(iso2022dec_program) {
    free_program(iso2022dec_program);
    iso2022dec_program = NULL;
  }
  if(iso2022enc_program) {
    free_program(iso2022enc_program);
    iso2022enc_program = NULL;
  }
}
