#pike __REAL_VERSION__

// $Id: vbuffer.pike,v 1.1 2002/05/29 21:59:11 hop Exp $

// Buffered I/O
//
// Author: Honza Petrous, hop@unibase.cz
//
// 

//#define MM_FILE_BUFFER_DEBUG
#ifdef MM_FILE_BUFFER_DEBUG
#define DEBUG(X, Y ...) werror("Multimedia.File.buffer: " + X, Y)
#else
#define DEBUG(X, Y ...)
#endif


#define BSIZE 8192

//! Buffer object for unified access to MultiMedia.File object represented
//! by Stdio.File object or string

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
    return sprintf("Multimedia.File.buffer(%s)", fd ? 
    	sprintf("%O", fd) : "string(" + sizeof(origbuf) + ")" );
  }
