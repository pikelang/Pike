/*
 * $Id: acconfig.h,v 1.3 2002/08/06 17:07:57 kiwi Exp $
 *
 * config file for Pike Ffmpeg glue module
 *
 * Honza Petrous, hop@unibase.cz
 */

#ifndef FFMPEG_CONFIG_H
#define FFMPEG_CONFIG_H

@TOP@
@BOTTOM@

/* Define if you have the avcodec library (-lavcodec).  */
#undef HAVE_LIBAVCODEC

/* Define if you have the ffmpeg library (-lffmpeg).  */
#undef HAVE_LIBFFMPEG

/* Define if you have a working ffmpeg library */
#undef HAVE_WORKING_LIBFFMPEG

#endif
