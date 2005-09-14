/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: freetype.c,v 1.27 2005/09/14 08:32:20 grubba Exp $
*/

#include "config.h"
#include "global.h"
#include "module.h"
#include "pike_error.h"

#ifdef HAVE_LIBFT2
#ifndef HAVE_FT_FT2BUILD
#include <freetype/freetype.h>
#else
#include <ft2build.h>
#include FT_FREETYPE_H
#endif
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "mapping.h"
#include "stralloc.h"
#include "builtin_functions.h"
#include "module_support.h"
#include "operators.h"
#include "../Image/image.h"
#endif /* HAVE_LIBFT2 */


#ifdef HAVE_LIBFT2

#if !defined(FT_ENC_TAG) && defined(FT_MAKE_TAG)
#define FT_ENC_TAG( value, _x1, _x2, _x3, _x4) \
	value = FT_MAKE_TAG( _x1, _x2, _x3, _x4 )
#endif /* !defined(FT_ENC_TAG) && defined(FT_MAKE_TAG) */

#define sp Pike_sp
#define fp Pike_fp

static FT_Library library;
static struct program *face_program;
#ifdef DYNAMIC_MODULE
static struct program *image_program;
#else
extern struct program *image_program;
#endif

struct face
{
  FT_Face face;
};

#define TFACE   ((struct face*)Pike_fp->current_storage)->face

#undef __FTERRORS_H__
#define FT_ERROR_START_LIST				\
  static const struct image_ft_error_lookup {		\
    const char *sym;					\
    FT_Error code;					\
    const char *msg;					\
  } image_ft_error_lookup[] = {
#define FT_ERRORDEF(SYM, VAL, MSG) { TOSTR(SYM), VAL, MSG },
#define FT_ERROR_END_LIST			\
  { NULL, 0, NULL },				\
    };
#ifdef FT_ERRORS_H
#include FT_ERRORS_H
#else
#include <freetype/fterrors.h>
#endif

static void image_ft_error(const char *msg, FT_Error errcode)
{
  const char *errmsg = NULL;
  if (errcode) {
    const struct image_ft_error_lookup *entry;
    for (entry = image_ft_error_lookup; entry->sym; entry++) {
      if (entry->code == errcode) {
	errmsg = entry->msg;
	break;
      }
    }
  }
  if (!errmsg) Pike_error("%s\n", msg);
  Pike_error("%s: %s\n", msg, errmsg);
}

/*! @module Image */

/*! @module FreeType
 *! Pike glue for the FreeType2 library, http://www.freetype.org/
 */

/*! @class Face
 *! A FreeType font face. We recommend using the more generic font handling
 *! interfaces in @[Image.Fonts] instead.
 */
static void image_ft_face_init( struct object *o )
{
  TFACE = NULL;
}

static void image_ft_face_free( struct object *o )
{
  FT_Done_Face( TFACE );
}

/*! @decl Image.Image write_char(int char)
 *! @fixme
 */
