#include <version.h>
#include <bignum.h>

void pgtk_encode_grey(struct image *i, unsigned char *dest, int bpp, int bpl );

void pgtk_verify_setup()
{
  if( !pigtk_is_setup )
    Pike_error("You must call GTK.setup_gtk( argv ) first\n");
}

void pgtk_verify_gnome_setup()
{
  extern int gnome_is_setup;
  if( !gnome_is_setup )
    Pike_error("You must call Gnome.init( app,version,argv[,do_corba] ) first\n");
}

void pgtk_verify_inited( )
{
  if(! THIS->obj )
    Pike_error( "Calling function in unitialized object\n" );
}

void pgtk_verify_not_inited( )
{
  if( THIS->obj )
    Pike_error( "Tried to initialize object twice\n" );
}

void my_pop_n_elems( int n ) /* anti-inline */
{
  pop_n_elems( n );
}

void my_ref_push_object( struct object *o )
{
  ref_push_object( o );
}

void pgtk_return_this( int n )
{
  pop_n_elems( n );
  ref_push_object( Pike_fp->current_object );
}

void pgtk_get_image_module()
{
  push_constant_text("Image");
  push_int(0);
  SAFE_APPLY_MASTER("resolv", 2);
  if (Pike_sp[-1].type!=PIKE_T_OBJECT)
    Pike_error("No Image module.\n");
}

void pgtk_index_stack( char *what )
{
  push_text(what);
  f_index(2);
#ifdef DEBUG
  if (Pike_sp[-1].type==PIKE_T_INT)
    Pike_error("Internal indexing error.\n");
#endif
}

int get_color_from_pikecolor( struct object *o, int *r, int *g, int *b )
{
  struct color_struct *col;
  static struct program *pike_color_program;
  if(!pike_color_program)
  {
    pgtk_get_image_module();
    pgtk_index_stack( "Color" );
    pgtk_index_stack( "Color" );
    pike_color_program = program_from_svalue(--Pike_sp);
  }

  col = (struct color_struct *)get_storage( o, pike_color_program );
  if(!col) return 0;
  *r = col->rgbl.r/(COLORLMAX/65535);
  *g = col->rgbl.g/(COLORLMAX/65535);
  *b = col->rgbl.b/(COLORLMAX/65535);
  return 1;
}


