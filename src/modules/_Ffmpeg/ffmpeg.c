/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: ffmpeg.c,v 1.15 2004/04/23 19:18:54 mast Exp $
*/

/*
 * Pike C glue module for FFMpeg library
 *
 * Creator: Honza Petrous <hop@unibase.cz>
 *
 */

#include "config.h"

#include "interpret.h"
#include "svalue.h"
#include "stralloc.h"
#include "array.h"
#include "object.h"
#include "mapping.h"
#include "pike_macros.h"
#include "module_support.h"
#include "builtin_functions.h"


#ifdef HAVE_WORKING_LIBFFMPEG

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ffmpeg includes typedef's these */
#ifdef FFMPEG_REDEFINES_UINT8
#ifdef INT64
#undef INT64
#endif
#ifdef INT32
#undef INT32
#endif
#ifdef INT16
#undef INT16
#endif
#ifdef INT8
#undef INT8
#endif
#ifdef UINT64
#undef UINT64
#endif
#ifdef UINT32
#undef UINT32
#endif
#ifdef UINT16
#undef UINT16
#endif
#ifdef UINT8
#undef UINT8
#endif
#endif

#ifdef HAVE_FFMPEG_AVCODEC_H
#include <ffmpeg/avcodec.h>
#else
#ifdef HAVE_LIBAVCODEC_AVCODEC_H
#include <libavcodec/avcodec.h>
#else
#ifdef HAVE_LIBFFMPEG_AVCODEC_H
#include <libffmpeg/avcodec.h>
#else
#ifdef HAVE_AVCODEC_H
#include <avcodec.h>
#endif
#endif
#endif
#endif

#ifdef AV_MEM
#define FF_ALLOC(x)	av_mallocz(x)
#define FF_FREE(x)	av_free(x)
#else
#define FF_ALLOC(x)	malloc(x)
#define FF_FREE(x)	free(x)
#endif

#ifndef HAVE_UINT8_T
#define uint8_t unsigned char
#endif

static struct program *ffmpeg_program;

typedef struct {
  AVCodec		*codec;
  AVCodecContext	 codec_context;
  AVCodecContext	*c;
  int			 encoder;
  uint8_t		*outbuf;
} ffmpeg_data;

#define THIS	((ffmpeg_data *)Pike_fp->current_storage)

int encoder_flg(AVCodec *codec) {
  int flg = -1;

  if( codec->encode )
    flg = 1;
  else if( codec->decode )
    flg = 0;
  return(flg);
}

/*! @module _Ffmpeg
 */

/*! @class ffmpeg
 *!
 *! Implements support of all codecs from a nice project Ffmpeg.
 *! You can find more info about it at @url{http://ffmpeg.sf.net/@}.
 */

/*! @decl void create(int codec_name, int encoder)
 *!
 *! Create decoder or encoder object.
 *!
 *! @param codec_name
 *!   Internal number of codec, eg. @[CODEC_ID_MP2].
 *!
 *! @param encoder
 *!   If @tt{true@}, encoder object will be created, decoder
 *!   object otherwise.
 *!
 */
static void f_create(INT32 args) {
  int codec_id = CODEC_ID_MP2;
  int rate, wide, chns;

  if(THIS->codec != NULL)
    Pike_error("Create already called!\n");

  switch(args) {
    case 5:
      if(Pike_sp[-1].type != T_INT)
	Pike_error("Invalid argument 5, expected int.\n");
      chns = (u_short)Pike_sp[-1].u.integer;
      Pike_sp--;
    case 4:
      if(Pike_sp[-1].type != T_INT)
	Pike_error("Invalid argument 4, expected int.\n");
      wide = (u_short)Pike_sp[-1].u.integer;
      Pike_sp--;
    case 3:
      if(Pike_sp[-1].type != T_INT)
	Pike_error("Invalid argument 3, expected int.\n");
      rate = (u_short)Pike_sp[-1].u.integer;
      Pike_sp--;

    case 2:
      if(Pike_sp[-1].type != T_INT)
	Pike_error("Invalid argument 2, expected int.\n");
      THIS->encoder = (u_short)Pike_sp[-1].u.integer;
      Pike_sp--;

    //case 1:
      if(Pike_sp[-1].type != T_INT)
	Pike_error("Invalid argument 1, expected int.\n");
      codec_id = (u_short)Pike_sp[-1].u.integer;
      Pike_sp--;
      if(THIS->encoder) {
        THIS->codec = avcodec_find_encoder(codec_id);
        if(!THIS->codec)
	  Pike_error("Codec for encoder not found.\n");
      } else {
        THIS->codec = avcodec_find_decoder(codec_id);
        if(!THIS->codec)
	  Pike_error("Codec for decoder not found.\n");
      }
      if(!THIS->codec)
	Pike_error("Codec not found.\n");

      THIS->c = &THIS->codec_context;
      memset(THIS->c, 0, sizeof(*THIS->c));
      if (avcodec_open(THIS->c, THIS->codec) < 0)
        Pike_error("Could not open codec.\n");

      if(THIS->outbuf != NULL)
        FF_FREE(THIS->outbuf);
      THIS->outbuf = FF_ALLOC(AVCODEC_MAX_AUDIO_FRAME_SIZE);
      /* FIXME: we really want so big buffer? */
      if(THIS->outbuf == NULL)
	Pike_error("Malloc of internal buffer failed.\n");
      break;

    default:
      Pike_error("Invalid number of arguments to create().\n");
  }

} /* create */


