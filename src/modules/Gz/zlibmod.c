/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: zlibmod.c,v 1.61 2003/03/30 20:45:35 nilsson Exp $
*/

#include "global.h"
RCSID("$Id: zlibmod.c,v 1.61 2003/03/30 20:45:35 nilsson Exp $");

#include "zlib_machine.h"
#include "module.h"

#if !defined(HAVE_LIBZ) && !defined(HAVE_LIBGZ)
#undef HAVE_ZLIB_H
#endif

#ifdef HAVE_ZLIB_H

#include "interpret.h"
#include "svalue.h"
#include "stralloc.h"
#include "array.h"
#include "pike_macros.h"
#include "program.h"
#include "stralloc.h"
#include "object.h"
#include "pike_types.h"
#include "threads.h"
#include "dynamic_buffer.h"

#include <zlib.h>


#define sp Pike_sp

struct zipper
{
  int  level;
  int  state;
  struct z_stream_s gz;
  gzFile gzfile;
#ifdef _REENTRANT
  DEFINE_MUTEX(lock);
#endif /* _REENTRANT */
};

#define BUF 32768
#define MAX_BUF	(64*BUF)

#undef THIS
#define THIS ((struct zipper *)(Pike_fp->current_storage))

/*! @module Gz
 *!
 *! The Gz module contains functions to compress and uncompress strings using
 *! the same algorithm as the program @tt{gzip@}. Compressing can be done in
 *! streaming mode or all at once.
 *!
 *! The Gz module consists of two classes; Gz.deflate and Gz.inflate.
 *! Gz.deflate is used to pack data
 *! and Gz.inflate is used to unpack data. (Think "inflatable boat")
 *!
 *! @note
 *!   Note that this module is only available if the gzip library was
 *!   available when Pike was compiled.
 *!
 *!   Note that although these functions use the same @i{algorithm@} as
 *!   @tt{gzip@}, they do not use the exact same format, so you cannot directly
 *!   unzip gzipped files with these routines. Support for this will be
 *!   added in the future.
 */

/*! @class deflate
 *!
 *! Gz_deflate is a builtin program written in C. It interfaces the
 *! packing routines in the libz library.
 *!
 *! @note
 *! This program is only available if libz was available and found when
 *! Pike was compiled.
 *!
 *! @seealso
 *! @[Gz.inflate()]
 */

/*! @decl void create(int(0..9)|void X)
 *!
 *! If given, @[X] should be a number from 0 to 9 indicating the
 *! packing / CPU ratio. Zero means no packing, 2-3 is considered 'fast',
 *! 6 is default and higher is considered 'slow' but gives better packing.
 *!
 *! This function can also be used to re-initialize a Gz.deflate object
 *! so it can be re-used.
 */
static void gz_deflate_create(INT32 args)
{
  int tmp;
  THIS->level=Z_DEFAULT_COMPRESSION;

  if(THIS->gz.state)
  {
/*     mt_lock(& THIS->lock); */
    deflateEnd(&THIS->gz);
/*     mt_unlock(& THIS->lock); */
  }

  if(args)
  {
    if(sp[-args].type != T_INT)
      Pike_error("Bad argument 1 to gz->create()\n");
    THIS->level=sp[-args].u.integer;
    if(THIS->level < Z_NO_COMPRESSION ||
       THIS->level > Z_BEST_COMPRESSION)
    {
      Pike_error("Compression level out of range for gz_deflate->create()\n");
    }
  }

  THIS->gz.zalloc=Z_NULL;
  THIS->gz.zfree=Z_NULL;
  THIS->gz.opaque=(void *)THIS;

  pop_n_elems(args);
/*   mt_lock(& THIS->lock); */
  tmp=deflateInit(&THIS->gz, THIS->level);
/*   mt_unlock(& THIS->lock); */
  switch(tmp)
  {
  case Z_OK:
    return;

  case Z_VERSION_ERROR:
    Pike_error("libz not compatible with zlib.h!!!\n");
    break;

  default:
    if(THIS->gz.msg)
      Pike_error("Failed to initialize gz_deflate: %s\n",THIS->gz.msg);
    else
      Pike_error("Failed to initialize gz_deflate\n");
  }
}

