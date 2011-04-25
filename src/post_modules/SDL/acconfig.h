/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#ifndef SDL_CONFIG_H
#define SDL_CONFIG_H

@TOP@
@BOTTOM@

#undef HAVE_SDL

# if defined(HAVE_LIBSDL_MIXER) && (defined(HAVE_SDL_MIXER_H) || defined(HAVE_SDL_SDL_MIXER_H))
#  define HAVE_SDL_MIXER
# endif
#endif