static void image_ft_face_write_char( INT32 args )
{
  FT_GlyphSlot  slot = TFACE->glyph; /* optimize. */
  int c, x, y;
  struct image *i;
  struct object *o;
  rgb_group *d;

  if( sp[-args].type != T_INT )
    Pike_error("Bad argument 1 to write_char\n" );

  c = sp[-args].u.integer;

  if( FT_Load_Char( TFACE, c, FT_LOAD_RENDER ) )
    Pike_error("The character %d is not available\n", c );

  push_int( slot->bitmap.width );
  push_int( slot->bitmap.rows );
  o = clone_object( image_program, 2 );
  i = ((struct image *)o->storage);
  d = i->img;
  if( slot->bitmap.pixel_mode == ft_pixel_mode_grays )
  {
    int p = slot->bitmap.pitch;
    int g = slot->bitmap.num_grays;
    unsigned char *s = slot->bitmap.buffer;
    if( s )
      for( y = 0; y<i->ysize; y++ )
        for( x = 0; x<i->xsize; x++ )
        {
          int pv = (s[ x + y*p ] * g) >> 8;
          d->r = pv;
          d->g = pv;
          d->b = pv;
          d++;
        }
  } else if( slot->bitmap.pixel_mode == ft_pixel_mode_mono ) {
    int p = slot->bitmap.pitch;
    char *s = slot->bitmap.buffer;
    p *= 8;
    if( s )
      for( y = 0; y<i->ysize; y++ )
        for( x = 0; x<i->xsize; x++ )
        {
          int pv =  ( ((s[(x + y*p) / 8]) << ((x + y*p) % 8)) & 128) ? 255 : 0;
          d->r = pv;
          d->g = pv;
          d->b = pv;
          d++;
        }
  } else
    Pike_error("Unhandled bitmap format received from renderer\n");

  push_text( "img" ); push_object( o );
  push_text( "x" )  ; push_int( slot->bitmap_left );
  push_text( "y" )  ; push_int( slot->bitmap_top );
  push_text( "advance" )  ; push_int( (slot->advance.x+62) >> 6 );
  push_text( "descender" ); push_int( TFACE->size->metrics.descender>>6 );
  push_text( "ascender" ); push_int( TFACE->size->metrics.ascender>>6 );
  push_text( "height" ); push_int( TFACE->size->metrics.height>>6 );
  
  f_aggregate_mapping( 14 );
}

/*! @decl int get_kerning(int l, int r)
 *! @fixme
 */
static void image_ft_face_get_kerning( INT32 args )
{
  INT_TYPE l, r;
  FT_Vector kern;
  get_all_args( "get_kerning", args, "%i%i", &l, &r );
  l = FT_Get_Char_Index( TFACE, l );
  r = FT_Get_Char_Index( TFACE, r );
  if( FT_Get_Kerning( TFACE, l, r, ft_kerning_default, &kern ) )
    kern.x = 0;
  pop_n_elems( args );
  push_int( kern.x );
}

/*! @decl void attach_file(string file)
 *! @fixme
 */
static void image_ft_face_attach_file( INT32 args )
{
  char *path;
  FT_Error errcode;
  get_all_args( "attach_file", args, "%s", &path );
  if ((errcode = FT_Attach_File( TFACE, path )))
    image_ft_error("Failed to attach file", errcode);
  pop_n_elems( args );
  push_int( 0 );
}

/*! @decl Face set_size(int width, int height)
 */
static void image_ft_face_set_size( INT32 args )
{
  int w, h;
  FT_Error errcode;
  if( (args != 2) || (sp[-args].type != sp[1-args].type)
      || (sp[-args].type != T_INT) )
    Pike_error("Illegal arguments to set_size\n");
  w = sp[-2].u.integer;
  h = sp[-1].u.integer;

  if((errcode = FT_Set_Pixel_Sizes( TFACE, w, h )))
    image_ft_error("Failed to set size", errcode);
  pop_n_elems( 2 );
  ref_push_object( fp->current_object );
}

/*! @decl array(string) list_encodings()
 */
static void image_ft_face_list_encodings( INT32 args )
{
  FT_Int enc_no;
  pop_n_elems( args );
  for(enc_no=0; enc_no<TFACE->num_charmaps; enc_no++) {
    FT_Encoding e = TFACE->charmaps[enc_no]->encoding;
    if(e == ft_encoding_none)
      push_int( 0 );
    else {
      push_constant_text( "%4c" );
      push_int( e );
      f_sprintf( 2 );
    }
  }
  f_aggregate( enc_no );
}

/*! @decl void select_encoding(string|int encoding)
 */
