/* this file only includes the header files, to use
 * sed in the Makefile to find the various codecs 
 * (that are enum'ed and not #defined and can't be detected otherwise) 
 */

#include "config.h"

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

