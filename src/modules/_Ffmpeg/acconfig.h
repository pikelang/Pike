/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: acconfig.h,v 1.6 2002/12/06 11:03:18 mirar Exp $
*/

/*
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

/* Define if you have the avformat library (-lavformat).  */
#undef HAVE_LIBAVFORMAT

/* Define if you have the libmp3lame library (-lmp3lame).  */
#undef HAVE_LIBMP3LAME

/* Define if you have a working ffmpeg library */
#undef HAVE_WORKING_LIBFFMPEG

#endif
