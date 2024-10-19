/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "module.h"
#include "config.h"

#if (defined(HAVE_SANE_SANE_H) || defined(HAVE_SANE_H)) && defined(HAVE_LIBSANE)
#ifdef HAVE_SANE_SANE_H
#include <sane/sane.h>
#elif defined(HAVE_SANE_H)
#include <sane.h>
#endif

#include "pike_macros.h"
#include "constants.h"
#include "interpret.h"
#include "threads.h"
#include "pike_error.h"
#include "backend.h"
#include "operators.h"
#include "pike_types.h"
#include "module_support.h"
#include "builtin_functions.h"

#include "../Image/image.h"


#define sp Pike_sp

/*! @module SANE
 *!
 *!  This module enables access to the SANE (Scanner Access Now Easy)
 *!  library from pike
 */

static int sane_is_inited;

struct scanner
{
  SANE_Handle h;
};

static void init_sane(void)
{
  if( sane_init( NULL, NULL ) )
    Pike_error( "Sane init failed.\n" );
  sane_is_inited =  1;
}

static void push_device( SANE_Device *d )
{
  push_static_text( "name" );    push_text( d->name );
  push_static_text( "vendor" );  push_text( d->vendor );
  push_static_text( "model" );   push_text( d->model );
  ref_push_string( literal_type_string );    push_text( d->type );
  f_aggregate_mapping( 8 );
}


/*! @decl array(mapping) list_scanners()
 *!
 *!  Returns an array with all available scanners.
 *!
 *! @example
 *!   Pike v0.7 release 120 running Hilfe v2.0 (Incremental Pike Frontend)
 *!   > SANE.list_scanners();
 *!     Result: ({
 *!            ([
 *!              "model":"Astra 1220S     ",
 *!              "name":"umax:/dev/scg1f",
 *!              "type":"flatbed scanner",
 *!              "vendor":"UMAX    "
 *!            ]),
 *!            ([
 *!              "model":"Astra 1220S     ",
 *!              "name":"net:lain.idonex.se:umax:/dev/scg1f",
 *!              "type":"flatbed scanner",
 *!              "vendor":"UMAX    "
 *!            ])
 *!        })
 */
static void f_list_scanners( INT32 UNUSED(args) )
{
  SANE_Device **devices;
  int i = 0;

  if( !sane_is_inited ) init_sane();
  switch( sane_get_devices( (void *)&devices, 0 ) )
  {
   case 0:
     while( devices[i] ) push_device( devices[i++] );
     f_aggregate( i );
     break;
   default:
     Pike_error("Failed to get device list\n");
  }
}

#define THIS ((struct scanner *)Pike_fp->current_storage)