static int do_deflate(dynamic_buffer *buf,
		      struct zipper *this,
		      int flush)
{
   int ret=0;

   THREADS_ALLOW();
   mt_lock(& this->lock);
   THREADS_DISALLOW();
   if(!this->gz.state)
      ret=Z_STREAM_ERROR;
   else
      do
      {
	 this->gz.next_out=low_make_buf_space(
	    /* recommended by the zlib people */
	    (this->gz.avail_out =
	     this->gz.avail_in ?
	     this->gz.avail_in+this->gz.avail_in/1000+42 :
	      4096),
	    buf);

	 THREADS_ALLOW();
	 ret=deflate(& this->gz, flush);
	 THREADS_DISALLOW();

	 /* Absorb any unused space /Hubbe */
	 low_make_buf_space(-((ptrdiff_t)this->gz.avail_out), buf);

	 if(ret == Z_BUF_ERROR) ret=Z_OK;
      }
      while (ret==Z_OK && (this->gz.avail_in || !this->gz.avail_out));

   mt_unlock(& this->lock);
   return ret;
}

/*! @decl string deflate(string data, int|void flush)
 *!
 *! This function performs gzip style compression on a string @[data] and
 *! returns the packed data. Streaming can be done by calling this
 *! function several times and concatenating the returned data.
 *!
 *! The optional argument @[flush] should be one of the following:
 *! @int
 *!   @value Gz.NO_FLUSH
 *!     Only data that doesn't fit in the internal buffers is returned.
 *!   @value Gz.PARTIAL_FLUSH
 *!     All input is packed and returned.
 *!   @value Gz.SYNC_FLUSH
 *!     All input is packed and returned.
 *!   @value Gz.FINISH
 *!     All input is packed and an 'end of data' marker is appended.
 *! @endint
 *!
 *! @seealso
 *! @[Gz.inflate->inflate()]
 */
static void gz_deflate(INT32 args)
{
  struct pike_string *data;
  int flush, fail;
  struct zipper *this=THIS;
  dynamic_buffer buf;
  ONERROR err;

  if(THIS->state == 1)
  {
    deflateEnd(& THIS->gz);
    deflateInit(& THIS->gz, THIS->level);
    THIS->state=0;
  }

  if(!THIS->gz.state)
    Pike_error("gz_deflate not initialized or destructed\n");

  if(args<1)
    Pike_error("Too few arguments to gz_deflate->deflate()\n");

  if(sp[-args].type != T_STRING)
    Pike_error("Bad argument 1 to gz_deflate->deflate()\n");

  data=sp[-args].u.string;

  if(args>1)
  {
    if(sp[1-args].type != T_INT)
      Pike_error("Bad argument 2 to gz_deflate->deflate()\n");
    
    flush=sp[1-args].u.integer;

    switch(flush)
    {
    case Z_PARTIAL_FLUSH:
    case Z_FINISH:
    case Z_SYNC_FLUSH:
    case Z_NO_FLUSH:
      break;

    default:
      Pike_error("Argument 2 to gz_deflate->deflate() out of range.\n");
    }
  }else{
    flush=Z_FINISH;
  }

  this->gz.next_in=(Bytef *)data->str;
  this->gz.avail_in = DO_NOT_WARN((unsigned INT32)(data->len));

  initialize_buf(&buf);

  SET_ONERROR(err,toss_buffer,&buf);
  fail=do_deflate(&buf,this,flush);
  UNSET_ONERROR(err);
  
  if(fail != Z_OK && fail != Z_STREAM_END)
  {
    toss_buffer(&buf);
    if(THIS->gz.msg)
      Pike_error("Error in gz_deflate->deflate(): %s\n",THIS->gz.msg);
    else
      Pike_error("Error in gz_deflate->deflate(): %d\n",fail);
  }

  if(fail == Z_STREAM_END)
    THIS->state=1;

  pop_n_elems(args);

  push_string(low_free_buf(&buf));
}


static void init_gz_deflate(struct object *o)
{
  mt_init(& THIS->lock);
  MEMSET(& THIS->gz, 0, sizeof(THIS->gz));
  THIS->gz.zalloc=Z_NULL;
  THIS->gz.zfree=Z_NULL;
  THIS->gz.opaque=(void *)THIS;
  THIS->state=0;
  deflateInit(& THIS->gz, THIS->level = Z_DEFAULT_COMPRESSION);
}

