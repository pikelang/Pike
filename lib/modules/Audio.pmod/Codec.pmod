/*

  Audio.Codec object

  creator: Honza Petrous, hop@unibase.cz

  $Id: Codec.pmod,v 1.4 2002/07/27 13:20:16 nilsson Exp $

 */

#if constant(_Ffmpeg.ffmpeg)
//! Decoder object.
class decoder {

  private object codec;

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
  void create(int codecnum, void|object _codec) {

    mixed err;

    if(_codec)
      error("Support for passing codec object is not implemented.\n");

    err = catch(codec = _Ffmpeg.ffmpeg(codecnum, 0));

    if(err)
      error(err[0]);

  }

  //! Decodes audio data
  //!
  //! @param data
  //!   Input buffer for decoding.
  //!
  //! @returns
  //!   If successfull a mapping with decoded data and byte number
  //!   of used input data is returned, @tt{0@} otherwise.
  mapping|int decode(string data) {
    return codec->decode(data);
  }

  //! Returns decoder status
  //!
  mapping get_status() {
    return codec->get_codec_status();
  }

  mixed _sprintf(int|void type) {
    if(type == 't')
      return "Audio.Codec";

    return sprintf("Audio.Codec /* %O */", codec);
  }

}
#endif