/*! @decl mapping|int get_codec_info()
 *!
 *! Returns mapping with info of used codec.
 *!
 *! @seealso
 *!   @[list_codecs()]
 */
static void f_codec_info(INT32 args) {

  pop_n_elems(args);
  if(THIS->codec != NULL) {
    push_text("name");		push_text( THIS->codec->name );
    push_text("type");		push_int( THIS->codec->type );
    push_text("id");		push_int( THIS->codec->id );
    push_text("encoder_flg");	push_int( encoder_flg(THIS->codec) );
    f_aggregate_mapping( 2*4 );
  } else
    push_int(0);
}

/*! @decl int set_codec_param(string name, mixed value)
 *!
 *! Sets one codec parameter
 *!
 *! @param name
 *!   The name of parameter. One of @tt{"sample_rate"@}, @tt{"bit_rate"@},
 *!   @tt{"channels"@}.
 *!
 *! @returns
 *!   Returns 1 on success, 0 otherwise (parameter not known).
 *!
 *! @seealso
 *!   @[get_codec_params()]
 */
static void f_set_codec_param(INT32 args) {
  struct pike_string *pname;

  if(args != 2)
    Pike_error("Invalid number of arguments to set_codec_param().\n");
  if(Pike_sp[-args].type != T_STRING)
    Pike_error("Invalid argument 1, expected string.\n");
  pname = Pike_sp[-args].u.string;

  /* bit_rate */
  if(!strncmp(pname->str, "bit_rate", 8)) {
    if(Pike_sp[-1].type != T_INT)
      Pike_error("Invalid argument 2, expected integer.\n");
    // FIXME: test correct value of bit rate argument
    THIS->c->bit_rate = Pike_sp[-1].u.integer;
    pop_n_elems(args);
    push_int(1);
    return;
  }

  /* sample_rate */
  if(!strncmp(pname->str, "sample_rate", 11)) {
    if(Pike_sp[-1].type != T_INT)
      Pike_error("Invalid argument 2, expected integer.\n");
    // FIXME: test correct value of bit rate argument
    THIS->c->sample_rate = Pike_sp[-1].u.integer;
    pop_n_elems(args);
    push_int(1);
    return;
  }

  /* channels */
  if(!strncmp(pname->str, "channels", 8)) {
    if(Pike_sp[-1].type != T_INT)
      Pike_error("Invalid argument 2, expected integer.\n");
    // FIXME: test correct value of bit rate argument
    THIS->c->channels = (u_short)Pike_sp[-1].u.integer;
    pop_n_elems(args);
    push_int(1);
    return;
  }

  pop_n_elems(args);
  push_int(0);
}

/*! @decl mapping|int get_codec_status()
 *!
 *! Returns a mapping with the actual codec parameters.
 *!
 *! @seealso
 *!   @[set_codec_param()]
 */
static void f_get_codec_status(INT32 args) {
  int cnt = 0;

  pop_n_elems(args);
  if(THIS->codec == NULL || THIS->c == NULL)
    push_int(0);
  
  push_text("name");		push_text( THIS->codec->name );
  push_text("type");		push_int( THIS->codec->type );
  push_text("id");		push_int( THIS->codec->id );
  push_text("encoder_flg");	push_int( encoder_flg(THIS->codec) );
  push_text("flags");		push_int( THIS->c->flags );
  cnt = 5;

  if(THIS->codec->type == CODEC_TYPE_AUDIO) {
    /* audio only */
    push_text("sample_rate");	push_int( THIS->c->sample_rate );
    push_text("channels");	push_int( THIS->c->channels );
    cnt += 2;
  }

  if(THIS->codec->type == CODEC_TYPE_VIDEO) {
    /* video only */
    push_text("frame_rate");	push_int( THIS->c->frame_rate );
    push_text("width");		push_int( THIS->c->width );
    cnt += 2;
  }

  f_aggregate_mapping(2 * cnt);
}

