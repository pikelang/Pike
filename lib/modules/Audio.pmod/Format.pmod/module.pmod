#pike __REAL_VERSION__

// Audio.Format module
//
// Author: Honza Petrous, hop@unibase.cz
//
// $Id: module.pmod,v 1.1 2002/09/13 13:37:53 hop Exp $

//#define AUDIO_FORMAT_DEBUG
#ifdef AUDIO_FORMAT_DEBUG
#define DEBUG(X, Y ...) werror("Audio.Format: " + X, Y)
#else
#define DEBUG(X, Y ...)
#endif


class ANY {

  object buffer;
  int nochk;
  int streamed = 0;
  mixed peekdata = 0;

  void create() { }

  //!
  object read_file(string filename, int|void nocheck) {
    real_read(Stdio.File(filename), nocheck);
    return this_object();
  }

  //!
  object read_streamed(string filename, int|void nocheck) {
    streamed = 1;
    return read_file(filename, nocheck);
  }

  //!
  object read_string(string data, int|void nocheck) {
    real_read(Stdio.FakeFile(data), nocheck);
    return this_object();
  }

  void real_read(Stdio.File|Stdio.FakeFile file, int|void nocheck);

  //!
  //!
  //! @note
  //!  The operation is destructive. Ie. current data cursor is moved over.
  //!
  //! @seealso
  //!  @[get_data], @[get_sample]
  string get_frame();

  //!
  //!
  //! @note
  //!  The operation is destructive. Ie. current data cursor is moved over.
  //!
  //! @seealso
  //!  @[get_frame], @[get_data]
  string get_sample();

  //!
  //!
  //! @note
  //!  The operation is destructive. Ie. current data cursor is moved over.
  //!
  //! @seealso
  //!  @[get_frame], @[get_sample]
  string get_data();

  //!
  int check_format() {
    return 0;
  }

  string _sprintf(int|void ctype) {
      sprintf("Audio.Format.Any(\"%O\",%O)", buffer->fd, nochk);
  }

  //!
  mapping get_map() {
    return ([
    	"type": "audio/any",
	"description": "unknown",
	"codec_type": "unknown",
	"seek": -1
    ]);
  }


}

// Internal module for invisible file/string source support
class vbuffer {

#define BSIZE 8192

  Stdio.File fd;
  private string buffer;
  string origbuf;
  private int bpos;

  // Creates Buffer object
  //
  // @param _fd
  //  The Stdio.File object or string contained MP3 file
  void create(Stdio.File|Stdio.FakeFile _fd) {
    if(objectp(_fd))
      fd = _fd;
    else
      error("Argument 1 must be a Stdio.File or Stdio.FakeFile object.\n");
  }

  // Returns the current offset in file
  int tell() {
      int fpos = fd->tell();
      return fpos - (buffer ? (BSIZE - bpos) : 0) ;
  }

  // Seek to a specified offset in a file
  //
  // @returns
  //  Returns the new offset, or -1 on failure.
  int seek(int val) {
      return fd->seek(val);
  }
    
  // Peeks data from buffer
  //
  // @param n
  //  Number of bytes
  //
  // @returns
  //  Returns data or -1 on failure
  string|int peek(int n) {
    int bsav = bpos;
    string rv = getbytes(n, 1);
    if(stringp(rv)) {
      buffer = rv + buffer;
    }
    return rv;
  }

  // Gets data from buffer
  //
  // @param n
  //  Number of bytes
  //
  // @param s
  //  Flag for type of returned data.
  //  1 = string, 0 or void = integer
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

}
