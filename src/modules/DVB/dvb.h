/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
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

/* pts_dts flags */
#define PTS_ONLY         0x80
#define PTS_DTS          0xC0

/* flags2 */
#define PTS_DTS_FLAGS    0xC0
#define ESCR_FLAG        0x20
#define ES_RATE_FLAG     0x10
#define DSM_TRICK_FLAG   0x08
#define ADD_CPY_FLAG     0x04
#define PES_CRC_FLAG     0x02
#define PES_EXT_FLAG     0x01


#define P2P_LENGTH 2048

enum{ 
	DVB_PES_TYPE_NOPES,
	DVB_PES_TYPE_AUDIO,
	DVB_PES_TYPE_VIDEO,
	DVB_PES_TYPE_PRIVATE
};

struct dvb_es_packet {
	unsigned char	*payload;
	int		 payload_len;
	int		 ptype;
	unsigned char	 attr[2];
	int		 skipped;
};

extern int dvb_pes2es(unsigned char *bufin, int count, struct dvb_es_packet *pkt, int id);


#endif