static void push_option_descriptor( const SANE_Option_Descriptor *o )
{
  int i;
  struct svalue *osp = sp;
  push_static_text( "name" );
  if( o->name )
    push_text( o->name );
  else
    push_int( 0 );
  push_static_text( "title" );
  if( o->title )
    push_text( o->title );
  else
    push_int( 0 );
  push_static_text( "desc" );
  if( o->desc )
    push_text( o->desc );
  else
    push_int( 0 );
  ref_push_string( literal_type_string );
  switch( o->type )
  {
   case SANE_TYPE_BOOL:   push_static_text( "boolean" ); break;
   case SANE_TYPE_INT:    ref_push_string( literal_int_string );     break;
   case SANE_TYPE_FIXED:  ref_push_string( literal_float_string );   break;
   case SANE_TYPE_STRING: ref_push_string( literal_string_string );  break;
   case SANE_TYPE_BUTTON: push_static_text( "button" );  break;
   case SANE_TYPE_GROUP:  push_static_text( "group" );   break;
  }


  push_static_text( "unit" );
  switch( o->unit )
  {
   case SANE_UNIT_NONE:  	push_static_text( "none" ); 		break;
   case SANE_UNIT_PIXEL:  	push_static_text( "pixel" ); 		break;
   case SANE_UNIT_BIT:  	push_static_text( "bit" ); 		break;
   case SANE_UNIT_MM:  		push_static_text( "mm" ); 		break;
   case SANE_UNIT_DPI:          push_static_text( "dpi" ); 		break;
   case SANE_UNIT_PERCENT:      push_static_text( "percent" );   	break;
   case SANE_UNIT_MICROSECOND:  push_static_text( "microsecond" ); 	break;
  }

  push_static_text( "size" );   push_int( o->size );

  push_static_text( "cap" );
  {
    struct svalue *osp = sp;
    if( o->cap & SANE_CAP_SOFT_SELECT )  push_static_text( "soft_select" );
    if( o->cap & SANE_CAP_HARD_SELECT )  push_static_text( "hard_select" );
    if( o->cap & SANE_CAP_EMULATED )     push_static_text( "emulated" );
    if( o->cap & SANE_CAP_AUTOMATIC )    push_static_text( "automatic" );
    if( o->cap & SANE_CAP_INACTIVE )     push_static_text( "inactive" );
    if( o->cap & SANE_CAP_ADVANCED )     push_static_text( "advanced" );
    f_aggregate_multiset( sp - osp );
  }

  push_static_text( "constaint" );
  switch( o->constraint_type )
  {
   case SANE_CONSTRAINT_NONE:  push_int( 0 ); break;
   case SANE_CONSTRAINT_RANGE:
     ref_push_string( literal_type_string );  push_static_text( "range" );
     push_static_text( "min" );   push_int( o->constraint.range->min );
     push_static_text( "max" );   push_int( o->constraint.range->max );
     push_static_text( "quant" ); push_int( o->constraint.range->quant );
     f_aggregate_mapping( 8 );
     break;
   case SANE_CONSTRAINT_WORD_LIST:
     ref_push_string( literal_type_string );
     push_static_text( "list" );
     push_static_text( "list" );
     for( i = 0; i<o->constraint.word_list[0]; i++ )
       if( o->type == SANE_TYPE_FIXED )
         push_float( SANE_UNFIX(o->constraint.word_list[i+1]) );
       else
         push_int( o->constraint.word_list[i+1] );
     f_aggregate( o->constraint.word_list[0] );
     f_aggregate_mapping( 4 );
     break;
   case SANE_CONSTRAINT_STRING_LIST:
     ref_push_string( literal_type_string );
     push_static_text( "list" );
     push_static_text( "list" );
     for( i = 0; o->constraint.string_list[i]; i++ )
       push_text( o->constraint.string_list[i] );
     f_aggregate( i );
     f_aggregate_mapping( 4 );
     break;
  }
  f_aggregate_mapping( sp - osp );
}

/*! @class Scanner
 */

/*! @decl void create(string name)
 */
static void f_scanner_create( INT32 args )
{
  char *name;
  if(!sane_is_inited) init_sane();
  get_all_args( NULL, args, "%c", &name );

  if( sane_open( name, &THIS->h ) )
    Pike_error("Failed to open scanner \"%s\"\n", name );
}

/*! @decl array(mapping) list_options()
 *!
 *! This method returns an array where every element is a
 *! mapping, layed out as follows.
 *!
 *! @mapping
 *!   @member string "name"
 *!
 *!   @member string "title"
 *!
 *!   @member string "desc"
 *!
 *!   @member string "type"
 *!     @string
 *!       @value "boolean"
 *!       @value "int"
 *!       @value "float"
 *!       @value "string"
 *!       @value "button"
 *!       @value "group"
 *!     @endstring
 *!   @member string "unit"
 *!     @string
 *!       @value "none"
 *!       @value "pixel"
 *!       @value "bit"
 *!       @value "mm"
 *!       @value "dpi"
 *!       @value "percent"
 *!       @value "microsend"
 *!     @endstring
 *!   @member int "size"
 *!
 *!   @member multiset "cap"
 *!     @multiset
 *!       @index "soft_select"
 *!       @index "hard_select"
 *!       @index "emulate"
 *!       @index "automatic"
 *!       @index "inactive"
 *!       @index "advanced"
 *!     @endmultiset
 *!   @member int(0..0)|mapping "constraint"
 *!     Constraints can be of three different types; range, word list or string
 *!     list. Constraint contains 0 if there are no constraints.
 *!
 *!     @mapping range
 *!       @member string "type"
 *!         Contains the value "range".
 *!       @member int "max"
 *!       @member int "min"
 *!       @member int "quant"
 *!     @endmapping
 *!
 *!     @mapping "word list"
 *!       @member string "type"
 *!         Contains the value "list".
 *!       @member array(float|int) "list"
 *!     @endmapping
 *!
 *!     @mapping "string list"
 *!       @member string "type"
 *!         Contains the value "list".
 *!       @member array(string) "list"
 *!     @endmapping
 *! @endmapping
 */
