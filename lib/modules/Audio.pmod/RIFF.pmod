#pike __REAL_VERSION__

// $Id: RIFF.pmod,v 1.1 2002/05/29 21:59:11 hop Exp $

// RIFF file parser/formatter
//
// Author: Honza Petrous, hop@unibase.cz
//
// http://www.goice.co.jp/member/mo/formats/wav.html

//#define AUDIO__RIFF_DEBUG
#ifdef AUDIO_RIFF_DEBUG
#define DEBUG(X, Y ...) werror("Audio.RIFF: " + X, Y)
#else
#define DEBUG(X, Y ...)
#endif


//! A RIFF file parser.
class decode {

  /*private*/ Audio.vbuffer buffer;
  private int nochk;
  private mapping parms = ([]);

  void create(Stdio.File|string fd, int|void nocheck) {
    nochk = nocheck;
    buffer = Audio.vbuffer(fd);
    if(!nocheck)
      if(!read_header_chunks(buffer))
        error("No RIFF file.\n");
  }

  string _sprintf(int|void ctype) {
    return buffer->fd ? 
      sprintf("Audio.Format.RIFF(\"%O\",%O)", buffer->fd, nochk) :
      sprintf("Audio.Format.RIFF(string(%d),%O)", sizeof(buffer->origbuf), nochk);
  }


  mapping get_map() {
    return ([
    	"type": "audio/x-wav",
	"description": "RIFF audio",
	"parameters": parms
    ]);
  }

  //! Gets next sample from file
  //!
  //! @returns
  //!   Returns sample or -1 on error
  string get_sample() { 
    string sample;

    sample = buffer->getbytes(parms->bytes_per_sample * parms->channels, 1);
    DEBUG("get_sample: getbytes = %O\n", sample);
    return sample;
  }

  private int find_chunk(string name, Audio.vbuffer fd) {
    if(fd->getbytes(4, 1) != name) {
      // FIXME: skip to next chunk and again
      return 0;
    }
    return 1;
  }

  mapping(string:int) encodings =
  ([
    "wave_pcm":       0x0001,
    "wave_adpcm":     0x0002,
    "wave_alaw":      0x0006,
    "wave_mulaw":     0x0007,
    "wave_oki_adpcm": 0x0010,
    "wave_digistd":   0x0015,
    "wave_digifix":   0x0016,
    "ibm_mulaw":      0x0101,
    "ibm_alaw":       0x0102,
    "ibm_adpcm":      0x0103
  ]);



  #define GET_INT(n)	ADT.struct(reverse(fd->getbytes(n, 1)))->get_uint(n)
  private int read_header_chunks(Audio.vbuffer fd) {
    string buff;

    fd->seek(0);

    // RIFF chunk
    if(fd->getbytes(4, 1) != "RIFF")
      return 0;
    fd->getbytes(4); // total lenght of package to follow
    if(fd->getbytes(4, 1) != "WAVE")
      return 0;

    // FORMAT chunk
    if(!find_chunk("fmt ", fd))
      return 0;
    fd->getbytes(4, 1); // lenght of format chunk, always '0x10'
    parms->format = GET_INT(2); // mostly '0x01' => PCM
    parms->channels = GET_INT(2);
    parms->sample_rate = GET_INT(4);
    parms->bit_rate = GET_INT(4)*8;
    parms->bytes_per_sample = GET_INT(2);
    parms->bits_per_sample = GET_INT(2);

    // DATA chunk
    if(!find_chunk("data", fd))
      return 0;
    parms->lenght = GET_INT(4);
    // data follow

    return 1;
  }

}