static void exit_gz_deflate(struct object *o)
{
/*   mt_lock(& THIS->lock); */
  deflateEnd(&THIS->gz);
/*   mt_unlock(& THIS->lock); */
  mt_destroy( & THIS->lock );
}

/*! @endclass
 */

/*******************************************************************/

/*! @class inflate
 *!
 *! Gz_deflate is a builtin program written in C. It interfaces the
 *! unpacking routines in the libz library.
 *!
 *! @note
 *! This program is only available if libz was available and found when
 *! Pike was compiled.
 *!
 *! @seealso
 *!   @[deflate]
 */

/*! @decl void create(int|void magic)
 *! @param magic
 *! The magic value is passed down to inflateInit2 in zlib. Specifically,
 *! if you want to uncompress PKZIP-compressed data, you have to specify
 *! -15 as the argument. What negative arguments does is undocumented as
 *! far as we know. Positive arguments set the maximum dictionary size
 *! though.
 */
static void gz_inflate_create(INT32 args)
{
  int tmp;
  if(THIS->gz.state)
  {
/*     mt_lock(& THIS->lock); */
    inflateEnd(&THIS->gz);
/*     mt_unlock(& THIS->lock); */
  }


  THIS->gz.zalloc=Z_NULL;
  THIS->gz.zfree=Z_NULL;
  THIS->gz.opaque=(void *)THIS;
  if( args  && Pike_sp[-1].type == PIKE_T_INT )
  {
    tmp=inflateInit2(& THIS->gz, Pike_sp[-1].u.integer);
  }
  else
  {
    tmp=inflateInit( &THIS->gz );
  }
  pop_n_elems(args);

/*    mt_lock(& THIS->lock);  */
/*    mt_unlock(& THIS->lock); */
  switch(tmp)
  {
  case Z_OK:
    return;

  case Z_VERSION_ERROR:
    Pike_error("libz not compatible with zlib.h!!!\n");
    break;

  default:
    if(THIS->gz.msg)
      Pike_error("Failed to initialize gz_inflate: %s\n",THIS->gz.msg);
    else
      Pike_error("Failed to initialize gz_inflate\n");
  }
}

static int do_inflate(dynamic_buffer *buf,
		      struct zipper *this,
		      int flush)
{
  int fail=0;
  THREADS_ALLOW();
  mt_lock(& this->lock);
  THREADS_DISALLOW();
  if(!this->gz.state)
  {
    fail=Z_STREAM_ERROR;
  }else{
#if 0
  static int fnord=0;
  fnord++;
#endif

    do
    {
      char *loc;
      int ret;
      loc=low_make_buf_space(BUF,buf);
      THREADS_ALLOW();
      this->gz.next_out=(Bytef *)loc;
      this->gz.avail_out=BUF;
#if 0
      fprintf(stderr,"INFLATE[%d]: avail_out=%7d  avail_in=%7d flush=%d\n",
	      fnord,
	      this->gz.avail_out,
	      this->gz.avail_in,
	      flush);
      fprintf(stderr,"INFLATE[%d]: mode=%d\n",fnord,
	      this->gz.state ? *(int *)(this->gz.state) : -1);
#endif
	      
      ret=inflate(& this->gz, flush);
#if 0
      fprintf(stderr,"Result [%d]: avail_out=%7d  avail_in=%7d  ret=%d\n",
	      fnord,
	      this->gz.avail_out,
	      this->gz.avail_in,
	      ret);
#endif

      THREADS_DISALLOW();
      low_make_buf_space(-((ptrdiff_t)this->gz.avail_out), buf);

      if(ret == Z_BUF_ERROR) ret=Z_OK;

      if(ret != Z_OK)
      {
	fail=ret;
	break;
      }
    } while(!this->gz.avail_out || flush==Z_FINISH || this->gz.avail_in);
  }
  mt_unlock(& this->lock);
  return fail;
}