static void f_scanner_list_options( INT32 args )
{
  int i, n;
  const SANE_Option_Descriptor *d;
  pop_n_elems( args );

  for( i = 1; (d = sane_get_option_descriptor( THIS->h, i) ); i++ )
    push_option_descriptor( d );
  f_aggregate( i-1 );
}

static int find_option( char *name, const SANE_Option_Descriptor **p )
{
  int i;
  const SANE_Option_Descriptor *d;
  for( i = 1; (d = sane_get_option_descriptor( THIS->h, i ) ); i++ )
    if(d->name && !strcmp( d->name, name ) )
    {
      *p = d;
      return i;
    }
  Pike_error("No such option: %s\n", name );
}


/*! @decl void set_option( string name, mixed new_value )
 *! @decl void set_option( string name )
 *!    If no value is specified, the option is set to it's default value
 */
static void f_scanner_set_option( INT32 args )
{
  char *name;
  int no;
  INT_TYPE int_value;
  FLOAT_TYPE float_value;
  SANE_Int tmp;
  const SANE_Option_Descriptor *d;
  get_all_args( NULL, args, "%c", &name );

  no = find_option( name, &d );
  if( args > 1 )
  {
    switch( d->type )
    {
     case SANE_TYPE_BOOL:
     case SANE_TYPE_INT:
     case SANE_TYPE_BUTTON:
       sp++;get_all_args( NULL, args, "%I", &int_value );sp--;
       sane_control_option( THIS->h, no, SANE_ACTION_SET_VALUE,
                            &int_value, &tmp );
       break;
     case SANE_TYPE_FIXED:
       sp++;get_all_args( NULL, args, "%F", &float_value );sp--;
       int_value = SANE_FIX(((double)float_value));
       sane_control_option( THIS->h, no, SANE_ACTION_SET_VALUE,
                            &int_value, &tmp );
       break;
     case SANE_TYPE_STRING:
       sp++;get_all_args( NULL, args, "%c", &name );sp--;
       sane_control_option( THIS->h, no, SANE_ACTION_SET_VALUE,
                            &name, &tmp );
     case SANE_TYPE_GROUP:
       break;
    }
  } else {
    int_value = 1;
    sane_control_option( THIS->h, no, SANE_ACTION_SET_AUTO, &int_value, &tmp );
  }
}


/*! @decl mixed get_option( string name )
 */
static void f_scanner_get_option( INT32 args )
{
  char *name;
  int no;
  SANE_Int int_value;
  float f;
  SANE_Int tmp;
  const SANE_Option_Descriptor *d;
  get_all_args( NULL, args, "%c", &name );

  no = find_option( name, &d );

  switch( d->type )
  {
   case SANE_TYPE_BOOL:
   case SANE_TYPE_INT:
   case SANE_TYPE_BUTTON:
     sane_control_option( THIS->h, no, SANE_ACTION_GET_VALUE,
                          &int_value, &tmp );
     pop_n_elems( args );
     push_int( int_value );
     return;
   case SANE_TYPE_FIXED:
     sane_control_option( THIS->h, no, SANE_ACTION_GET_VALUE,
                          &int_value, &tmp );
     pop_n_elems( args );
     push_float( SANE_UNFIX( int_value ) );
     break;
   case SANE_TYPE_STRING:
     sane_control_option( THIS->h, no, SANE_ACTION_GET_VALUE,
                          &name, &tmp );
     pop_n_elems( args );
     push_text( name );
   case SANE_TYPE_GROUP:
     break;
  }
}

/*! @decl mapping(string:int) get_parameters()
 *!
 *! @returns
 *!   @mapping
 *!  	@member int "format"
 *!  	@member int "last_frame"
 *!  	@member int "lines"
 *!  	@member int "depth"
 *!  	@member int "pixels_per_line"
 *!  	@member int "bytes_per_line"
 *!   @endmapping
 */
