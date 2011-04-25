#pike __REAL_VERSION__

// $Id$

// MP3 file parser/formatter
//
// Author: Honza Petrous, hop@unibase.cz
//
// Based on Per Hedbor's MP3 parser in original Icecast module for Roxen

//#define AUDIO_FORMAT_DEBUG
#ifdef AUDIO_FORMAT_DEBUG
#define DEBUG(X, Y ...) werror("Audio.Format.MP3: " + X, Y)
#else
#define DEBUG(X, Y ...)
#endif


//! A MP3 file parser with ID3 tag support.

  inherit .module.ANY;
  private int metainterval;
  private int new_meta;
  private string metadata;
  private int start = 1;
  private int name;

  void create() { }

  private void real_read(Stdio.File|Stdio.FakeFile file, int|void nocheck) {
    nochk = nocheck;
    buffer = Audio.Format.vbuffer(file);
    if(!nocheck)
      if(!mappingp(peekdata = get_frame()))
        error("No MP3 file.\n");
  }

  int check_format() {
    return mappingp(peekdata) || mappingp(peekdata = get_frame());
  }

  mapping get_map() {
    return ([
    	"type": "audio/x-mpeg",
	"description": "MPEG Layer III",
	"codec_type": "MP3",
	"seek": buffer->tell() //FIXME: doesn't work
    ]);
  }

  private int rate_of(int r) {
    switch(r)
    {
      case 0: return 44100;
      case 1: return 48000;
      case 2: return 32000;
      default:return 44100;
    }
  }

  static array(array(int)) bitrates_map =
  ({
    ({0,32,64,96,128,160,192,224,256,288,320,352,384,416,448}),
    ({0,32,48,56,64,80,96,112,128,160,192,224,256,320,384}),
    ({0,32,40,48,56,64,80,96,112,128,160,192,224,256,320}),
  });

  static array(string) channels_map =
  ({ "stereo", "joint", "dual", "single" });

  string|int get_data(int maxlen) {
    mapping rv = get_frame();
    if(mappingp(rv))
      return rv->data;
    return 0;
  }

  //! Gets next frame from file
  //!
  //! Frame is represented by the following mapping.
  //! It contains from parsed frame headers and frame
  //! data itself.
  //!
  //!  ([ "bitrate": int
  //!	  "copyright": int(0..1),
  //!	  "data": frame_data,
  //!	  "emphasis": emphasis,
  //!	  "extension": 
  //!	  "channels":0,
  //!	  "id":1,
  //!	  "layer":3,
  //!	  "original": int(0..1),
  //!	  "padding": int(0..1),
  //!	  "private": int(0..1),
  //!	  "sampling": int
  //!   ])
  //!
  mapping|int get_frame() { 
    string data;
    int bitrate;
    int trate = 0;
    int patt = 0;
    int prot = 0;
    int by, p=0, sw=0;
    mapping rv;

    if(mappingp(peekdata)) {
      rv = peekdata;
      peekdata = 0;
      return rv;
    }

    if(start && !buffer->tell())
      if(streamed)
        find_frame();
      else
        skip_id3v2();
    start = 0;

    while( (by = buffer->getbytes( 1  )) > 0  ) {
      DEBUG("get_frame: getbytes = %O\n", by);
      patt <<= 8;
      patt |= by;
      p++;
      if( (patt & 0xfff0) == 0xfff0 )
      {
	int srate, channels, layer, ID, pad, blen;
	int header = ((patt&0xffff)<<16);
	if( (by = buffer->getbytes( 2 )) < 0 )
	  break;
	header |= by;

        int getbits(int n) {
          int res = 0;
    
          while( n-- ) {
            res <<= 1;
            if( header&(1<<31) )
              res |= 1;
            header<<=1;
          }
          return res;
        };

	string data = sprintf("%4c",header);
	patt=0;
	header <<= 12;
	ID = getbits(1); // version
	if(!ID) /* not MPEG1 */
	  continue;
	layer = (4-getbits(2));

	prot = getbits(1);

	bitrate = getbits(4); 
	srate = getbits(2);
      
	if((layer>3) || (layer<2) ||  (bitrate>14) || (srate>2))
	  continue;
      
	pad = getbits(1);
	rv = ([ "private": getbits(1),
		"channels": channels_map[ getbits(2) ],
		"extension": getbits(2),
		"copyright": getbits(1),
		"original": getbits(1),
		"emphasis": getbits(2)
	      ]);
	bitrate = bitrates_map[ layer-1 ][ bitrate ] * 1000;
	srate = rate_of( srate );

	switch( layer )
	{
	  case 1:
	    blen = (int)(12 * bitrate / (float)srate + (pad?4:0))-4;
	    break;
	  case 2:
	  case 3:
	    blen = (int)(144 * bitrate / (float)srate + pad )-4;
	    break;
	}

	string q = buffer->getbytes( blen,1 );
	if(!q)
	  return 0;
	return ([ "data": data + q,
			"id": ID,
			"layer": layer,
			"bitrate": bitrate,
			"padding": pad,
			"sampling": srate,

		]) + rv;
      }
    }
    return 0;
  }

  // Skips ID3 version 2 tags at beginning of file
  private int skip_id3v2() {
	string buf = buffer->peek(10);
	int nlen;
	if(buf[..2] == "ID3") {
	  nlen = ss2int( (array(int))buf[6..9] );
	  if(nlen)
	    buffer->getbytes(nlen + 10, 1);
	  while(buffer->peek(1) == "\0") {
	    //padding
	    buffer->getbytes(1, 1);
	    nlen++;
	  }
	} 
	return nlen ? nlen+10 : 0;
  }

  private int find_frame() {

    do {
      while(buffer->peek(1) != "\xff")
        buffer->getbytes(1, 1);
    } while((buffer->peek(1)[0] & 0xf8) != 0xf8);
    return 0;
  }


  // Decodes a synchsafe integer
  private int ss2int(array(int) bytes) {
    int res;
    DEBUG("ss2int: %O\n", bytes);
    foreach(bytes, int byte)
      res = res << 7 | byte;
    DEBUG("ss2int: ret=%O\n", res);
    return res;
  }

