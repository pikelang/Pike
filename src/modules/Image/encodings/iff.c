/* $Id: iff.c,v 1.4 2000/02/03 19:01:29 grubba Exp $ */

#include "global.h"

#include "stralloc.h"
RCSID("$Id: iff.c,v 1.4 2000/02/03 19:01:29 grubba Exp $");
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "array.h"
#include "error.h"
#include "operators.h"
#include "builtin_functions.h"


static INT32 low_parse_iff(unsigned char *data, INT32 len, unsigned char *hdr,
			   struct mapping *m, unsigned char *stopchunk)
{
  INT32 clen;
  clen = (EXTRACT_CHAR(hdr+4)<<24)|(hdr[5]<<16)|(hdr[6]<<8)|hdr[7];

  if(clen==-1)
    clen = len;
  else {
    if(!memcmp(hdr, "FORM", 4))
      clen -= 4;
    if(clen > len)
      error("truncated file\n");
    else if(clen < 0)
      error("invalid chunk length\n");
  }

  if((!memcmp(hdr, "FORM", 4)) || (!memcmp(hdr, "LIST", 4))) {
    INT32 pos=0;
    while(pos+8 <= clen) {
      INT32 l = low_parse_iff(data+pos+8, clen-pos-8, data+pos, m, stopchunk);
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

void parse_iff(char *id, unsigned char *data, INT32 len,
	       struct mapping *m, char *stopchunk)
{
  if(len<12 || memcmp("FORM", data, 4))
    error("invalid IFF FORM\n");

  if(memcmp(id, data+8, 4))
    error("FORM is not %s\n", id);

  low_parse_iff(data+12, len-12, data, m, (unsigned char *)stopchunk);
}

static struct pike_string *low_make_iff(struct svalue *s)
{
  INT32 len;
  unsigned char lenb[4];

  if(s->type != T_ARRAY || s->u.array->size != 2 ||
     s->u.array->item[0].type != T_STRING ||
     s->u.array->item[1].type != T_STRING)
    error("invalid chunk\n");

  add_ref(s->u.array);
  push_array_items(s->u.array);
  len = sp[-1].u.string->len;
  lenb[0] = (len & 0xff000000)>>24;
  lenb[1] = (len & 0x00ff0000)>>16;
  lenb[2] = (len & 0x0000ff00)>>8;
  lenb[3] = (len & 0x000000ff);
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
