/*
 *  DVB module, PES extractor
 * 
 *  Creator: Honza Petrous <hop@unibase.cz>
 * 
 *  $Id: psparser.c,v 1.2 2002/10/08 11:44:59 hop Exp $
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <netinet/in.h>

#include "dvb.h"

/* PES extracting utility */
/* (from excelent mpegtools package by Marcus Metzler) */

void pes_filt(dvb_p2p *p, dvb_avpacket *pkt)
{
  int factor = p->mpeg-1;
  int len;

  if ( p->cid == p->filter) {
    if (p->es) {
#ifdef DVB_DEBUG
    printf("DEB: dvb: pes_filt: plength: %d, hlength: %d, factor: %d\n",
	   p->plength, p->hlength, factor);
#endif
      len = p->plength-p->hlength-3*factor;
      memcpy(pkt->data + pkt->size, p->buf+p->hlength+6+3*factor, len);
    } else {
      len = p->plength+6;
      memcpy(pkt->data + pkt->size, p->buf, len);
    }
    pkt->size += len;
#ifdef DVB_DEBUG
    printf("DEB: dvb: pes_filt: len: %d (size=%d)\n", len, pkt->size);
#endif

  }
}

/*void init_p2p(dvb_p2p *p, void (*func)(uint8_t *buf, int count, dvb_p2p *p,
	      dvb_avpacket *pkt), int repack){*/
int init_p2p(dvb_p2p *p, int repack){
  p->found = 0;
  p->cid = 0;
  p->mpeg = 0;
  memset(p->buf,0,MMAX_PLENGTH);
  p->done = 0;
  p->bigend_repack = 0;
  p->repack = 0; 
  if ( repack < MAX_PLENGTH && repack > 265 ){
    p->repack = repack-6;
    p->bigend_repack = (uint16_t)htons((short)
					((repack-6) & 0xFFFF));
    return 0;
  }
  return repack; /* Repack size is out of range */
}


/*void get_pes (uint8_t *buf, int count, dvb_p2p *p, void (*func)(dvb_p2p *p,
	      dvb_avpacket *pkt), dvb_avpacket *pkt)*/