GdkImage *gdkimage_from_pikeimage( struct object *img, int fast, GdkImage *i )
{
  GdkColormap *col = gdk_colormap_get_system();
  GdkVisual *vis = gdk_visual_get_system();
  INT_TYPE x, y;

  /* 1a: create the actual image... */
  TIMER_INIT("Getting extents");
  apply(img, "xsize", 0); apply(img, "ysize", 0);
  get_all_args("internal", 2, "%d%d", &x, &y);
  pop_n_elems( 2 );


  if( x==0 || y==0 )
    Pike_error("Size of image must be > 0x0\n");
  if(i)
  {
    if((i->width != x) || (i->height != y))
    {
      gdk_image_destroy((void *)i);
      i = NULL;
    }
  }
  if(!i)
  {
    PFTIME("Create");
    i = (void *)gdk_image_new(fast, vis, x, y);
  }

  if(!i)
    Pike_error("Failed to create gdkimage\n");

  /* 1b: do the work.. */

  if(vis->type == GDK_VISUAL_TRUE_COLOR || vis->type == GDK_VISUAL_STATIC_GRAY)
    /* no colormap.. */
  {
    int pad = 0;
    int native_byteorder;
    PFTIME("Convert");
    if(vis->type == GDK_VISUAL_STATIC_GRAY)
      pgtk_encode_grey( (void *)img->storage, i->mem, i->bpp, i->bpl );
    else
    {
      if(i->bpl != (i->bpp*x))
	switch(i->bpl & 3)
	{
	 case  0: pad = 4; break;
	 case  1: pad = 1; break;
	 case  2: pad = 2; break;
	 case  3: pad = 1; break;
	}
      else
	pad = 0;
      pgtk_encode_truecolor_masks( (void *)img->storage, i->bpp*8, pad*8,
                                   (i->byte_order!=1), vis->red_mask,
                                   vis->green_mask, vis->blue_mask,
                                   i->mem, i->bpl*y );
    }
  } else {
    static int colors_allocated = 0;
    static struct object *pike_cmap;
    /* I hate this... colormaps, here we come.. */
    /* This is rather complicated, but:
       1/ build an array of the colors in the colormap
       2/ use that array to build a pike X-image colormap.
       3/ call Image.X.encode_pseudocolor( img, bpp, lpad, depth, colormp )
       4/ copy the actual data to the image..
    */
    if(!colors_allocated)
    {
#define COLORMAP_SIZE 256
      char allocated[COLORMAP_SIZE];
      int j, i, r, g, b;
      PFTIME("Creating colormap");
      colors_allocated=1;
      MEMSET(allocated, 0, sizeof(allocated));
      for(r=0; r<3; r++) for(g=0; g<4; g++) for(b=0; b<3; b++)
      {
	GdkColor color;
	color.red = (int)(r * (65535/2.0));
	color.green = (int)(g * (65535/3.0));
	color.blue = (int)(b * (65535/2.0));
	color.pixel = 0;
	if(gdk_color_alloc( col, &color ))
          if(color.pixel < COLORMAP_SIZE)
            allocated[ color.pixel ] = 1;
      }
      for(r=0; r<6; r++) for(g=0; g<7; g++) for(b=0; b<6; b++)
      {
	GdkColor color;
	color.red = (int)(r * (65535/5.0));
	color.green = (int)(g * (65535/6.0));
	color.blue = (int)(b * (65535/5.0));
	color.pixel = 0;
	if(gdk_color_alloc( col, &color ))
          if(color.pixel < COLORMAP_SIZE)
            allocated[ color.pixel ] = 1;
      }

      for(i=0; i<COLORMAP_SIZE; i++)
      {
	if( allocated[ i ] )
	{
	  push_int(col->colors[i].red>>8);
	  push_int(col->colors[i].green>>8);
	  push_int(col->colors[i].blue>>8);
	  f_aggregate(3);
	}
	else
	  push_int(0);
      }
      f_aggregate(256);
      /* now on stack: the array with colors. */
      pgtk_get_image_module();
      pgtk_index_stack("colortable");
      /* on stack: array function */
      Pike_sp[0]=Pike_sp[-1];
      Pike_sp[-1]=Pike_sp[-2];
      Pike_sp[-2]=Pike_sp[0];
      /* on stack: function array */
      PFTIME("Creating colormap obj");
      apply_svalue( Pike_sp-2, 1 );
      /* on stack: function cmap */
      get_all_args("internal", 1, "%o", &pike_cmap);
      pike_cmap->refs+=100; /* lets keep this one.. :-) */
      push_int(8); push_int(8); push_int(8);
      apply(pike_cmap, "rigid", 3);      pop_stack();
      apply(pike_cmap, "ordered", 0);    pop_stack();
      pop_stack();
    }

    { /* now we have a colormap available. Happy happy joy joy! */
      struct pike_string *s;
      pgtk_get_image_module();
      pgtk_index_stack( "X" );
      pgtk_index_stack( "encode_pseudocolor" );
      /* on stack: function */
      add_ref(img);
      push_object( img );
      push_int( i->bpp*8 );
      {
	int pad = 0;
	switch(i->bpl - (i->bpp*x))
	{
	 case  0: pad = 0; break;
	 case  1: pad = 16; break;
	 default: pad = 32; break;
	}
	push_int( pad  ); /* extra padding.. */
      }
      push_int( i->depth );
      add_ref(pike_cmap);
      push_object( pike_cmap );
      /* on stack: function img bpp linepad depth cmap*/
      /*             6       5    4  3       2     1 */
      PFTIME("Dithering image");
      apply_svalue( Pike_sp-6, 5 );
      if(Pike_sp[-1].type != PIKE_T_STRING)
      {
	gdk_image_destroy((void *)i);
	Pike_error("Failed to convert image\n");
      }
      PFTIME("Converting image");
      MEMCPY(i->mem, Pike_sp[-1].u.string->str, Pike_sp[-1].u.string->len);
      pop_stack(); /* string */
      pop_stack(); /* function */
    }
  }
  TIMER_END();
  return i;
}