static void image_ft_face_select_encoding( INT32 args )
{
  FT_Encoding e = 0;
  FT_Error er;
  if( args != 1 || (sp[-args].type != PIKE_T_INT &&
		    sp[-args].type != PIKE_T_STRING) )
    Pike_error("Illegal arguments to select_encoding\n");
  if( sp[-args].type == PIKE_T_INT )
    e = (sp[-args].u.integer == 0? ft_encoding_none : sp[-args].u.integer);
  else if( sp[-args].u.string->len != 4 || sp[-args].u.string->size_shift )
    Pike_error("Invalid encoding name in select_encoding\n");
  else {
    p_wchar0 *s = STR0(sp[-args].u.string);
    FT_ENC_TAG( e, s[0], s[1], s[2], s[3] );
  }
  pop_n_elems( args );
  er = FT_Select_Charmap(TFACE, e);
  if( er )
    image_ft_error("Character encoding not available in this font", er);
}

/*! @decl mapping info()
 *! @returns
 *!   @mapping
 *!     @member string "family"
 *!       The font family, or the string "unknown"
 *!     @member string "style_name"
 *!       The name of the font style, or "unknown"
 *!     @member int "face_flags"
 *!     @member int "style_flags"
 *!       The sum of all face/style flags respectively.
 *!   @endmapping
 */
static void image_ft_face_info( INT32 args )
{
  pop_n_elems( args );
  push_text( "family" );
  if( TFACE->family_name )
    push_text( TFACE->family_name );
  else
    push_text( "unknown" );
  push_text( "style" );
  if( TFACE->style_name )
    push_text( TFACE->style_name );
  else
    push_text( "unknown" );
  push_text( "face_flags" );  push_int( TFACE->face_flags );
  push_text( "style_flags" );  push_int( TFACE->style_flags );
  f_aggregate_mapping( 8 );
}

/*! @decl void create(string font)
 *! @param font
 *!   The path of the font file to use
 */
static void image_ft_face_create( INT32 args )
{
  int er;
  FT_Encoding best_enc = ft_encoding_none;
  int enc_no, enc_score, best_enc_score = -2;
  if( !args || sp[-args].type != T_STRING )
    Pike_error("Illegal argument 1 to FreeType.Face. Expected string.\n");
  er = FT_New_Face( library, sp[-args].u.string->str, 0, &TFACE );
  if( er == FT_Err_Unknown_File_Format )
    Pike_error("Failed to parse the font file %S\n", sp[-args].u.string);
  else if( er )
    Pike_error("Failed to open the font file %S\n", sp[-args].u.string);
  for(enc_no=0; enc_no<TFACE->num_charmaps; enc_no++) {
    switch(TFACE->charmaps[enc_no]->encoding) {
    case ft_encoding_symbol: enc_score = -1; break;
    case ft_encoding_unicode: enc_score = 2; break;
#if HAVE_DECL_FT_ENCODING_LATIN_1
    case ft_encoding_latin_1: enc_score = 1; break;
#endif
#if 0
      /* FIXME: Should we rate any of these? */
    case ft_encoding_none:
    case ft_encoding_latin_2:
    case ft_encoding_sjis:
    case ft_encoding_gb2312:
    case ft_encoding_big5:
    case ft_encoding_wansung:
    case ft_encoding_johab:
    case ft_encoding_adobe_standard:
    case ft_encoding_adobe_expert:
    case ft_encoding_adobe_custom:
    case ft_encoding_apple_roman:
#endif
    default: enc_score = 0; break;
    }
    if(enc_score > best_enc_score) {
      best_enc_score = enc_score;
      best_enc = TFACE->charmaps[enc_no]->encoding;
    }
  }
  er = FT_Select_Charmap(TFACE, best_enc);
  if( er )
    Pike_error("Failed to set a character map for the font %S\n",
	       sp[-args].u.string);
  pop_n_elems( args );
  push_int( 0 );
}

/*! @endclass */

PIKE_MODULE_EXIT
{
  if( face_program )
    free_program( face_program );
}

