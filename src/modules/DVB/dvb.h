/*
 *
 * $Id: dvb.h,v 1.2 2002/10/08 18:45:41 hop Exp $
 */

#ifndef DVB_H
#define DVB_H

/*#define DVB_DEBUG 1*/

#define AUDIO_BLOCK_SIZE 4096

#define MAX_PLENGTH 0xFFFF
#define MMAX_PLENGTH (8*MAX_PLENGTH)

#define PROG_STREAM_MAP  0xBC
#ifndef PRIVATE_STREAM1
#define PRIVATE_STREAM1  0xBD
#endif
#define PADDING_STREAM   0xBE
#ifndef PRIVATE_STREAM2
#define PRIVATE_STREAM2  0xBF
#endif
#define AUDIO_STREAM_S   0xC0
#define AUDIO_STREAM_E   0xDF
#define VIDEO_STREAM_S   0xE0
#define VIDEO_STREAM_E   0xEF
#define ECM_STREAM       0xF0
#define EMM_STREAM       0xF1
#define DSM_CC_STREAM    0xF2
#define ISO13522_STREAM  0xF3
#define PROG_STREAM_DIR  0xFF

//pts_dts flags 
#define PTS_ONLY         0x80
#define PTS_DTS          0xC0

//flags2
#define PTS_DTS_FLAGS    0xC0
#define ESCR_FLAG        0x20
#define ES_RATE_FLAG     0x10
#define DSM_TRICK_FLAG   0x08
#define ADD_CPY_FLAG     0x04
#define PES_CRC_FLAG     0x02
#define PES_EXT_FLAG     0x01


#define P2P_LENGTH 2048

	enum{NOPES, AUDIO, VIDEO};

	typedef struct dvb_avpstruct {
		int size;
		uint8_t *data;
	} dvb_avpacket;

	typedef struct dvb_p2pstruct {
		int found;
		uint8_t buf[MMAX_PLENGTH];
		uint8_t cid;
		uint32_t plength;
		uint8_t plen[2];
		uint8_t flag1;
		uint8_t flag2;
		uint8_t hlength;
		uint8_t pts[5];
		int mpeg;
		uint8_t check;
		int es;
		int filter;
		int which;
		int done;
		int repack;
		uint16_t bigend_repack;
		int startv;
                int starta;
		int64_t apts;
                int64_t vpts;
		uint16_t pid;
		uint16_t pida;
		uint16_t pidv;
		uint8_t acounter;
		uint8_t vcounter;
		uint8_t count0;
		uint8_t count1;
		void *data;
	} dvb_p2p;


extern int init_p2p(dvb_p2p *p, int repack);
extern int get_pes_filt (uint8_t *buf, int count, dvb_p2p *p, dvb_avpacket *pkt);

#endif