static void f_scanner_get_parameters( INT32 args )
{
  SANE_Parameters p;
  pop_n_elems( args );
  sane_get_parameters( THIS->h, &p );
  push_static_text( "format" );          push_int( p.format );
  push_static_text( "last_frame" );      push_int( p.last_frame );
  push_static_text( "lines" );           push_int( p.lines );
  push_static_text( "depth" );           push_int( p.depth );
  push_static_text( "pixels_per_line" ); push_int( p.pixels_per_line );
  push_static_text( "bytes_per_line" );  push_int( p.bytes_per_line );
  f_aggregate_mapping( 12 );
}


static struct program *image_program;

static void get_grey_frame( SANE_Handle h, SANE_Parameters *p,
			    SANE_Byte *data )
{
  SANE_Byte buffer[8000];
  int nbytes = p->lines * p->bytes_per_line, amnt_read;
  while( nbytes )
  {
    SANE_Byte *pp = buffer;
    if( sane_read( h, buffer, MINIMUM(8000,nbytes), &amnt_read ) )
      return;
    while( amnt_read-- && nbytes--)
    {
      *(data++) = *(pp);
      *(data++) = *(pp);
      *(data++) = *(pp++);
    }
  }
}

static void get_rgb_frame( SANE_Handle h, SANE_Parameters *p,
			   SANE_Byte *data )
{
  SANE_Byte buffer[8000];
  int nbytes = p->lines * p->bytes_per_line, amnt_read;
  while( nbytes )
  {
    SANE_Byte *pp = buffer;
    if( sane_read( h, buffer, MINIMUM(8000,nbytes), &amnt_read ) )
      return;
    while( amnt_read-- && nbytes--)
      *(data++) = *(pp++);
  }
}

static void get_comp_frame( SANE_Handle h, SANE_Parameters *p,
			    SANE_Byte *data )
{
  SANE_Byte buffer[8000];
  int nbytes = p->lines * p->bytes_per_line, amnt_read;
  while( nbytes )
  {
    SANE_Byte *pp = buffer;
    if( sane_read( h, buffer, MINIMUM(8000,nbytes), &amnt_read ) )
      return;
    while( amnt_read-- && nbytes--)
    {
      data[0] = *(pp++);
      data += 3;
    }
  }
}

static void assert_image_program(void)
{
  if( !image_program )
  {
    push_static_text( "Image.Image" );
    APPLY_MASTER( "resolv", 1 );
    image_program = program_from_svalue( sp - 1  );
    sp--;/* Do not free image program.. */
  }
  if( !image_program )
    Pike_error("No Image.Image?!\n");
}

/*! @decl Image.Image simple_scan()
 */
static void f_scanner_simple_scan( INT32 args )
{
  SANE_Parameters p;
  SANE_Handle h = THIS->h;
  struct object *o;
  SANE_Byte *r;	/* Actually an rgb_group *. */

  assert_image_program();

  pop_n_elems( args );
  if( sane_start( THIS->h ) )   Pike_error("Start failed\n");
  if( sane_get_parameters( THIS->h, &p ) )  Pike_error("Get parameters failed\n");

  if( p.depth != 8 )
    Pike_error("Sorry, only depth 8 supported right now.\n");

  push_int( p.pixels_per_line );
  push_int( p.lines );
  o = clone_object( image_program, 2 );
  r = (SANE_Byte *)((struct image *)o->storage)->img;

  THREADS_ALLOW();
  do
  {
    switch( p.format )
    {
     case SANE_FRAME_GRAY:
       get_grey_frame( h, &p, r );
       p.last_frame = 1;
       break;
     case SANE_FRAME_RGB:
       get_rgb_frame( h, &p, r );
       p.last_frame = 1;
       break;
     case SANE_FRAME_RED:
       get_comp_frame( h, &p, r );
       break;
     case SANE_FRAME_GREEN:
       get_comp_frame( h, &p, r + 1 );
       break;
     case SANE_FRAME_BLUE:
       get_comp_frame( h, &p, r + 2 );
       break;
    }
  }
  while( !p.last_frame );

  THREADS_DISALLOW();
  push_object( o );
}

/*! @decl void row_scan(function(Image.Image,int,Scanner:void) callback)
 */
