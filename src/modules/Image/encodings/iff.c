/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: iff.c,v 1.14 2004/03/06 00:06:59 nilsson Exp $
*/

#include "global.h"

#include "stralloc.h"
RCSID("$Id: iff.c,v 1.14 2004/03/06 00:06:59 nilsson Exp $");
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "array.h"
#include "pike_error.h"
#include "operators.h"
#include "builtin_functions.h"


#define sp Pike_sp

static ptrdiff_t low_parse_iff(unsigned char *data, ptrdiff_t len,
			       unsigned char *hdr,
			       struct mapping *m, unsigned char *stopchunk)
{
  ptrdiff_t clen;
  clen = (EXTRACT_CHAR(hdr+4)<<24)|(hdr[5]<<16)|(hdr[6]<<8)|hdr[7];

  if((clen & 0xffffffff) == 0xffffffff)
    clen = len;
  else {
    if(!memcmp(hdr, "FORM", 4))
      clen -= 4;
    if(clen > len)
      Pike_error("truncated file\n");
    else if(clen < 0)
      Pike_error("invalid chunk length\n");
  }

  if((!memcmp(hdr, "FORM", 4)) || (!memcmp(hdr, "LIST", 4))) {
    ptrdiff_t pos=0;
    while(pos+8 <= clen) {
      ptrdiff_t l = low_parse_iff(data+pos+8, clen-pos-8, data+pos,
				  m, stopchunk);
      if(!l)
	return 0;
      pos += l+8;
    }
  } else {
    push_string(make_shared_binary_string((char *)hdr, 4));
    push_string(make_shared_binary_string((char *)data, clen));
    mapping_insert(m, sp-2, sp-1);
    pop_n_elems(2);
    if(!memcmp(hdr, stopchunk, 4))
      return 0;
  }
  return clen + (clen & 1);
}

void parse_iff(char *id, unsigned char *data, ptrdiff_t len,
	       struct mapping *m, char *stopchunk)
{
  if(len<12 || memcmp("FORM", data, 4))
    Pike_error("invalid IFF FORM\n");

  if(memcmp(id, data+8, 4))
    Pike_error("FORM is not %s\n", id);

  low_parse_iff(data+12, len-12, data, m, (unsigned char *)stopchunk);
}

static struct pike_string *low_make_iff(struct svalue *s)
{
  size_t len;
  unsigned char lenb[4];

  if(s->type != T_ARRAY || s->u.array->size != 2 ||
     s->u.array->item[0].type != T_STRING ||
     s->u.array->item[1].type != T_STRING)
    Pike_error("invalid chunk\n");

  add_ref(s->u.array);
  push_array_items(s->u.array);
  len = sp[-1].u.string->len;
  lenb[0] = DO_NOT_WARN((unsigned char)((len & 0xff000000)>>24));
  lenb[1] = DO_NOT_WARN((unsigned char)((len & 0x00ff0000)>>16));
  lenb[2] = DO_NOT_WARN((unsigned char)((len & 0x0000ff00)>>8));
  lenb[3] = DO_NOT_WARN((unsigned char)(len & 0x000000ff));
  push_string(make_shared_binary_string((char *)lenb, 4));
  stack_swap();
  if(len&1) {
    push_string(make_shared_binary_string("\0", 1));
    f_add(4);
  } else
    f_add(3);
  return (--sp)->u.string;
}

struct pike_string *make_iff(char *id, struct array *chunks)
{
  struct pike_string *res;
  INT32 i;

  push_text("FORM");
  push_text(id);
  if(chunks->size > 0) {
    for(i=0; i<chunks->size; i++)
      push_string(low_make_iff(&chunks->item[i]));
    if(chunks->size > 1)
      f_add(chunks->size);
  } else
    push_text("");
  f_add(2);
  f_aggregate(2);
  res = low_make_iff(&sp[-1]);
  pop_stack();
  return res;
}