int IS_OBJECT_PROGRAM(struct program *X);

void push_gtkobjectclass(void *obj, struct program *def)
{
  struct object *o;
  if(!obj)
  {
    push_int(0);
    return;
  }
  if( IS_OBJECT_PROGRAM(def) )
    if( (o=gtk_object_get_data(((void *)obj), "pike_object")) )
    {
      ref_push_object( o );
      return;
    }
  o = low_clone( def );
  call_c_initializers( o );
  ((struct object_wrapper *)o->storage)->obj = obj;
  pgtk__init_object( o );
  ref_push_object( o );
  return;
}


void push_pgdkobject(void *obj, struct program *def)
{
  struct object *o;
  if(!obj)
  {
    push_int(0);
    return;
  }
  o = low_clone( def );
  call_c_initializers( o );
  ((struct object_wrapper *)o->storage)->obj = obj;
  ref_push_object( o );
  return;
}

GtkObject *get_pgtkobject(struct object *from, struct program *type)
{
  struct object_wrapper * o;
  if(!from) return NULL;
  o=(struct object_wrapper *)get_storage( from, type );
  if(!o) return 0;
  return o->obj;
}

void *get_pgdkobject(struct object *from, struct program *type)
{
  void *f;
  if(!from) return NULL;
  if(type)
    f = get_storage( from, type );
  else
    f = from->storage; /* Add a warning? */
  if(!f)
    return 0;
  return (void *)((struct object_wrapper *)f)->obj;
}

void my_destruct( struct object *o )
{
  struct object_wrapper *ow =
           (struct object_wrapper *)get_storage( o, pgtk_object_program );
  if( ow ) /* This should always be true. But let's add a check anyway. */
    ow->obj = NULL;
  if( o->refs > 1 )
    destruct( o );
  free_object( o ); /* ref added in __init_object below. */
}

void pgtk__init_object( struct object *o )
{
  GtkObject *go = get_gtkobject(o);
  if(!go) /* Not a real GTK object. Refhandling done elsewhere */
    return;
  o->refs++;
  gtk_object_set_data_full(go,"pike_object",(void*)o, (void*)my_destruct);
}

void pgtk_get_mapping_arg( struct mapping *map,
                           char *name, int type, int madd,
                           void *dest, long *mask, int len )
{
  struct svalue *s;
  if( (s = simple_mapping_string_lookup( map, name )) )
  {
    if( s->type == type )
    {
      switch(type)
      {
       case PIKE_T_STRING:
#ifdef DEBUG
         if(len != sizeof(char *))
           fatal("oddities detected\n");
#endif
         MEMCPY(((char **)dest), &s->u.string->str, sizeof(char *));
         break;
       case PIKE_T_INT:
         if(len == 2)
         {
           short i = (short)s->u.integer;
           MEMCPY(((short *)dest), &i, 2);
         }
         else if(len == 4)
           MEMCPY(((int *)dest), &s->u.integer, len);
         break;
       case PIKE_T_FLOAT:
         if(len == sizeof(FLOAT_TYPE))
           MEMCPY(((FLOAT_TYPE *)dest), &s->u.float_number,len);
         else if(len == sizeof(double))
         {
           double d = s->u.float_number;
           MEMCPY(((double *)dest), &d,len);
         }
         break;
      }
      if( mask )
        *mask |= madd;
    }
  }
}

GdkAtom get_gdkatom( struct object *o )
{
  if(get_gdkobject( o,_atom ))
    return (GdkAtom)get_gdkobject( o, _atom );
  apply( o, "get_atom", 0);
  get_all_args( "internal_get_atom", 1, "%o", &o );
  if(get_gdkobject( o,_atom ))
  {
    GdkAtom r = (GdkAtom)get_gdkobject( o,_atom );
    pop_stack();
    return r;
  }
  Pike_error("Got non GDK.Atom object to get_gdkatom()\n");
}