/*! @decl string inflate(string data)
 *!
 *! This function performs gzip style decompression. It can inflate
 *! a whole file at once or in blocks.
 *!
 *! @example
 *! // whole file
 *! write(Gz_inflate()->inflate(stdin->read(0x7fffffff));
 *!
 *! // streaming (blocks)
 *! function inflate=Gz_inflate()->inflate;
 *! while(string s=stdin->read(8192))
 *!   write(inflate(s));
 *!
 *! @seealso
 *! @[Gz.deflate->deflate()]
 */
static void gz_inflate(INT32 args)
{
  struct pike_string *data;
  int fail;
  struct zipper *this=THIS;
  dynamic_buffer buf;
  ONERROR err;

  if(!THIS->gz.state)
    Pike_error("gz_inflate not initialized or destructed\n");

  if(args<1)
    Pike_error("Too few arguments to gz_inflate->inflate()\n");

  if(sp[-args].type != T_STRING)
    Pike_error("Bad argument 1 to gz_inflate->inflate()\n");

  data=sp[-args].u.string;

  this->gz.next_in=(Bytef *)data->str;
  this->gz.avail_in = DO_NOT_WARN((unsigned INT32)(data->len));

  initialize_buf(&buf);

  SET_ONERROR(err,toss_buffer,&buf);
  fail=do_inflate(&buf,this,Z_SYNC_FLUSH);
  UNSET_ONERROR(err);

  if(fail != Z_OK && fail != Z_STREAM_END)
  {
    toss_buffer(&buf);
    if(THIS->gz.msg)
      Pike_error("Error in gz_inflate->inflate(): %s\n",THIS->gz.msg);
    else
      Pike_error("Error in gz_inflate->inflate(): %d\n",fail);
  }

  pop_n_elems(args);

  push_string(low_free_buf(&buf));
  if(fail != Z_STREAM_END && fail!=Z_OK && !sp[-1].u.string->len)
  {
    pop_stack();
    push_int(0);
  }
}

static void init_gz_inflate(struct object *o)
{
  mt_init(& THIS->lock);
  MEMSET(& THIS->gz, 0, sizeof(THIS->gz));
  THIS->gz.zalloc=Z_NULL;
  THIS->gz.zfree=Z_NULL;
  THIS->gz.opaque=(void *)THIS;
  inflateInit(&THIS->gz);
  inflateEnd(&THIS->gz);
}

static void exit_gz_inflate(struct object *o)
{
/*   mt_lock(& THIS->lock); */
  inflateEnd(& THIS->gz);
/*   mt_unlock(& THIS->lock); */
  mt_destroy( & THIS->lock );
}

/*! @endclass
 */

/*! @decl int crc32(string data, void|int start_value)
 *!
 *!   This function calculates the standard ISO3309 Cyclic Redundancy Check.
 */
static void gz_crc32(INT32 args)
{
   unsigned INT32 crc;
   if (!args ||
       sp[-args].type!=T_STRING)
      Pike_error("Gz.crc32: illegal or missing argument 1 (expected string)\n");

   if (args>1) {
      if (sp[1-args].type!=T_INT)
	 Pike_error("Gz.crc32: illegal argument 2 (expected integer)\n");
      else
	 crc=(unsigned INT32)sp[1-args].u.integer;
   } else
      crc=0;
	 
   crc=crc32(crc,
	     (unsigned char*)sp[-args].u.string->str,
	     DO_NOT_WARN((unsigned INT32)(sp[-args].u.string->len)));

   pop_n_elems(args);
   push_int((INT32)crc);
}

/*! @class _file
 *! Low-level implementation of read/write support for GZip files
 */

/*! @decl int open(string|int file, void|string mode)
 *!   Opens a file for I/O.
 *! @param file
 *!   The filename or an open filedescriptor for the GZip file to use.
 *! @param mode
 *!   Mode for the fileoperations. Defaults to read only.
 *!
 *! @note
 *!   If the object already has been opened, it will first be closed.
 */
