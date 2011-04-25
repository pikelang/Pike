/*

  Audio.Codec object

  creator: Honza Petrous, hop@unibase.cz

  $Id$

 */

#pike __REAL_VERSION__

  mapping codec_map = ([
#if constant(_Ffmpeg.ffmpeg1)
	"MP3": ([ "encoder": 0,
		  "decoder": ({ "_Ffmpeg.ffmpeg", _Ffmpeg.CODEC_ID_MP3LAME })
		]),

	"WAV": ([ "encoder": ({ "_Ffmpeg.ffmpeg", _Ffmpeg.CODEC_ID_PCM_S16LE }),
		  "decoder": ({ "_Ffmpeg.ffmpeg", _Ffmpeg.CODEC_ID_PCM_S16LE })
		])
#endif
  	
  ]);

#if constant(_Ffmpeg.ffmpeg)
//! Decoder object.
//!
//! @note
//!  It needs @[_Ffmpeg.ffmpeg] module for real work.
class decoder {

  /*private*/ object codec;
  /*private*/ Audio.Format.ANY infile;

  //! Creates decoder object
  //!
  //! @param codecnum
  //!  Some of supported codec, like _Ffmpeg.CODEC_ID_*
  //!
  //! @param _codec
  //!  The low level object will be used for decoder.
  //!  By default @[_Ffmpeg.ffmpeg] object will be used.
  //!
  //! @note
  //!   Until additional library is implemented the second
  //!   parameter @[_codec] hasn't effect.
  //!
  //! @seealso
  //!  @[_Ffmpeg.ffmpeg], @[_Ffmpeg.CODEC_ID_MP2]
  static void create(string|void codecname, object|void _codec) {
    if(stringp(codecname))
      init(codecname);
  }

  private this_program init(string codecname) {

    mixed err;
    string cn = upper_case(codecname);

#if 0
    if(zero_type(codec_map[cn]) || !codec_map[cn]->decoder)
      error("Decoder codec '"+codecname+"' isn't supported.\n");

    err = catch(codec = _Ffmpeg.ffmpeg(codec_map[cn]->decoder[1], 0));
#else
    foreach(Array.filter(_Ffmpeg.list_codecs(), lambda(mapping m, string n) { return m->name == n; }, codecname), mapping fc)
      if(!fc->encoder_flg && fc->type == _Ffmpeg.CODEC_TYPE_AUDIO) {
        err = catch(codec = _Ffmpeg.ffmpeg(fc->id, 0));
	break;
      }
#endif

    if(err)
      error(err[0]);

    return this;
  }

  //! Set codec type from file
  //!
  //! It uses @[Audio.Format.ANY]'s method get_map()
  //! to determine which codec should be used.
  //!
  //! @param file
  //!   The object @[Audio.Format.ANY].
  this_program from_file(Audio.Format.ANY file) {
    string ctype;

    if(objectp(file) && file->get_map)
      ctype = file->get_map()->codec_type;

    if(stringp(ctype)) {
      infile = file;
      return init(ctype);
    }
    return 0;
  }


  //! Decodes audio data
  //!
  //! @param partial
  //!   Only one frame will be decoded per call.
  //!
  //! @returns
  //!   If successfull a mapping with decoded data and byte number
  //!   of used input data is returned, @tt{0@} otherwise.
  mapping|int decode(int|void partial) {
      return codec->decode(infile->get_data()); //, partial);
  }

  //! Returns decoder status
  //!
  mapping get_status() {
    return codec && codec->get_codec_status();
  }

  static mixed _sprintf(int|void type) {
    return type=='O' && sprintf("Audio.Codec(/* %O */)", codec);
  }

}
#endif