struct my_pixel pgtk_pixel_from_xpixel( unsigned int pix, GdkImage *i )
{
  static GdkColormap *col;
  GdkColor * c;
  struct my_pixel res;
  int l;
  if(!col) col = gdk_colormap_get_system();
  *((int  *)&res) = 0;
  switch(i->visual->type)
  {
   case GDK_VISUAL_GRAYSCALE:
   case GDK_VISUAL_PSEUDO_COLOR:
     for(l=0; l<col->size; l++)
       if(col->colors[l].pixel == pix) /* 76 */
       {
	 res.r = col->colors[l].red/257;
	 res.g = col->colors[l].green/257;
	 res.b = col->colors[l].blue/257;
	 break;
       }
     break;

   case GDK_VISUAL_STATIC_COLOR:
   case GDK_VISUAL_TRUE_COLOR:
   case GDK_VISUAL_DIRECT_COLOR:
     /* Well well well.... */
     res.r = ((pix&i->visual->red_mask)
	      >> i->visual->red_shift)
	      << (8-i->visual->red_prec);
     res.g = ((pix&i->visual->green_mask)
	      >> i->visual->green_shift)
	      << (8-i->visual->green_prec);
     res.b = ((pix&i->visual->blue_mask)
	      >> i->visual->blue_shift)
	      << (8-i->visual->blue_prec);
     break;
   case GDK_VISUAL_STATIC_GRAY:
     res.r = res.g = res.b = (pix*256) / 1<<i->visual->depth;
     break;
  }
  return res;
}

void push_atom( GdkAtom a )
{
  /* this should really be inserted in the GDK.Atom mapping. */
  push_pgdkobject( (void *)a, pgdk__atom_program );
}

void push_Xpseudo32bitstring( void *f, int nelems )
{
  if( sizeof( long ) != 4 )
  {
    long *q = (long *)f;
    int *res = (int *)xalloc( nelems * 4 ), i;
    for(i=0; i<nelems; i++ )
      res[i] = q[i];
    push_string( make_shared_binary_string2( res, nelems ) );
    xfree( res );
  } else {
    push_string( make_shared_binary_string2( f, nelems ) );
  }
}


gint pgtk_buttonfuncwrapper(GtkObject *obj, struct signal_data *d, void *foo)
{
  int res;
  push_svalue(&d->args);
  apply_svalue(&d->cb, 1);
  res = Pike_sp[-1].u.integer;
  pop_stack();
  return res;
}

void push_gdk_event(GdkEvent *e)
{
  if( e )
  {
    GdkEvent *f = g_malloc( sizeof(GdkEvent) ); 
    *f = *e;
    push_gdkobject( f, event );
  } else
    push_int( 0 );
}

enum { PUSHED_NOTHING, PUSHED_VALUE, NEED_RETURN, };

static int pgtk_push_selection_data_param( GtkArg *a )
{
  push_pgdkobject( GTK_VALUE_POINTER(*a), pgtk_selection_data_program);
  return PUSHED_VALUE;
}

static int pgtk_push_accel_group_param( GtkArg *a )
{
  gtk_accel_group_ref( (void *)GTK_VALUE_POINTER(*a) );
  push_pgdkobject( GTK_VALUE_POINTER(*a), pgtk_accel_group_program);
  return PUSHED_VALUE;
}

static int pgtk_push_ctree_node_param( GtkArg *a )
{
  push_pgdkobject( GTK_VALUE_POINTER(*a), pgtk_ctree_node_program);
  return PUSHED_VALUE;
}

static int pgtk_push_gdk_drag_context_param( GtkArg *a )
{
  push_gdkobject( GTK_VALUE_POINTER(*a), drag_context );
  return PUSHED_VALUE;
}

static int pgtk_push_gdk_event_param( GtkArg *a )
{
  push_gdk_event( GTK_VALUE_POINTER( *a ) );
  return NEED_RETURN;
}

static int pgtk_push_int_param( GtkArg *a )
{
  LONGEST retval;
  switch( a->type ) {
   case GTK_TYPE_INT:   retval = (LONGEST)GTK_VALUE_INT( *a ); break;
   case GTK_TYPE_ENUM:  retval = (LONGEST)GTK_VALUE_ENUM( *a ); break;
   case GTK_TYPE_FLAGS: retval = (LONGEST)GTK_VALUE_FLAGS( *a ); break;
   case GTK_TYPE_BOOL:  retval = (LONGEST)GTK_VALUE_BOOL( *a ); break;
   case GTK_TYPE_LONG:  retval = (LONGEST)GTK_VALUE_LONG( *a ); break;
   case GTK_TYPE_CHAR:  retval = (LONGEST)GTK_VALUE_CHAR( *a ); break;
   default:             retval = (LONGEST)GTK_VALUE_UINT( *a ); break;
  }
  push_int64( retval );
  return PUSHED_VALUE;
}

