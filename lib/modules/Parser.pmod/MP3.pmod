#pike __REAL_VERSION__

// $Id: MP3.pmod,v 1.1 2002/04/18 09:06:15 hop%unibase.cz Exp $

// MP3 file parser
//
// Author: Honza Petrous, hop@unibase.cz
//
// Based on Per Hedbor's MP3 parser in original Icecast module for Roxen

//#define PARSER_MP3_DEBUG
#ifdef PARSER_MP3_DEBUG
#define DEBUG(X, Y ...) werror("Parser.MP3: " + X, Y)
#else
#define DEBUG(X, Y ...)
#endif


#define BSIZE 8192

//! A MP3 file parser with ID3 tag support.
class File {

  /*private*/ Buffer buffer;
  private int metainterval;
  private int new_meta;
  private string metadata;
  private mapping peekdata;
  private int start = 1;
  private int nochk;

  void create(Stdio.File|string fd, int|void nocheck) {
    nochk = nocheck;
    buffer = Buffer(fd);
    if(!nocheck)
      if(!mappingp(peekdata = get_frame()))
        error("No MP3 file.\n");
  }

  string _sprintf(int|void ctype) {
    return buffer->fd ? 
      sprintf("Parser.MP3.File(\"%O\",%O)", buffer->fd, nochk) :
      sprintf("Parser.MP3.File(string(%d),%O)", sizeof(buffer->origbuf), nochk);
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
  //!	  "channel":0,
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

    if(start && buffer->is_file() && !buffer->tell())
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
		"channel": getbits(2),
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

  // Decodes a synchsafe integer
  private int ss2int(array(int) bytes) {
    int res;
    DEBUG("ss2int: %O\n", bytes);
    foreach(bytes, int byte)
      res = res << 7 | byte;
    DEBUG("ss2int: ret=%O\n", res);
    return res;
  }
      
}

//! Buffer object for unified access to MP3 file represented
//! by Stdio.File object or string
class Buffer {

  Stdio.File fd;
  private string buffer;
  string origbuf;
  private int bpos;
  private int abspos;

  //! Creates Buffer object
  //!
  //! @param _fd
  //!  The Stdio.File object or string contained MP3 file
  void create(Stdio.File|string _fd) {
    if(objectp(_fd))
      fd = _fd;
    else {
      buffer = _fd;
      origbuf = _fd;
    }
  }

  //! Returns the current offset in MP3 file
  //!
  //! @note
  //!  Works only with Stdio.File object
  int tell() {
    if(fd) {
      int fpos = fd->tell();
      return fpos + (buffer ? (BSIZE - bpos) : 0) ;
    }
    //FIXME !
    error("No implemented for non Stdio.File source.\n");
  }

  //! Seek to a specified offset in a MP3 file
  //!
  //! @returns
  //!  Returns the new offset, or -1 on failure.
  int seek(int val) {
    if(fd) {
      buffer = 0;
      return fd->seek(val);
    }
    if(val > strlen(origbuf))
      return -1;
    buffer = origbuf;
    return val;
  }
    
  //! Peeks data from buffer
  //!
  //! @param n
  //!  Number of bytes
  //!
  //! @returns
  //!  Returns data or -1 on failure
  string|int peek(int n) {
    int bsav = bpos;
    string rv = getbytes(n, 1);
    if(stringp(rv)) {
      buffer = rv + buffer;
    }
    return rv;
  }

  //! Gets data from buffer
  //!
  //! @param n
  //!  Number of bytes
  //!
  //! @param s
  //!  Flag for type of returned data.
  //!  1 = string, 0 or void = integer
  string|int getbytes( int n, int|void s ) {
    DEBUG("getbytes: n: %d, s: %d\n", n, s);
    if( !buffer || !strlen(buffer) ) {
      if(!fd)
        return -1;
      bpos = 0;
      buffer = fd->read( BSIZE );
    }
    if( !strlen(buffer) )
      return s?0:-1;
    if( s ) {
      if( strlen(buffer) - bpos > n ) {
	string d = buffer[bpos..bpos+n-1];
	buffer = buffer[bpos+n..];
	bpos=0;
	return d;
      }
      else {
        if(!fd)
          return 0;
	buffer = buffer[bpos..];
	bpos=0;
	string t = fd->read( BSIZE );
	if( !t || !strlen(t) )
	  return -1;
	buffer+=t;
	return getbytes(n,1);
      }
    }
    int res=0;
    while( n-- ) {
      res<<=8;
      res|=buffer[ bpos++ ];
      if( bpos == strlen(buffer) ) {
        if(!fd)
          return -1;
	bpos = 0;
	buffer = fd->read( BSIZE );
	if( !buffer || !strlen( buffer ) )
	  return -1;
      }
    }
    return res;
  }

  //! Checks if object is using Stdio.File or string for MP3 file
  //! source
  //!
  //! @returns
  //!  Returns true, if the object is Stdio.File
  int is_file() {
    return objectp(fd);
  }

  string _sprintf() {
    return sprintf("Parser.MP3.Buffer(%s)", fd ? 
    	sprintf("%O", fd) : "string(" + sizeof(origbuf) + ")" );
  }
}