void gz_file_open(INT32 args)
{
  char *mode = "rb";

  if (THIS->gzfile!=NULL) {
    gzclose(THIS->gzfile);
    THIS->gzfile = NULL;
  }

  if (args<1 || args>2) {
    Pike_error("Bad number of arguments to file->open()\n"
	       "Got %d, expected 1 or 2.\n", args);
  }

  if (sp[-args].type != PIKE_T_STRING &&
      sp[-args].type != PIKE_T_INT) {
    Pike_error("Bad parameter 1 to file->open()\n");
  }

  if (args == 2 && sp[1-args].type != PIKE_T_STRING) {
    Pike_error("Bad parameter 2 to file->open()\n");
  } else if (args == 2) {
    mode = STR0(sp[1-args].u.string);
  }

  if (sp[-args].type == PIKE_T_INT) {
    /* FIXME: This is not likely to work on NT. */
    THIS->gzfile = gzdopen(sp[-args].u.integer, mode);
  } else {
    THIS->gzfile = gzopen(STR0(sp[-args].u.string), mode);
  }

  pop_n_elems(args);
  push_int(THIS->gzfile != NULL);
}

/*! @decl void create(void|string gzFile, void|string mode)
 *!   Opens a gzip file for reading.
 */
void gz_file_create(INT32 args)
{
  THIS->gzfile = NULL;
  if (args) {
    gz_file_open(args);
    if (sp[-1].u.integer == 0) {
      Pike_error("Failed to open file.\n");
    }
    pop_stack();
  }
}

/*! @decl int close()
 *! closes the file
 *! @returns 
 *!  1 if successful
 */
void gz_file_close(INT32 args)
{
  if (args!=0) {
    Pike_error("Bad number of arguments to file->close()\n"
	       "Got %d, expected 0.\n", args);
  }

  if (THIS->gzfile!=NULL) {
    gzclose(THIS->gzfile);
    THIS->gzfile = NULL;
  }

  push_int(1);
}

/*! @decl int|string read(int len)
 *! Reads len (uncompressed) bytes from the file.
 *! If read is unsuccessful, 0 is returned.
 */
void gz_file_read(INT32 args)
{
  struct pike_string *buf;
  int len;
  int res;

  if (args!=1) {
    Pike_error("Bad number of arguments to gz_file->read()\n"
	       "Got %d, expected 1.\n", args);
  }

  if (sp[-args].type != PIKE_T_INT) {
    Pike_error("Bad argument 1 to gz_file->read()\n");
  }

  if (THIS->gzfile == NULL) {
    Pike_error("File not open!\n");
  }

  len = sp[-args].u.integer;

  buf = begin_shared_string(len);

  pop_n_elems(args);

  res = gzread(THIS->gzfile, STR0(buf), len);

  /* Check to make sure read went well */
  if (res<0) {
    push_int(0);
    free_string(end_shared_string(buf));
    return;
  }

  /* Make sure the returned string is the same length as
   * the data read.
   */
  push_string(end_and_resize_shared_string(buf, res));
}

/*! @decl int write(string data)
 *! Writes the data to the file.
 *! @returns 
 *!  the number of bytes written to the file.
 */
void gz_file_write(INT32 args)
{
  int res = 0;

  if (args!=1) {
    Pike_error("Bad number of arguments to gz_file->write()\n"
	       "Got %d, expected 1.\n", args);
  }

  if (sp[-args].type != PIKE_T_STRING) {
    Pike_error("Bad argument 1 to gz_file->write()\n");
  }

  if (THIS->gzfile == NULL) {
    Pike_error("File not open!\n");
  }

  res = gzwrite(THIS->gzfile,
		sp[-args].u.string->str,
		(unsigned INT32)sp[-args].u.string->len);

  pop_n_elems(args);
  push_int(res);
}

#ifdef HAVE_GZSEEK
/*! @decl int seek(int pos, void|int type)
 *!   Seeks within the file.
 *! @param pos
 *!   Position relative to the searchtype.
 *! @param type
 *!   SEEK_SET = set current position in file to pos
 *!   SEEK_CUR = new position is current+pos
 *!   SEEK_END is not supported.
 *! @returns 
 *!   New position or negative number if seek failed.
 *!
 *! @note
 *!   Not supported on all operating systems.
 */
void gz_file_seek(INT32 args)
{
  int res, newpos;
  int type = SEEK_SET;

  if (args>2) {
    Pike_error("Bad number of arguments to file->seek()\n"
	       "Got %d, expected 1 or 2.\n", args);
  }

  if (sp[-args].type != PIKE_T_INT) {
    Pike_error("Bad argument 1 to file->seek()\n");
  }

  if (args == 2 && sp[1-args].type != PIKE_T_INT) {
    Pike_error("Bad argument 2 to file->seek()\n");
  }
  else if (args == 2) {
    type = sp[1-args].u.integer;
  }

  if (THIS->gzfile == NULL) {
    Pike_error("File not open!\n");
  }

  newpos = sp[-args].u.integer;

  pop_n_elems(args);

  res = gzseek(THIS->gzfile, newpos, type);

  push_int(res);
}
#endif /* HAVE_GZSEEK */