static int pgtk_push_float_param( GtkArg *a )
{
  FLOAT_TYPE retval;
  if( a->type == GTK_TYPE_FLOAT )
    retval = (FLOAT_TYPE)GTK_VALUE_FLOAT( *a );
  else
    retval = (FLOAT_TYPE)GTK_VALUE_DOUBLE( *a );
  push_float( retval );
  return PUSHED_VALUE;
}

static int pgtk_push_string_param( GtkArg *a )
{
  gchar *t = GTK_VALUE_STRING( *a );
  if( t )
    PGTK_PUSH_GCHAR( t );
  else
    push_text( "" );
  return PUSHED_VALUE;
}

static int pgtk_push_object_param( GtkArg *a )
{
  push_gtkobject( ((void *)GTK_VALUE_OBJECT( *a )) );
  return PUSHED_VALUE;
}

static struct push_callback
{
  int (*callback)(GtkArg *);
  GtkType id;
  struct push_callback *next;
} push_callbacks[100], *push_cbtable[63];

static int last_used_callback = 0;

static void insert_push_callback( GtkType i, int (*cb)(GtkArg *) )
{
  struct push_callback *new = push_callbacks + last_used_callback++;
  struct push_callback *old = push_cbtable[ i%63 ];
  new->id = i;
  new->callback = cb;
  if( old )  new->next = old;
  push_cbtable[ i%63 ] = new;
}

static void build_push_callbacks( )
{
#define CB(X,Y)  insert_push_callback( X, Y );
  CB( gtk_widget_get_type(),     pgtk_push_object_param );
  CB( GTK_TYPE_OBJECT,           pgtk_push_object_param );

  CB( GTK_TYPE_SELECTION_DATA,   pgtk_push_selection_data_param );
  CB( GTK_TYPE_ACCEL_GROUP,      pgtk_push_accel_group_param );
  /*#ifndef HAS_GTK_20*/
  CB( GTK_TYPE_CTREE_NODE,       pgtk_push_ctree_node_param );
#ifndef HAVE_GTK_20
  CB( GTK_TYPE_GDK_DRAG_CONTEXT, pgtk_push_gdk_drag_context_param );
  /*#endif*/
  CB( GTK_TYPE_GDK_EVENT,        pgtk_push_gdk_event_param );
#endif

  CB( GTK_TYPE_ACCEL_FLAGS,      pgtk_push_int_param );
#ifndef HAVE_GTK_20
  CB( GTK_TYPE_GDK_MODIFIER_TYPE,pgtk_push_int_param );
#endif

  CB( GTK_TYPE_FLOAT,            pgtk_push_float_param );
  CB( GTK_TYPE_DOUBLE,           pgtk_push_float_param );

  CB( GTK_TYPE_STRING,           pgtk_push_string_param );

  CB( GTK_TYPE_INT,              pgtk_push_int_param );
  CB( GTK_TYPE_ENUM,             pgtk_push_int_param );
  CB( GTK_TYPE_FLAGS,            pgtk_push_int_param );
  CB( GTK_TYPE_BOOL,             pgtk_push_int_param );
  CB( GTK_TYPE_UINT,             pgtk_push_int_param );
  CB( GTK_TYPE_LONG,             pgtk_push_int_param );
  CB( GTK_TYPE_ULONG,            pgtk_push_int_param );
  CB( GTK_TYPE_CHAR,             pgtk_push_int_param );

  CB( GTK_TYPE_NONE,    NULL ); 
  CB( GTK_TYPE_POINTER,  NULL );

/*
   CB( GTK_TYPE_SIGNAL,   NULL );
   CB( GTK_TYPE_INVALID,  NULL );
 *   This might not be exactly what we want */
  
}
int pgtk_new_signal_call_convention;

