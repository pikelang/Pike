/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: psparser.c,v 1.6 2002/11/08 12:06:19 hop Exp $
*/

/*
 *  DVB module, PES extractor
 * 
 *  Creator: Honza Petrous <hop@unibase.cz>
 *
 */

#include "dvb.h"

#include <stdio.h>
#include <sys/types.h>
#include <string.h>

int dvb_pes2es(unsigned char *bufin, int count, struct dvb_es_packet *pkt, int id) {

	unsigned char *cptr = bufin;
	int i = 0, plen, extf = 0, hlen, ptype;
	int skipped = 0;

	while(i+4 < count && (cptr[0] != 0 || cptr[1] != 0 || cptr[2] != 1 || cptr[3] != id)) {
	  i++;
	  cptr++;
	  skipped++;
	}
	if(i+4 > count)
	  return -1;
#ifdef DVB_DEBUG
	if(skipped)
	  printf("skipped: %d\n", skipped);
#endif

	switch(cptr[3]) {
	  case 0xC0 ... 0xDF:
	    extf = 1;
	    pkt->ptype = DVB_PES_TYPE_AUDIO;
	    break;
	  case 0xE0 ... 0xEF:
	    extf = 1;
	    pkt->ptype = DVB_PES_TYPE_VIDEO;
	    break;
	  case 0xBD:
	    extf = 1;
	    pkt->ptype = DVB_PES_TYPE_PRIVATE;
	    break;
	  default:
	    pkt->ptype = DVB_PES_TYPE_NOPES;
	    extf = 0;
	    break;
	}

	hlen = extf ? 9 : 6;
	if(i+hlen > count)
	  return -1;

	plen = (cptr[4] << 8) + cptr[5];
	if(extf) {
	  pkt->attr[0] = cptr[6];
	  pkt->attr[1] = cptr[7];
	  hlen += cptr[8];
	}

	if(i+plen+hlen > count)
	  return -1;

	memcpy(pkt->payload, cptr + hlen, plen-hlen+6);
	pkt->payload_len = plen-hlen+6;
	pkt->skipped = skipped;

	return i+plen+6;
}
