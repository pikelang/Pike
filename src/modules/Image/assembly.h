/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/* d == s is valid for all of these functions. */

void image_mult_buffer_mmx_x86asm(void *dest,void *source,int npixels_div_4,
                                  int rgbr, int gbrg, int brgb);

void image_mult_buffers_mmx_x86asm(void *dest, void *s1, void *s2, 
                                   int npixels_div_4);

void image_add_buffers_mmx_x86asm( void *d,void *s1, void *s2, 
                                   int npixels_div_8 );

void image_add_buffer_mmx_x86asm( void *d, void *s,
                                  int npixels_div_4,
                                  int rgbr, int gbrg, int brgb );

void image_sub_buffer_mmx_x86asm( void *d, void *s,
                                  int npixels_div_4,
                                  int rgbr, int gbrg, int brgb );

void image_clear_buffer_mmx_x86asm_eq( void *d, 
                                       int npixels_div_8,
                                       int colv );

void image_clear_buffer_mmx_x86asm_from( void *d, int npixels_div_8 );

void image_get_cpuid( int oper, void *cpuid1, void *cpuid2, void *cpuid3, void *d );



#define MCcol( A, B, C, D )   ((A<<24) | (B<<16) | (C<<8) | D)
#define RGB2ASMCOL( _X ) MCcol(_X.r,_X.b,_X.g,_X.r),MCcol(_X.g,_X.r,_X.b,_X.g),MCcol(_X.b,_X.g,_X.r,_X.b)