static void push_param_r( GtkArg *param, GtkType t )
{
  int i;
  struct push_callback *cb = push_cbtable[ t%63 ];

  while( cb && (cb->id != t) )
    cb = cb->next;

  if(!cb) /* find parent type */
    for( i = 0; i<last_used_callback; i++ )
      if( gtk_type_is_a( t, push_callbacks[i].id ) )
        cb = push_callbacks + i;

  if( cb )
  {
    if( cb->callback ) cb->callback( param );
    return;
  }
  else
  {
    char *s = gtk_type_name( t );
    char *a = "";
    if(!s)
    {
      a = "Unknown child of ";
      s = gtk_type_name( gtk_type_parent( t ) );
      if(!s) s = "unknown type";
    }
    fprintf( stderr, "** Warning: No push callback for type %d/%d (%s%s)\n",
             t, 0,a, s);
  }
  return;
}


/* This function makes a few assumptions about how signal handlers are
 * called in GTK. I could not find any good documentation about that,
 * and the source is somewhat obscure (for good reasons, it's a highly
 * non-trivial thing to do)
 *
 * But, the thing that this code asumes that I am most unsure about is that
 * params[nparams] should be set to the return value. It does seem to work,
 * though.
 */
int pgtk_signal_func_wrapper(GtkObject *obj,struct signal_data *d,
                             int nparams, GtkArg *params)
{
  int i, return_value = 0;
  struct svalue *osp = Pike_sp;
/* #ifdef HAVE_GTK_20 */
/*   GSignalQuery _opts; */
/*   GSignalQuery *opts = &_opts; */
/*   g_signal_query( d->signal_id, &_opts ); */
/* #else */
/*   GtkSignalQuery *opts = gtk_signal_query( d->signal_id ); */
/*   if( !opts ) */
/*   { */
/*     fprintf( stderr, "** Warning: Got signal callback for " */
/*              "non-existing signal!\n" ); */
/*     return 0; */
/*   } */
/* #endif */

  if( !last_used_callback )
    build_push_callbacks();

  if( !(d->new_interface || pgtk_new_signal_call_convention) )
  {
    push_svalue(&d->args);
    push_gtkobject( obj );
  }

  for( i = 0; i<nparams; i++ )
  {
/*     if( params[i].type != opts->params[i] ) */
/*       fprintf( stderr, "** Warning: Parameter type mismatch %d: %u / %u\n" */
/*                "Expect things to break in spectacular ways\n\n" */
/*                "The most likely reason is that GTK has been compiled with\n" */
/*                "a different C-compiler. Especially with gcc that's a very\n" */
/*                "bad idea, since the varargs implementation recides in libgcc\n" */
/*                "which has a tendency to change in incompatible ways now and " */
/*                "then\nPlease recompile gtk+, or change to the C-compiler\n" */
/*                "that was used when GTK+ was compiled.\n", */
/*                i,  params[i].type, opts->params[i] ); */

    push_param_r( params+i,params[i].type );
  }
  
  if( d->new_interface || pgtk_new_signal_call_convention)
  {
    push_gtkobject( obj );
    push_svalue(&d->args);
  }
  apply_svalue(&d->cb, Pike_sp-osp);

  switch( return_value )
  {
   case 0:
   case GTK_TYPE_NONE:
     break;
   case GTK_TYPE_CHAR:
     *GTK_RETLOC_CHAR(params[nparams]) = PGTK_GETINT( Pike_sp-1 );
     break;     
   case GTK_TYPE_UCHAR:
     *GTK_RETLOC_UCHAR(params[nparams]) = PGTK_GETINT( Pike_sp-1 );
     break;     
   case GTK_TYPE_BOOL:
     *GTK_RETLOC_BOOL(params[nparams]) = PGTK_GETINT( Pike_sp-1 );
     break;     
   case GTK_TYPE_INT:
   case GTK_TYPE_ENUM:
     *GTK_RETLOC_INT(params[nparams]) = PGTK_GETINT( Pike_sp-1 );
     break;     
   case GTK_TYPE_UINT:
   case GTK_TYPE_FLAGS:
     *GTK_RETLOC_UINT(params[nparams]) = PGTK_GETINT( Pike_sp-1 );
     break;     
   case GTK_TYPE_LONG:
     *GTK_RETLOC_LONG(params[nparams]) = PGTK_GETINT( Pike_sp-1 );
     break;     
   case GTK_TYPE_ULONG:
     *GTK_RETLOC_ULONG(params[nparams]) = PGTK_GETINT( Pike_sp-1 );
     break;
   case GTK_TYPE_FLOAT:
     *GTK_RETLOC_FLOAT(params[nparams]) = PGTK_GETFLT( Pike_sp-1 );
     break;
   case GTK_TYPE_DOUBLE:
     *GTK_RETLOC_DOUBLE(params[nparams]) = PGTK_GETFLT( Pike_sp-1 );
     break;
   case GTK_TYPE_STRING:
     {
       gchar *s = PGTK_GETSTR(Pike_sp-1 );
       *GTK_RETLOC_STRING(params[nparams]) = g_strdup(s);
       PGTK_FREESTR(s);
     }
     break;
   default:
     {
       char *rn = gtk_type_name( return_value );
       if( !rn ) rn = "unknown type";
       fprintf( stderr, "** Warning: Cannot handle return type %d(%s)\n",
                return_value, rn );
     }
  }
  pop_stack();
/* #ifndef HAVE_GTK_20 */
/*   g_free( opts ); */
/* #endif */
  return return_value;
}

