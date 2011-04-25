/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
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
	int i = 0, plen, extf = 0, hlen;
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
	  case 0xC0: case 0xC1: case 0xC2: case 0xC3:
	  case 0xC4: case 0xC5: case 0xC6: case 0xC7:
	  case 0xC8: case 0xC9: case 0xCA: case 0xCB:
	  case 0xCC: case 0xCD: case 0xCE: case 0xCF:
	  case 0xD0: case 0xD1: case 0xD2: case 0xD3:
	  case 0xD4: case 0xD5: case 0xD6: case 0xD7:
	  case 0xD8: case 0xD9: case 0xDA: case 0xDB:
	  case 0xDC: case 0xDD: case 0xDE: case 0xDF:
	    extf = 1;
	    pkt->ptype = DVB_PES_TYPE_AUDIO;
	    break;
	  case 0xE0: case 0xE1: case 0xE2: case 0xE3:
	  case 0xE4: case 0xE5: case 0xE6: case 0xE7:
	  case 0xE8: case 0xE9: case 0xEA: case 0xEB:
	  case 0xEC: case 0xED: case 0xEE: case 0xEF:
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
