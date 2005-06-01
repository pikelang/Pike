/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: acconfig.h,v 1.5 2005/06/01 14:08:53 per Exp $
*/

#ifndef SDL_CONFIG_H
#define SDL_CONFIG_H

@TOP@
@BOTTOM@

/* Define if your SDL has joystick support */
#undef HAVE_LIBSDL_JOYSTICK

#undef HAVE_SDL

#undef HAVE_SDL_SDL_H

# if defined(HAVE_LIBSDL_MIXER) && (defined(HAVE_SDL_MIXER_H) || defined(HAVE_SDL_SDL_MIXER_H))
#  define HAVE_SDL_MIXER
# endif
#endif