void pgtk_free_signal_data( struct signal_data *s)
{
  free_svalue( &s->cb );
  free_svalue( &s->args );
  xfree(s);
}

#ifdef PGTK_AUTO_UTF8
void pgtk_push_gchar( gchar *s )
{
  push_text( s ); push_int( 1 );
  f_utf8_to_string( 2 );
}

gchar *pgtk_get_str( struct svalue *sv )
{
  gchar *res;

  push_svalue(sv);
  push_int(1);
  f_string_to_utf8(2);

  res = g_malloc( Pike_sp[-1].u.string->len+1 );
  memcpy(res, STR0(Pike_sp[-1].u.string), Pike_sp[-1].u.string->len+1);
  pop_stack();

  return res;
}

void pgtk_free_str( gchar *s )
{
  g_free( s );
}
#endif

void pgtk_default__sprintf( int args, int offset, int len )
{
  my_pop_n_elems( args );
  push_string( make_shared_binary_string( __pgtk_string_data+offset, len ) );
}

void pgtk_clear_obj_struct(struct object *o)
{
  MEMSET(Pike_fp->current_storage, 0, sizeof(struct object_wrapper));
}


LONGEST pgtk_get_int( struct svalue *s )
{
  if( s->type == PIKE_T_INT )
    return s->u.integer;
#ifdef AUTO_BIGNUM
  if( is_bignum_object_in_svalue( s ) )
  {
    LONGEST res;
    int64_from_bignum( &res, s->u.object );
    return res;
  }
#endif
  if( s->type == PIKE_T_FLOAT )
    return (LONGEST)s->u.float_number;
  return 0;
}

int pgtk_is_int( struct svalue *s )
{
  return ((s->type ==PIKE_T_INT) ||
          (s->type ==PIKE_T_FLOAT)
#ifdef AUTO_BIGNUM
          || is_bignum_object_in_svalue( s )
#endif
         );
}

/* double should be enough */
double pgtk_get_float( struct svalue *s )
{
  if( s->type == PIKE_T_FLOAT )
    return s->u.float_number;
  if( s->type == PIKE_T_INT )
    return (double)s->u.integer;
#ifdef AUTO_BIGNUM
  if( is_bignum_object_in_svalue( s ) )
  {
    FLOAT_TYPE f;
    push_text( "float" );
    apply( s->u.object, "cast", 1 );
    f = Pike_sp[-1].u.float_number;
    pop_stack();
    return (double)f;
  }
#endif
  return 0.0;
}

void pgtk_free_object(struct object *o)
{
  free_object( o );
}

int pgtk_is_float( struct svalue *s )
{
  return ((s->type ==PIKE_T_FLOAT) ||
          (s->type ==PIKE_T_INT)
#ifdef AUTO_BIGNUM
          ||is_bignum_object_in_svalue( s )
#endif
         );
}