/*! @decl mapping|int decode(string data)
 *!
 *! Returns a mapping with the new decoded frame and lenght of
 *! @[data] which was used for decoding.
 *!
 *! @decl int decode(string data, function shuffler)
 *!
 *! Decode all @[data] buffer and pass result to @[shuffler].
 *! Returns @tt{1@} on success, @tt{0@} otherwise.
 *!
 *! @note
 *!   Shuffler variant isn't implemented, yet.
 *!
 *! @note
 *!   Usable only in decoder.
 *!
 *! @seealso
 *!   @[create()]
 */
static void f_decode(INT32 args) {

  struct pike_string *idata;
  int len, samples_size;
  struct svalue feeder;

  if(Pike_sp[-args].type != T_STRING)
    Pike_error("Invalid argument 1, expected string.\n");

  idata = Pike_sp[-args].u.string;

  if(THIS->c == NULL)
    Pike_error("Codec wasn't inited.\n");

  if(THIS->outbuf == NULL)
    Pike_error("Low memory? Decoder buffer doesn't exist.\n");

  if(!idata->len)
    Pike_error("Encoded data is empty.\n");

  if (idata->size_shift)
    Pike_error("Encoded data is wide.\n");

  if(args > 1) {
    /* FIXME: shuffler part not implemented, yet */
#if NOT_IMPLEMENTED_YET
    if(Pike_sp[-1].type != T_FUNCTION)
      Pike_error("Invalid argument 2, expected function.\n");
    feeder = Pike_sp[1-args].u.svalue;
    apply_svalue(&feeder, 0); /* we want more data */
#endif
    pop_n_elems(args);
    push_int(0);
  }

  /* one pass decoding */
  len = avcodec_decode_audio(THIS->c, (short *)THIS->outbuf, &samples_size,
		  	     STR0(idata), idata->len);
  if(len < 0)
    Pike_error("Error while decoding.\n");
  if(samples_size > 0) {
    /* frame was decoded */
    pop_n_elems(args);
    push_text("data");
    push_string(make_shared_binary_string((char *)THIS->outbuf, samples_size));
    push_text("decoded");
    push_int(len);
    f_aggregate_mapping( 2*2 );
    return;
  }
  pop_n_elems(args);
  push_int(len);

}

/* @decl mapping|int encode(string data)
 *
 * Returns mapping with new encoded frame and lenght of
 * @[data] which was used for encoding.
 *
 * @decl int encode(string data, function shuffler);
 * Returns @tt{1@} on success, @tt{0@} otherwise.
 *
 * @note
 *   Usable only in encoder
 *
 * @seealso
 *   @[create()]
 */
static void f_encode(INT32 args) {

  pop_n_elems(args);
  push_int(0);
}


/*! @decl array(mapping) list_codecs()
 *!
 *! Gets all supported codecs.
 *!
 *! @returns
 *!   Array of mapping with codec features.
 */
static void f_list_codecs(INT32 args) {
  int cnt = 0;
  AVCodec *codec;

  avcodec_init();
  avcodec_register_all();

  pop_n_elems(args);
  if(first_avcodec == NULL) {
    push_int(0);
    return;
  }

  codec = first_avcodec;
  while(codec != NULL) {
    cnt++;
    push_text("name");		push_text( codec->name );
    push_text("type");		push_int( codec->type );
    push_text("id");		push_int( codec->id );
    push_text("encoder_flg");	push_int( encoder_flg(codec) );
    codec = codec->next;
    f_aggregate_mapping( 2*4 );
  }
  push_array(aggregate_array( cnt ));
}

/*! @endclass
 */