int get_pes_filt (uint8_t *buf, int count, dvb_p2p *p, dvb_avpacket *pkt)
{

	int l;
	unsigned short *pl;
	int c = 0, rv = 0;

	uint8_t headr[3] = { 0x00, 0x00, 0x01} ;

#ifdef DVB_DEBUG
    printf("DEB: dvb: get_pes_filt(pkt.size=%d): ...\n", pkt->size);
#endif
	while (c < count && (p->mpeg == 0 ||
			     (p->mpeg == 1 && p->found < 7) ||
			     (p->mpeg == 2 && p->found < 9))
	       &&  (p->found < 5 || !p->done)){
		switch ( p->found ){
		case 0:
		case 1:
			if (buf[c] == 0x00) p->found++;
			else p->found = 0;
			c++;
			break;
		case 2:
			if (buf[c] == 0x01) p->found++;
			else if (buf[c] == 0){
				p->found = 2;
			} else p->found = 0;
			c++;
			break;
		case 3:
			p->cid = 0;
			switch (buf[c]){
			case PROG_STREAM_MAP:
			case PRIVATE_STREAM2:
			case PROG_STREAM_DIR:
			case ECM_STREAM     :
			case EMM_STREAM     :
			case PADDING_STREAM :
			case DSM_CC_STREAM  :
			case ISO13522_STREAM:
				p->done = 1;
			case PRIVATE_STREAM1:
			case VIDEO_STREAM_S ... VIDEO_STREAM_E:
			case AUDIO_STREAM_S ... AUDIO_STREAM_E:
				p->found++;
				p->cid = buf[c];
				c++;
				break;
			default:
				p->found = 0;
				break;
			}
			break;
			

		case 4:
			if (count-c > 1){
				pl = (unsigned short *) (buf+c);
				p->plength =  ntohs(*pl);
				p->plen[0] = buf[c];
				c++;
				p->plen[1] = buf[c];
				c++;
				p->found+=2;
			} else {
				p->plen[0] = buf[c];
				p->found++;
				return c;
			}
			break;
		case 5:
			p->plen[1] = buf[c];
			c++;
			pl = (unsigned short *) p->plen;
			p->plength = ntohs(*pl);
			p->found++;
			break;


		case 6:
			if (!p->done){
				p->flag1 = buf[c];
				c++;
				p->found++;
				if ( (p->flag1 & 0xC0) == 0x80 ) p->mpeg = 2;
				else {
					p->hlength = 0;
					p->which = 0;
					p->mpeg = 1;
					p->flag2 = 0;
				}
			}
			break;

		case 7:
			if ( !p->done && p->mpeg == 2){
				p->flag2 = buf[c];
				c++;
				p->found++;
			}	
			break;

		case 8:
			if ( !p->done && p->mpeg == 2){
				p->hlength = buf[c];
				c++;
				p->found++;
			}
			break;
			
		default:

			break;
		}
	}
#ifdef DVB_DEBUG
    printf("DEB: dvb: get_pes_filt: cid:%d, mpeg:%d, found: %d\n", p->cid, p->mpeg, p->found);
#endif

	if (!p->plength) p->plength = MMAX_PLENGTH-6;


	if ( p->done || ((p->mpeg == 2 && p->found >= 9)  || 
	     (p->mpeg == 1 && p->found >= 7)) ){
		switch (p->cid){
			
		case AUDIO_STREAM_S ... AUDIO_STREAM_E:			
		case VIDEO_STREAM_S ... VIDEO_STREAM_E:
		case PRIVATE_STREAM1:

			memcpy(p->buf, headr, 3);
			p->buf[3] = p->cid;
			memcpy(p->buf+4,p->plen,2);

			if (p->mpeg == 2 && p->found == 9){
				p->buf[6] = p->flag1;
				p->buf[7] = p->flag2;
				p->buf[8] = p->hlength;
			}

			if (p->mpeg == 1 && p->found == 7){
				p->buf[6] = p->flag1;
			}


			if (p->mpeg == 2 && (p->flag2 & PTS_ONLY) &&  
			    p->found < 14){
				while (c < count && p->found < 14){
					p->pts[p->found-9] = buf[c];
					p->buf[p->found] = buf[c];
					c++;
					p->found++;
				}
				if (c == count) return c;
			}

			if (p->mpeg == 1 && p->which < 2000){

				if (p->found == 7) {
					p->check = p->flag1;
					p->hlength = 1;
				}

				while (!p->which && c < count && 
				       p->check == 0xFF){
					p->check = buf[c];
					p->buf[p->found] = buf[c];
					c++;
					p->found++;
					p->hlength++;
				}

				if ( c == count) return c;
				
				if ( (p->check & 0xC0) == 0x40 && !p->which){
					p->check = buf[c];
					p->buf[p->found] = buf[c];
					c++;
					p->found++;
					p->hlength++;

					p->which = 1;
					if ( c == count) return c;
					p->check = buf[c];
					p->buf[p->found] = buf[c];
					c++;
					p->found++;
					p->hlength++;
					p->which = 2;
					if ( c == count) return c;
				}

				if (p->which == 1){
					p->check = buf[c];
					p->buf[p->found] = buf[c];
					c++;
					p->found++;
					p->hlength++;
					p->which = 2;
					if ( c == count) return c;
				}
				
				if ( (p->check & 0x30) && p->check != 0xFF){
					p->flag2 = (p->check & 0xF0) << 2;
					p->pts[0] = p->check;
					p->which = 3;
				} 

				if ( c == count) return c;
				if (p->which > 2){
					if ((p->flag2 & PTS_DTS_FLAGS)
					    == PTS_ONLY){
						while (c < count && 
						       p->which < 7){
							p->pts[p->which-2] =
								buf[c];
							p->buf[p->found] = 
								buf[c];
							c++;
							p->found++;
							p->which++;
							p->hlength++;
						}
						if ( c == count) return c;
					} else if ((p->flag2 & PTS_DTS_FLAGS) 
						   == PTS_DTS){
						while (c < count && 
						       p->which< 12){
							if (p->which< 7)
								p->pts[p->which
								      -2] =
									buf[c];
							p->buf[p->found] = 
								buf[c];
							c++;
							p->found++;
							p->which++;
							p->hlength++;
						}
						if ( c == count) return c;
					}
					p->which = 2000;
				}
							
			}

			while (c < count && p->found < p->plength+6){
				l = count -c;
				if (l+p->found > p->plength+6)
					l = p->plength+6-p->found;
				memcpy(p->buf+p->found, buf+c, l);
				p->found += l;
				c += l;
			}			
#ifdef DVB_DEBUG
			printf("DEB: dvb: get_pes_filt: found:%d, plength: %d\n", p->found, p->plength);
#endif
			if(p->found == p->plength+6)
				pes_filt(p, pkt);
			
			break;
		}


		if ( p->done ){
			if( p->found + count - c < p->plength+6){
				p->found += count-c;
				c = count;
			} else {
				c += p->plength+6 - p->found;
				p->found = p->plength+6;
			}
		}

		if (p->plength && p->found == p->plength+6) {
			p->found = 0;
			p->done = 0;
			p->plength = 0;
			memset(p->buf, 0, MAX_PLENGTH);
#if 1
			if (c < count)
				rv = get_pes_filt(buf+c, count-c, p, pkt);
#endif
		}
	}
	return c + rv;
}