#ifdef HAVE_GZTELL
/*! @decl int tell()
 *! @returns 
 *!  the current position within the file.
 *!
 *! @note
 *!   Not supported on all operating systems.
 */
void gz_file_tell(INT32 args)
{
  if (args!=0) {
    Pike_error("Bad number of arguments to file->tell()\n"
	       "Got %d, expected 0.\n", args);
  }

  if (THIS->gzfile == NULL) {
    Pike_error("File not open!\n");
  }

  push_int(gztell(THIS->gzfile));

}
#endif /* HAVE_GZTELL */

#ifdef HAVE_GZEOF
/*! @decl int(0..1) eof()
 *! @returns 
 *!  1 if EOF has been reached.
 *!
 *! @note
 *!   Not supported on all operating systems.
 */
void gz_file_eof(INT32 args)
{
  if (args!=0) {
    Pike_error("Bad number of arguments to file->eof()\n"
	       "Got %d, expected 0.\n", args);
  }

  push_int(gzeof(THIS->gzfile));
}
#endif /* HAVE_GZEOF */

#ifdef HAVE_GZSETPARAMS
/*! @decl int setparams(int level, int strategy)
 *!   Sets the encoding level and strategy
 *! @param level
 *!   Level of the compression.
 *!   0 is the least compression, 9 is max. 8 is default.
 *! @param strategy
 *!   Set strategy for encoding to one of the following:
 *!   Z_DEFAULT_STRATEGY
 *!   Z_FILTERED
 *!   Z_HUFFMAN_ONLY
 *!
 *! @note
 *!   Not supported on all operating systems.
 */
void gz_file_setparams(INT32 args)
{
  int res;
  if (args!=2) {
    Pike_error("Bad number of arguments to file->setparams()\n"
	       "Got %d, expected 2.\n", args);
  }

  if (sp[-args].type != PIKE_T_INT ||
      sp[1-args].type != PIKE_T_INT) {
    Pike_error("Bad type in argument\n");
  }

  res = gzsetparams(THIS->gzfile,
		    sp[-args].u.integer,
		    sp[1-args].u.integer);

  pop_n_elems(args);
  push_int(res == Z_OK);
}
#endif /* HAVE_GZSETPARAMS */

static void init_gz_file(struct object *o)
{
  mt_init(& THIS->lock);
  THIS->gzfile = NULL;
}

static void exit_gz_file(struct object *o)
{
  if (THIS->gzfile != NULL)
    gzclose(THIS->gzfile);

  mt_destroy( & THIS->lock );
}

/*! @endclass
 */

/*! @endmodule
 */
#endif

PIKE_MODULE_EXIT {}