/*! @decl constant CODEC_ID_NONE
 *! @decl constant CODEC_ID_AC3
 *! @decl constant CODEC_ID_ADPCM_IMA_QT
 *! @decl constant CODEC_ID_ADPCM_IMA_WAV
 *! @decl constant CODEC_ID_ADPCM_MS
 *! @decl constant CODEC_ID_H263
 *! @decl constant CODEC_ID_H263I
 *! @decl constant CODEC_ID_H263P
 *! @decl constant CODEC_ID_MJPEG
 *! @decl constant CODEC_ID_MPEG1VIDEO
 *! @decl constant CODEC_ID_MPEG4
 *! @decl constant CODEC_ID_MP2
 *! @decl constant CODEC_ID_MP3LAME
 *! @decl constant CODEC_ID_MSMPEG4V1
 *! @decl constant CODEC_ID_MSMPEG4V2
 *! @decl constant CODEC_ID_MSMPEG4V3
 *! @decl constant CODEC_ID_PCM_ALAW
 *! @decl constant CODEC_ID_PCM_MULAW
 *! @decl constant CODEC_ID_PCM_S16BE
 *! @decl constant CODEC_ID_PCM_S16LE
 *! @decl constant CODEC_ID_PCM_S8
 *! @decl constant CODEC_ID_PCM_U16BE
 *! @decl constant CODEC_ID_PCM_U16LE
 *! @decl constant CODEC_ID_PCM_U8
 *! @decl constant CODEC_ID_RAWVIDEO
 *! @decl constant CODEC_ID_RV10
 *! @decl constant CODEC_ID_SVQ1
 *! @decl constant CODEC_ID_VORBIS
 *! @decl constant CODEC_ID_WMV1
 *! @decl constant CODEC_ID_WMV2
 *!
 *!  Various audio and video codecs.
 *!
 *! @note
 *!   The list of supported codecs depends on Ffmpeg library.
 *!
 *! @decl constant CODEC_TYPE_AUDIO
 *! @decl constant CODEC_TYPE_VIDEO
 *!
 *!  Type of codec.
 */

/*! @endmodule
 */

static void init_ffmpeg_data(struct object *obj) {


  THIS->codec = NULL;
  THIS->outbuf = NULL;
  THIS->encoder = 0;

  avcodec_init();
  avcodec_register_all(); // FIXME: register only "interesting" codec ?
}

static void exit_ffmpeg_data(struct object *obj) {

  if(THIS->codec != NULL)
    avcodec_close(THIS->c);
  if(THIS->outbuf != NULL)
    FF_FREE(THIS->outbuf);
}

/*
 * ---------------------
 *    Pike module API
 * ---------------------
 */