/*! @decl constant FACE_FLAG_SCALABLE
 *! @decl constant FACE_FLAG_FIXED_WIDTH
 *! @decl constant FACE_FLAG_SFNT
 *! @decl constant FACE_FLAG_HORIZONTAL
 *! @decl constant FACE_FLAG_VERTICAL
 *! @decl constant FACE_FLAG_KERNING
 *! @decl constant FACE_FLAG_FAST_GLYPHS
 *! @decl constant FACE_FLAG_MULTIPLE_MASTERS
 *! @decl constant FACE_FLAG_GLYPH_NAMES
 *! @fixme
 */

/*! @decl constant STYLE_FLAG_ITALIC
 *! @decl constant STYLE_FLAG_BOLD
 *! @fixme
 */

/*! @endmodule */

/*! @endmodule */

PIKE_MODULE_INIT
{
  if( !FT_Init_FreeType( &library ) )
  {
#ifdef DYNAMIC_MODULE
    push_text("Image.Image");
    SAFE_APPLY_MASTER("resolv",1);
    if (sp[-1].type==T_PROGRAM)
      image_program=program_from_svalue(sp-1);
    pop_n_elems(1);
#endif /* DYNAMIC_MODULE */

    start_new_program( );
    ADD_STORAGE( struct face );

    ADD_FUNCTION("create",image_ft_face_create, tFunc(tStr,tVoid), 0 );
    ADD_FUNCTION("set_size",image_ft_face_set_size,tFunc(tInt tInt,tObj),0);
    ADD_FUNCTION("attach_file",image_ft_face_attach_file,tFunc(tString,tVoid),0);
    ADD_FUNCTION("list_encodings",image_ft_face_list_encodings,tFunc(tNone,tArr(tString)),0);
    ADD_FUNCTION("select_encoding",image_ft_face_select_encoding,tFunc(tOr(tString,tInt),tVoid),0);
    ADD_FUNCTION("info",image_ft_face_info,tFunc(tVoid,tMapping),0);
    ADD_FUNCTION("write_char",image_ft_face_write_char,tFunc(tInt,tObj),0);
    ADD_FUNCTION("get_kerning",image_ft_face_get_kerning,
                 tFunc(tInt tInt,tInt),0);
    
    set_exit_callback( image_ft_face_init );
    set_exit_callback( image_ft_face_free );

    face_program = end_program();
    add_program_constant("Face", face_program, 0 );
    add_integer_constant( "FACE_FLAG_SCALABLE", FT_FACE_FLAG_SCALABLE, 0 );
    add_integer_constant( "FACE_FLAG_FIXED_WIDTH", FT_FACE_FLAG_FIXED_WIDTH ,0); 
    add_integer_constant( "FACE_FLAG_SFNT", FT_FACE_FLAG_SFNT, 0 );
    add_integer_constant( "FACE_FLAG_HORIZONTAL", FT_FACE_FLAG_HORIZONTAL, 0 );
    add_integer_constant( "FACE_FLAG_VERTICAL", FT_FACE_FLAG_VERTICAL, 0 );
    add_integer_constant( "FACE_FLAG_KERNING", FT_FACE_FLAG_KERNING, 0 );
    add_integer_constant( "FACE_FLAG_FAST_GLYPHS", FT_FACE_FLAG_FAST_GLYPHS, 0 );
    add_integer_constant( "FACE_FLAG_MULTIPLE_MASTERS", FT_FACE_FLAG_MULTIPLE_MASTERS, 0 );
    add_integer_constant( "FACE_FLAG_GLYPH_NAMES", FT_FACE_FLAG_GLYPH_NAMES, 0 );


    add_integer_constant( "STYLE_FLAG_ITALIC", FT_STYLE_FLAG_ITALIC, 0 );
    add_integer_constant( "STYLE_FLAG_BOLD", FT_STYLE_FLAG_BOLD, 0 );

  }
}
#else
PIKE_MODULE_EXIT{}
PIKE_MODULE_INIT{}
#endif
