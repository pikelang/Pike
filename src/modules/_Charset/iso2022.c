/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "global.h"
RCSID("$Id$");
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
  UNICHAR *transl;
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
  struct pike_string *replace;
  struct string_builder strbuild;
  struct svalue repcb;
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

#define REPLACE_CHAR(ch) \
          if(repcb != NULL && call_repcb(repcb, ch)) { \
	    eat_enc_string(sp[-1].u.string, s, rep, NULL); \
            pop_stack(); \
	  } else if(rep != NULL) \
            eat_enc_string(rep, s, NULL, NULL); \
	  else \
	    Pike_error("Character %lu unsupported by encoding.\n", \
		       (unsigned long) ch);


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
	} else
#endif
	if(c<0x7f) {
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
	  REPLACE_CHAR(0xfffd);
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
	  UNICHAR *ttab = NULL, *ttt;
	  p_wchar1 *rmap = NULL;

	  if(c>=0x3000) {
	    /* CJK */

	    /* FIXME:  Need to support Chinese and Korean, too... */
	    mode = MODE_9494;
	    index = 0x12;

	  } else if(c<0x180) {
	    unsigned char map1[] = {
	      0x02, 0x00, 0x15, 0x00, 0xa0, 0xa0, 0x02, 0xff,
	      0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 0x80, 0xc0,
	      0x00, 0x08, 0xfc, 0x03, 0x30, 0x20, 0x00, 0x15,
	      0x00, 0xf0, 0xff, 0x03, 0xf0, 0xff, 0x00, 0x00 };
	    unsigned char map2[] = { 0x12, 0x13, 0x14, 0x20 };
	    mode = MODE_96;
	    index = map2[(map1[(c-0x100)>>2]>>((c&3)<<1))&3];
	  } else {
	    Pike_error("Not implemented.\n");
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
	    REPLACE_CHAR(c);
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

static void f_create(INT32 args)
{
  struct iso2022enc_stor *s = (struct iso2022enc_stor *)fp->current_storage;

  check_all_args("create()", args, BIT_STRING|BIT_VOID|BIT_INT,
		 BIT_FUNCTION|BIT_VOID|BIT_INT, 0);

  if(args>0 && sp[-args].type == T_STRING) {
    if(s->replace != NULL)
      free_string(s->replace);
    add_ref(s->replace = sp[-args].u.string);
  }

  if(args>1 && sp[1-args].type == T_FUNCTION)
    assign_svalue(&s->repcb, &sp[1-args]);

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

  s->replace = NULL;

  init_string_builder(&s->strbuild,0);

  f_enc_clear(0);
  pop_n_elems(1);
}

static void exit_enc_stor(struct object *o)
{
  struct iso2022enc_stor *s = (struct iso2022enc_stor *)fp->current_storage;

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
  ADD_FUNCTION("create", f_create,tFunc(tOr(tStr,tVoid) tOr(tFunc(tStr,tStr),tVoid),tVoid), 0);
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