static void f_scanner_row_scan( INT32 args )
{
  SANE_Parameters p;
  SANE_Handle h = THIS->h;
  struct svalue *s;
  struct object *o;
  SANE_Byte *r; /* Actually an rgb_group *. */
  int i, nr;

  if( sane_start( THIS->h ) )               Pike_error("Start failed\n");
  if( sane_get_parameters( THIS->h, &p ) )  Pike_error("Get parameters failed\n");
  if( p.depth != 8 )  Pike_error("Sorry, only depth 8 supported right now.\n");

  assert_image_program();
  switch( p.format )
  {
   case SANE_FRAME_GRAY:
   case SANE_FRAME_RGB:
     break;
   case SANE_FRAME_RED:
   case SANE_FRAME_GREEN:
   case SANE_FRAME_BLUE:
     Pike_error("Composite frame mode not supported for row_scan\n");
     break;
  }
  push_int( p.pixels_per_line );
  push_int( 1 );
  o = clone_object( image_program, 2 );
  r = (SANE_Byte *)((struct image *)o->storage)->img;

  nr = p.lines;
  p.lines=1;

  for( i = 0; i<nr; i++ )
  {
    THREADS_ALLOW();
    switch( p.format )
    {
     case SANE_FRAME_GRAY:
       get_grey_frame( h, &p, r );
       break;
     case SANE_FRAME_RGB:
       get_rgb_frame( h, &p, r );
       break;
     case SANE_FRAME_RED:
     case SANE_FRAME_GREEN:
     case SANE_FRAME_BLUE:
       break;
    }
    THREADS_DISALLOW();
    ref_push_object( o );
    push_int( i );
    ref_push_object( Pike_fp->current_object );
    apply_svalue( sp-args-3, 3 );
    pop_stack();
  }
  free_object( o );
}

struct row_scan_struct
{
  SANE_Handle h;
  SANE_Parameters p;
  rgb_group *r;
  struct object *o;
  struct object *t;
  int current_row;
  SANE_Byte *buffer;
  int bufferpos, nonblocking;
  struct svalue callback;
};

static int nonblocking_row_scan_callback( int fd, void *_c )
{
  struct row_scan_struct *c = (struct row_scan_struct *)_c;
  int done = 0;
  int nbytes;

  do
  {
    int ec;
    THREADS_ALLOW();
    if( (ec = sane_read( c->h, c->buffer+c->bufferpos,
                         c->p.bytes_per_line-c->bufferpos, &nbytes) ) )
    {
      done = 1;
    }
    else
    {
      c->bufferpos += nbytes;
      if( c->bufferpos == c->p.bytes_per_line )
      {
        int i;
        switch( c->p.format )
        {
         case SANE_FRAME_GRAY:
           for( i=0; i<c->p.bytes_per_line; i++ )
           {
             c->r[i].r = c->buffer[i];
             c->r[i].g = c->buffer[i];
             c->r[i].b = c->buffer[i];
           }
           break;
         case SANE_FRAME_RGB:
           memcpy( c->r, c->buffer, c->p.bytes_per_line );
           break;
         default:break;
        }
        c->bufferpos=0;
      }
    }
    THREADS_DISALLOW();

    if( !nbytes || c->bufferpos )
      return 0; /* await more data */

    c->current_row++;

    if( c->current_row == c->p.lines )
      done = 1;

    ref_push_object( c->o );
    push_int( c->current_row-1 );
    ref_push_object( c->t );
    push_int( done );
    apply_svalue( &c->callback, 4 );
    pop_stack();
  } while( c->nonblocking && !done );

  if( done )
  {
    set_read_callback( fd, 0, 0 );
    free_object( c->o );
    free_object( c->t );
    free_svalue( &c->callback );
    free( c->buffer );
    free( c );
  }
  return 0;
}

/*! @decl void nonblocking_row_scan(function(Image.Image,int,Scanner,int:void) callback)
 */
