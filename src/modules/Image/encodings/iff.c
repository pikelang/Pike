/* $Id: iff.c,v 1.1 1999/04/06 17:24:32 marcus Exp $ */

#include "global.h"

#include "stralloc.h"
RCSID("$Id: iff.c,v 1.1 1999/04/06 17:24:32 marcus Exp $");
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "error.h"


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
    push_string(make_shared_binary_string(hdr, 4));
    push_string(make_shared_binary_string(data, clen));
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

  low_parse_iff(data+12, len-12, data, m, stopchunk);
}
