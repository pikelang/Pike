#pike __REAL_VERSION__

// $Id: AU.pmod,v 1.1 2002/05/29 21:59:11 hop Exp $

// AU file parser/formatter
//
// Author: Honza Petrous, hop@unibase.cz
//
// http://www.opengroup.org/public/pubs/external/auformat.html

//#define AUDIO_AU_DEBUG
#ifdef AUDIO_AU_DEBUG
#define DEBUG(X, Y ...) werror("Audio.AU: " + X, Y)
#else
#define DEBUG(X, Y ...)
#endif


//! A AU file parser.
class decode {

  /*private*/ Audio.vbuffer buffer;
  private int nochk;
  private mapping parms = ([]);

  void create(Stdio.File|string fd, int|void nocheck) {
    nochk = nocheck;
    buffer = Audio.vbuffer(fd);
    if(!nocheck)
      if(!read_headers(buffer))
        error("No AU file.\n");
  }

  string _sprintf(int|void ctype) {
    return buffer->fd ? 
      sprintf("Audio.AU(\"%O\",%O)", buffer->fd, nochk) :
      sprintf("Audio.AU(string(%d),%O)", sizeof(buffer->origbuf), nochk);
  }

  mapping get_map() {
    return ([
    	"type": "audio/basic",
	"description": "SUN AU format",
	"parameters": parms
    ]);
  }

  mapping(string:int) encodings =
  ([
     "mulaw_8":			 1, /* 8-bit ISDN u-law */
     "linear_8":		 2, /* 8-bit linear PCM */
     "linear_16":		 3, /* 16-bit linear PCM */
     "linear_24":		 4, /* 24-bit linear PCM */
     "linear_32":		 5, /* 32-bit linear PCM */
     "float":			 6, /* 32-bit IEEE floating point */
     "double":			 7, /* 64-bit IEEE floating point */
     "indirect":		 8, /* fragmented sampled data
     "nested":			 9, /* ? */
     "dsp_core":		10, /* DSP program */
     "dsp_data_8":		11, /* 8-bit fixed-point samples */
     "dsp_data_16":		12, /* 16-bit fixed-point samples */
     "dsp_data_24":		13, /* 24-bit fixed-point samples */
     "dsp_data_32":		14, /* 32-bit fixed-point samples */
     "15":			15, /* ? */
     "display":			16, /* non-audio display data */
     "mulaw_squelsch":		17, /* ? */
     "emphasized":		18, /* 16-bit linear with emphasis */
     "compressed":		19, /* 16-bit linear with compression */
     "compressed_empasized":	20, /* A combination of the two above */
     "dsp_commands":		21, /*  Music Kit DSP commands */
     "dsp_commands_samples":	22, /* ? */
     "adpcm_g721":		23, /* 4-bit CCITT g.721 ADPCM */
     "adpcm_g722":		24, /* CCITT g.722 ADPCM */
     "adpcm_g723_3":		25, /* CCITT g.723 3-bit ADPCM */
     "adpcm_g723_5":		26, /* CCITT g.723 5-bit ADPCM */
     "alaw_8":			27  /* 8-bit ISDN A-law */
  ]);

  //! Gets next sample from file
  //!
  //! @returns
  //!   Returns sample or -1 on error
  string get_sample() { 
    string sample;

    DEBUG("get_sample: getbytes = %O\n", sample);
    return sample;
  }

  private int read_headers(Audio.vbuffer fd) {
    string buff;

    fd->seek(0);

    // SUN audio magic
    if(fd->getbytes(4, 1) != ".snd")
      return 0;
    fd->getbytes(4); // header size (always 24)
    parms->length = fd->getbytes(4);
    parms->format = fd->getbytes(4);
    parms->sample_rate = fd->getbytes(4);
    parms->channels = fd->getbytes(4);
    fd->getbytes(4,1); // comment

    // data follow

    return 1;
  }

}