PIKE_MODULE_INIT
{
#ifdef HAVE_ZLIB_H
  start_new_program();
  ADD_STORAGE(struct zipper);
  
  /* function(int|void:void) */
  ADD_FUNCTION("create",gz_deflate_create,tFunc(tOr(tInt,tVoid),tVoid),0);
  /* function(string,int|void:string) */
  ADD_FUNCTION("deflate",gz_deflate,tFunc(tStr tOr(tInt,tVoid),tStr),0);

  add_integer_constant("NO_FLUSH",Z_NO_FLUSH,0);
  add_integer_constant("PARTIAL_FLUSH",Z_PARTIAL_FLUSH,0);
  add_integer_constant("SYNC_FLUSH",Z_SYNC_FLUSH,0);
  add_integer_constant("FINISH",Z_FINISH,0);

  set_init_callback(init_gz_deflate);
  set_exit_callback(exit_gz_deflate);

  end_class("deflate",0);

  start_new_program();
  ADD_STORAGE(struct zipper);
  
  /* function(int|void:void) */
  ADD_FUNCTION("create",gz_inflate_create,tFunc(tOr(tInt,tVoid),tVoid),0);
  /* function(string:string) */
  ADD_FUNCTION("inflate",gz_inflate,tFunc(tStr,tStr),0);

  add_integer_constant("NO_FLUSH",Z_NO_FLUSH,0);
  add_integer_constant("PARTIAL_FLUSH",Z_PARTIAL_FLUSH,0);
  add_integer_constant("SYNC_FLUSH",Z_SYNC_FLUSH,0);
  add_integer_constant("FINISH",Z_FINISH,0);

  set_init_callback(init_gz_inflate);
  set_exit_callback(exit_gz_inflate);

  end_class("inflate",0);

  start_new_program();
  ADD_STORAGE(struct zipper);

  ADD_FUNCTION("create", gz_file_create, tFunc(tOr(tVoid, tString) tOr(tVoid, tString), tVoid), 0);
  ADD_FUNCTION("open", gz_file_open, tFunc(tString tOr(tVoid, tString), tInt), 0);
  ADD_FUNCTION("close", gz_file_close, tFunc(tVoid, tInt), 0);
  ADD_FUNCTION("read", gz_file_read, tFunc(tInt,tOr(tString,tInt)), 0);
  ADD_FUNCTION("write", gz_file_write, tFunc(tString,tInt), 0);
#ifdef HAVE_GZSEEK
  ADD_FUNCTION("seek", gz_file_seek, tFunc(tInt tOr(tVoid,tInt), tInt), 0);
#endif /* HAVE_GZSEEK */
#ifdef HAVE_GZTELL
  ADD_FUNCTION("tell", gz_file_tell, tFunc(tVoid, tInt), 0);
#endif /* HAVE_GZTELL */
#ifdef HAVE_GZEOF
  ADD_FUNCTION("eof", gz_file_eof, tFunc(tVoid, tInt), 0);
#endif /* HAVE_GZEOF */
#ifdef HAVE_GZSETPARAMS
  ADD_FUNCTION("setparams", gz_file_setparams, tFunc(tInt tInt, tInt), 0);
#endif /* HAVE_GZSETPARAMS */

  add_integer_constant("SEEK_SET", SEEK_SET, 0);
  add_integer_constant("SEEK_CUR", SEEK_CUR, 0);
  add_integer_constant("Z_DEFAULT_STRATEGY", Z_DEFAULT_STRATEGY,0);
  add_integer_constant("Z_FILTERED", Z_FILTERED,0);
  add_integer_constant("Z_HUFFMAN_ONLY", Z_HUFFMAN_ONLY,0);
  set_init_callback(init_gz_file);
  set_exit_callback(exit_gz_file);
  end_class("_file", 0);

  add_integer_constant("NO_FLUSH",Z_NO_FLUSH,0);
  add_integer_constant("PARTIAL_FLUSH",Z_PARTIAL_FLUSH,0);
  add_integer_constant("SYNC_FLUSH",Z_SYNC_FLUSH,0);
  add_integer_constant("FINISH",Z_FINISH,0);

  /* function(string,void|int:int) */
  ADD_FUNCTION("crc32",gz_crc32,tFunc(tStr tOr(tVoid,tInt),tInt),
	       OPT_TRY_OPTIMIZE);

#endif
}

#if defined(HAVE___VTBL__9TYPE_INFO) || defined(HAVE___T_9__NOTHROW)
/* Super-special kluge for IRIX 6.3 */
#ifdef HAVE___VTBL__9TYPE_INFO
extern void __vtbl__9type_info(void);
#endif /* HAVE___VTBL__9TYPE_INFO */
#ifdef HAVE___T_9__NOTHROW
extern void __T_9__nothrow(void);
#endif /* HAVE___T_9__NOTHROW */
/* Don't even think of calling this one
 * Not static, so the compiler can't optimize it away totally.
 */
void zlibmod_strap_kluge(void)
{
#ifdef HAVE___VTBL__9TYPE_INFO
  __vtbl__9type_info();
#endif /* HAVE___VTBL__9TYPE_INFO */
#ifdef HAVE___T_9__NOTHROW
  __T_9__nothrow();
#endif /* HAVE___T_9__NOTHROW */
}
#endif /* HAVE___VTBL__9TYPE_INFO || HAVE___T_9__NOTHROW */