static void f_scanner_nonblocking_row_scan( INT32 UNUSED(args) )
{
  SANE_Parameters p;
  SANE_Handle h = THIS->h;
  struct svalue *s;
  int fd;
  struct row_scan_struct *rsp;

  if( sane_start( THIS->h ) )               Pike_error("Start failed\n");
  if( sane_get_parameters( THIS->h, &p ) )  Pike_error("Get parameters failed\n");
  if( p.depth != 8 )  Pike_error("Sorry, only depth 8 supported right now.\n");

  switch( p.format )
  {
   case SANE_FRAME_GRAY:
   case SANE_FRAME_RGB:
     break;
   case SANE_FRAME_RED:
   case SANE_FRAME_GREEN:
   case SANE_FRAME_BLUE:
     Pike_error("Composite frame mode not supported for row_scan\n");
     break;
  }

  assert_image_program();

  rsp = malloc( sizeof(struct row_scan_struct) );
  push_int( p.pixels_per_line );
  push_int( 1 );
  rsp->o = clone_object( image_program, 2 );
  rsp->t = Pike_fp->current_object;
  add_ref(Pike_fp->current_object);
  rsp->r = ((struct image *)rsp->o->storage)->img;
  rsp->h = THIS->h;
  rsp->p = p;
  rsp->buffer = malloc( p.bytes_per_line );
  rsp->current_row = 0;
  rsp->bufferpos = 0;
  rsp->callback = sp[-1];
  rsp->nonblocking = !sane_set_io_mode( THIS->h, 1 );
  sp--;

  if( sane_get_select_fd( THIS->h, &fd ) )
  {
    free_object( rsp->o );
    free_object( rsp->t );
    free( rsp->buffer );
    free( rsp );
    Pike_error("Failed to get select fd for scanning device!\n");
  }
  set_read_callback( fd, (file_callback)nonblocking_row_scan_callback,
		     (void*)rsp );
}

/*! @decl void cancel_scan()
 */
static void f_scanner_cancel_scan( INT32 UNUSED(args) )
{
  sane_cancel( THIS->h );
}

/*! @endclass
 */

/*! @decl constant FrameGray
 *! @decl constant FrameRGB
 *! @decl constant FrameRed
 *! @decl constant FrameGreen
 *! @decl constant FrameBlue
 */

/*! @endmodule
 */

static void init_scanner_struct( struct object *UNUSED(p) )
{
  THIS->h = 0;
}

static void exit_scanner_struct( struct object *UNUSED(p) )
{
  if( THIS->h )
    sane_close( THIS->h );
}

PIKE_MODULE_INIT
{
  struct program *p;
  ADD_FUNCTION( "list_scanners", f_list_scanners,
                tFunc(tNone,tArr(tMapping)), 0 );

  add_integer_constant( "FrameGray", SANE_FRAME_GRAY,0  );
  add_integer_constant( "FrameRGB",  SANE_FRAME_RGB,0   );
  add_integer_constant( "FrameRed",  SANE_FRAME_RED,0   );
  add_integer_constant( "FrameGreen",SANE_FRAME_GREEN,0 );
  add_integer_constant( "FrameBlue", SANE_FRAME_BLUE,0  );


  start_new_program();
  ADD_STORAGE( struct scanner );
  ADD_FUNCTION( "get_option", f_scanner_get_option, tFunc(tStr, tMix), 0 );
  ADD_FUNCTION( "set_option", f_scanner_set_option,
		tFunc(tStr  tOr(tVoid, tMix), tVoid), 0 );
  ADD_FUNCTION( "list_options", f_scanner_list_options,
		tFunc(tNone, tArr(tMap(tStr, tMix))), 0 );

  ADD_FUNCTION( "simple_scan", f_scanner_simple_scan, tFunc(tNone, tObj), 0 );

  ADD_FUNCTION( "row_scan", f_scanner_row_scan,
		tFunc(tFunc(tObj tInt tObj,tVoid),tVoid), 0 );

  ADD_FUNCTION( "nonblocking_row_scan", f_scanner_nonblocking_row_scan,
		tFunc(tFunc(tObj tInt tObj tInt,tVoid),tVoid), 0 );

  ADD_FUNCTION( "cancel_scan", f_scanner_cancel_scan, tFunc(tNone, tObj), 0 );

  ADD_FUNCTION( "get_parameters", f_scanner_get_parameters,
		tFunc(tNone, tMapping), 0 );

  ADD_FUNCTION( "create", f_scanner_create, tFunc(tStr, tVoid), ID_PROTECTED );

  set_init_callback(init_scanner_struct);
  set_exit_callback(exit_scanner_struct);

  add_program_constant( "Scanner", (p=end_program( ) ), 0 );
  free_program( p );
}

PIKE_MODULE_EXIT
{
  if( sane_is_inited )
    sane_exit();
  if( image_program )
    free_program( image_program );
}

#else
#include "program.h"
#include "module_support.h"
PIKE_MODULE_INIT {
  HIDE_MODULE();
}
PIKE_MODULE_EXIT {}
#endif