PIKE_MODULE_INIT {

  add_function("list_codecs", f_list_codecs, "function(:array|int)", 0);

#ifdef LIBAVCODEC_VERSION
  add_string_constant("lib_version", LIBAVCODEC_VERSION, 0);
#else
#ifdef FFMPEG_VERSION
  add_string_constant("lib_version", FFMPEG_VERSION, 0);
#endif
#endif
#ifdef LIBAVCODEC_BUILD_STR
  add_string_constant("lib_build", LIBAVCODEC_BUILD_STR, 0);
#endif

  /*
   * Internal codec constant name
   *
   */
#ifndef CODEC_ID_NONE
/* the codecs are enum'ed */
#include "codecs_auto.h"
#else
  add_integer_constant("CODEC_ID_NONE", CODEC_ID_NONE, 0);
  add_integer_constant("CODEC_ID_MPEG1VIDEO", CODEC_ID_MPEG1VIDEO, 0);
  add_integer_constant("CODEC_ID_H263", CODEC_ID_H263, 0);
  add_integer_constant("CODEC_ID_RV10", CODEC_ID_RV10, 0);
  add_integer_constant("CODEC_ID_MP2", CODEC_ID_MP2, 0);
#ifdef CODEC_ID_MP3LAME
  add_integer_constant("CODEC_ID_MP3LAME", CODEC_ID_MP3LAME, 0);
#endif
  add_integer_constant("CODEC_ID_AC3", CODEC_ID_AC3, 0);
  add_integer_constant("CODEC_ID_MJPEG", CODEC_ID_MJPEG, 0);
  add_integer_constant("CODEC_ID_MPEG4", CODEC_ID_MPEG4, 0);
  add_integer_constant("CODEC_ID_RAWVIDEO", CODEC_ID_RAWVIDEO, 0);
#ifdef CODEC_ID_MSMPEG4V1
  add_integer_constant("CODEC_ID_MSMPEG4V1", CODEC_ID_MSMPEG4V1, 0);
#endif
#ifdef CODEC_ID_MSMPEG4V2
  add_integer_constant("CODEC_ID_MSMPEG4V2", CODEC_ID_MSMPEG4V2, 0);
#endif
#ifdef CODEC_ID_MSMPEG4V3
  add_integer_constant("CODEC_ID_MSMPEG4V3", CODEC_ID_MSMPEG4V3, 0);
#endif
#ifdef CODEC_ID_WMV1
  add_integer_constant("CODEC_ID_WMV1", CODEC_ID_WMV1, 0);
#endif
  add_integer_constant("CODEC_ID_H263I", CODEC_ID_H263I, 0);
  add_integer_constant("CODEC_ID_H263P", CODEC_ID_H263P, 0);
#ifdef CODEC_ID_WMV2
  add_integer_constant("CODEC_ID_WMV2", CODEC_ID_WMV2, 0);
#endif
#ifdef CODEC_ID_SVQ1
  add_integer_constant("CODEC_ID_SVQ1", CODEC_ID_SVQ1, 0);
#endif
#ifdef CODEC_ID_PCM_S16LE
  add_integer_constant("CODEC_ID_PCM_S16LE", CODEC_ID_PCM_S16LE, 0);
#endif
#ifdef CODEC_ID_PCM_S16BE
  add_integer_constant("CODEC_ID_PCM_S16BE", CODEC_ID_PCM_S16BE, 0);
#endif
#ifdef CODEC_ID_PCM_U16LE
  add_integer_constant("CODEC_ID_PCM_U16LE", CODEC_ID_PCM_U16LE, 0);
#endif
#ifdef CODEC_ID_PCM_U16BE
  add_integer_constant("CODEC_ID_PCM_U16BE", CODEC_ID_PCM_U16BE, 0);
#endif
#ifdef CODEC_ID_PCM_S8
  add_integer_constant("CODEC_ID_PCM_S8", CODEC_ID_PCM_S8, 0);
#endif
#ifdef CODEC_ID_PCM_U8
  add_integer_constant("CODEC_ID_PCM_U8", CODEC_ID_PCM_U8, 0);
#endif
#ifdef CODEC_ID_PCM_MULAW
  add_integer_constant("CODEC_ID_PCM_MULAW", CODEC_ID_PCM_MULAW, 0);
#endif
#ifdef CODEC_ID_PCM_ALAW
  add_integer_constant("CODEC_ID_PCM_ALAW", CODEC_ID_PCM_ALAW, 0);
#endif
#ifdef CODEC_ID_ADPCM_MS
  add_integer_constant("CODEC_ID_ADPCM_MS", CODEC_ID_ADPCM_MS, 0);
#endif
#ifdef CODEC_ID_ADPCM_IMA_WAV
  add_integer_constant("CODEC_ID_ADPCM_IMA_WAV", CODEC_ID_ADPCM_IMA_WAV, 0);
#endif
#ifdef CODEC_ID_ADPCM_IMA_QT
  add_integer_constant("CODEC_ID_ADPCM_IMA_QT", CODEC_ID_ADPCM_IMA_QT, 0);
#endif
#ifdef CODEC_ID_VORBIS
  add_integer_constant("CODEC_ID_VORBIS", CODEC_ID_VORBIS, 0);
#endif

#endif

  /*
   * Internal type of codec.
   *
   */
  add_integer_constant("CODEC_TYPE_AUDIO", CODEC_TYPE_AUDIO, 0);
  add_integer_constant("CODEC_TYPE_VIDEO", CODEC_TYPE_VIDEO, 0);
#ifndef CODEC_ID_NONE
/* it's enum'ed */
  add_integer_constant("CODEC_TYPE_UNKNOWN", CODEC_TYPE_UNKNOWN, 0);
#else
#ifdef CODEC_TYPE_UNKNOWN
  add_integer_constant("CODEC_TYPE_UNKNOWN", CODEC_TYPE_UNKNOWN, 0);
#endif
#endif

  start_new_program();
  ADD_STORAGE(ffmpeg_data);

  add_function("create", f_create, "function(int, int, int|void, int|void, int|void:void)", 0);
  add_function("get_codec_info", f_codec_info, "function(:mapping)", 0);
  add_function("decode", f_decode, "function(string:mapping|int)", 0);
  add_function("encode", f_encode, "function(string:mapping|int)", 0);
  add_function("set_codec_param", f_set_codec_param, "function(string,mixed:int)", 0);
  add_function("get_codec_status", f_get_codec_status, "function(:mapping|int)", 0);

  set_init_callback(init_ffmpeg_data);
  set_exit_callback(exit_ffmpeg_data);
  ffmpeg_program = end_program();
  add_program_constant("ffmpeg", ffmpeg_program, 0);
} /* PIKE_MODULE_INIT */

PIKE_MODULE_EXIT {

  if(ffmpeg_program) {
    free_program(ffmpeg_program);
    ffmpeg_program = NULL;
  }
} /* PIKE_MODULE_EXIT */

#else

PIKE_MODULE_INIT {

}

PIKE_MODULE_EXIT {
}

#endif
