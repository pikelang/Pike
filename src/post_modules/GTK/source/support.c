#include <version.h>
#include <bignum.h>

void pgtk_verify_setup()
{
  if( !pigtk_is_setup )
    error("You must call GTK.setup_gtk( argv ) first\n");
}

void pgtk_verify_inited( )
{
  if(! THIS->obj )
    error( "Calling function in unitialized object\n" );
}

void pgtk_verify_not_inited( )
{
  if( THIS->obj )
    error( "Tried to initialize object twice\n" );
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
  ref_push_object( fp->current_object );
}

void pgtk_get_image_module()
{
  push_constant_text("Image");
  push_int(0);
  SAFE_APPLY_MASTER("resolv", 2);
  if (sp[-1].type!=T_OBJECT)
    error("No Image module.\n");
}

void pgtk_index_stack( char *what )
{
  push_text(what);
  f_index(2);
#ifdef DEBUG
  if (sp[-1].type==T_INT)
    error("Internal indexing error.\n");
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
    pike_color_program = program_from_svalue(--sp);
  }

  col = (struct color_struct *)get_storage( o, pike_color_program );
  if(!col) return 0;
  *r = col->rgbl.r/(COLORLMAX/65535);
  *g = col->rgbl.g/(COLORLMAX/65535);
  *b = col->rgbl.b/(COLORLMAX/65535);
  return 1;
}


void *get_swapped_string( struct pike_string *s,int force_wide )
{
  int i;
  unsigned short *res, *tmp;
#if (BYTEORDER!=4321)
  switch(s->size_shift)
  {
   default:
     return 0;
   case 1:
     {
       res= malloc(s->len * 2);
       tmp = (unsigned short *)s->str;
       for(i=0;i<s->len; i++)
	 res[i] = htons( tmp[i] );
       return res;
     }
  }
#endif
  if(force_wide)
  {
    switch(s->size_shift)
    {
     case 0:
       res = malloc(s->len*2);
       for(i=0;i<s->len;i++)
	 res[i] = htons( (short)(((unsigned char *)s->str)[i]) );
       return res;
     case 1:
       return 0;
     case 2:
       res = malloc(s->len*2);
       for(i=0;i<s->len;i++)
	 res[i] = htons( (short)(((unsigned int *)s->str)[i]) );
       return res;
    }
  }
#ifdef DEBUG
  fatal("not reached\n");
#endif
  return 0;
}

struct object *pikeimage_from_gdkimage( GdkImage *img )
{
  return NULL;
}



GdkImage *gdkimage_from_pikeimage( struct object *img, int fast, GdkImage *i )
{
  GdkColormap *col = gdk_colormap_get_system();
  GdkVisual *vis = gdk_visual_get_system();
  int x, y;

  /* 1a: create the actual image... */
  TIMER_INIT("Getting extents");
  apply(img, "xsize", 0); apply(img, "ysize", 0);
  get_all_args("internal", 2, "%d%d", &x, &y);
  pop_n_elems( 2 );


  if( x==0 || y==0 )
    error("Size of image must be > 0x0\n");
  if(i)
  {
    if((i->width != x) || (i->height != y))
    {
/*    fprintf(stderr, "New %s%dx%d image\n", (fast?"fast ":""), x, y); */
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
    error("Failed to create gdkimage\n");

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
      switch(i->bpl - (i->bpp*x))
      {
       case  0: pad = 0; break;
       case  1: pad = 2; break;
       default: pad = 4; break;
      }
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
      sp[0]=sp[-1];
      sp[-1]=sp[-2];
      sp[-2]=sp[0];
      /* on stack: function array */
      PFTIME("Creating colormap obj");
      apply_svalue( sp-2, 1 );
      /* on stack: function cmap */
      get_all_args("internal", 1, "%o", &pike_cmap);
      pike_cmap->refs+=100; /* lets keep this one.. :-) */
      push_int(8); push_int(8); push_int(8);
#if (PIKE_MAJOR_VERSION > 0) || (PIKE_MINOR_VERSION > 6)
      apply(pike_cmap, "rigid", 3);       pop_stack();
#else
      apply(pike_cmap, "cubicles", 3);    pop_stack();
#endif
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
      apply_svalue( sp-6, 5 );
      if(sp[-1].type != T_STRING)
      {
	gdk_image_destroy((void *)i);
	error("Failed to convert image\n");
      }
      PFTIME("Converting image");
      MEMCPY(i->mem, sp[-1].u.string->str, sp[-1].u.string->len);
      pop_stack(); /* string */
      pop_stack(); /* function */
    }
  }
  TIMER_END();
  return i;
}

void push_gtkobjectclass(void *obj, struct program *def)
{
  struct object *o;
  if(!obj)
  {
    push_int(0);
    return;
  }
  if( (o=gtk_object_get_data(GTK_OBJECT(obj), "pike_object")) )
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
    f = from->storage;
  if(!f)
    return 0;
  return (void *)((struct object_wrapper *)f)->obj;
}

/* static void adjust_refs( GtkWidget *w, void *f, void *q) */
/* { */
/*   struct object *o; */
/*   if( !(o=gtk_object_get_data(GTK_OBJECT(w), "pike_object")) ) */
/*     return; */

/*   if( w->parent ) */
/*   { */
/*     o->refs++; */
/*   } */
/*   else */
/*   { */
/*     free_object( o );  */
/*   } */
/* } */

void my_destruct( struct object *o )
{
  GtkObject *go = get_gtkobject(o);
  if(!go) return;
  ((struct object_wrapper *)o->storage)->obj = NULL;
/*   if( !GTK_IS_WIDGET( go ) ) */
  free_object( o );
}

void pgtk__init_object( struct object *o )
{
  GtkObject *go = get_gtkobject(o);
  if(!go)
    fatal("pgtk__init_object called on a non-pgtk object!\n");

  /* A real refcounting system.. Might work, who knows? :-) */
/*   if( GTK_IS_WIDGET( go ) ) */
/*     gtk_signal_connect( go, "parent_set", adjust_refs, NULL ); */
/*   else */
  o->refs++;

  gtk_object_set_data_full(go,"pike_object",
			   (void*)o, (void*)my_destruct);
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
       case T_STRING:
#ifdef DEBUG
         if(len != sizeof(char *))
           fatal("oddities detected\n");
#endif
         MEMCPY(((char **)dest), &s->u.string->str, sizeof(char *));
         break;
       case T_INT:
         if(len == 2)
         {
           short i = (short)s->u.integer;
           MEMCPY(((short *)dest), &i, 2);
         }
         else if(len == 4)
           MEMCPY(((int *)dest), &s->u.integer, len);
         break;
       case T_FLOAT:
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
  if(get_gdkobject( o,_Atom ))
    return (GdkAtom)get_gdkobject( o, _Atom );
  apply( o, "get_atom", 0);
  get_all_args( "internal_get_atom", 1, "%o", &o );
  if(get_gdkobject( o,_Atom ))
  {
    GdkAtom r = (GdkAtom)get_gdkobject( o,_Atom );
    pop_stack();
    return r;
  }
  error("Got non GDK.Atom object to get_gdkatom()\n");
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

static void push_atom( GdkAtom a )
{
  /* this should really be inserted in the GDK.Atom mapping. */
  push_pgdkobject( (void *)a, pgtk_Gdk_Atom_program );
}

void push_Xpseudo32bitstring( void *f, int nelems )
{
  if( sizeof( long ) != 4 )
  {
    long *q = (long *)f;
    int *res = malloc( nelems * 4 ), i;
    for(i=0; i<nelems; i++ )
      res[i] = q[i];
    push_string( make_shared_binary_string2( res, nelems ) );
    free( res );
  } else {
    push_string( make_shared_binary_string2( f, nelems ) );
  }
}


int pgtkbuttonfuncwrapper(GtkObject *obj, struct signal_data *d, void *foo)
{
  int res;
  push_svalue(&d->args);
  apply_svalue(&d->cb, 1);
  res = sp[-1].u.integer;
  pop_stack();
  return res;
}

void push_gdk_event(GdkEvent *e)
{
  struct svalue *osp = sp;
  if(!e)
  {
    push_text("NullEvent"); /* Why, o why? */
    return;
  }
  switch(e->type)
  {
   case GDK_NOTHING:
     push_text("type");  push_text("nothing");
     break;
   case GDK_DELETE:
     push_text("type");  push_text("delete");
     break;
   case GDK_DESTROY:
     push_text("type");  push_text("destroy");
     break;
   case GDK_EXPOSE:
     push_text("type");    push_text("expose");
     push_text("count");   push_int(e->expose.count);
     push_text("x");       push_int(e->expose.area.x);
     push_text("y");       push_int(e->expose.area.y);
     push_text("width");   push_int(e->expose.area.width);
     push_text("height");  push_int(e->expose.area.height);
     break;

   case GDK_MOTION_NOTIFY:
     push_text("type");    push_text("motion");
     push_text("x");       push_float(e->motion.x);
     push_text("y");       push_float(e->motion.y);
     push_text("pressure");push_float(e->motion.pressure);
     push_text("xtilt");   push_float(e->motion.xtilt);
     push_text("ytilt");   push_float(e->motion.ytilt);
     push_text("state");   push_int(e->motion.state);
     push_text("deviceid");push_int(e->motion.deviceid);
     push_text("is_hint"); push_int(e->motion.is_hint);
     push_text("x_root");  push_float(e->motion.x_root);
     push_text("y_root");  push_float(e->motion.y_root);
     break;

   case GDK_BUTTON_PRESS:
     push_text("type");    push_text("button_press");
     goto press_event;
   case GDK_2BUTTON_PRESS:
     push_text("type");    push_text("2button_press");
     goto press_event;
   case GDK_3BUTTON_PRESS:
     push_text("type");    push_text("3button_press");
     goto press_event;
   case GDK_BUTTON_RELEASE:
     push_text("type");    push_text("button_release");

  press_event:
     push_text("time");    push_int(e->button.time);
     push_text("x");       push_float(e->button.x);
     push_text("y");       push_float(e->button.y);
     push_text("pressure");push_float(e->button.pressure);
     push_text("xtilt");   push_float(e->button.xtilt);
     push_text("ytilt");   push_float(e->button.ytilt);
     push_text("state");   push_int(e->button.state);
     push_text("button");    push_int(e->button.button);
     push_text("deviceid");push_int(e->button.deviceid);
     push_text("x_root");  push_float(e->button.x_root);
     push_text("y_root");  push_float(e->button.y_root);
     break;

   case GDK_KEY_PRESS:
     push_text("type");   push_text("key_press");
     goto key_event;

   case GDK_KEY_RELEASE:
     push_text("type");   push_text("key_release");
  key_event:
     push_text("time");   push_int(e->key.time);
     push_text("state");   push_int(e->key.state);
     push_text("keyval");   push_int(e->key.keyval);
     if(e->key.string)
     {
       push_text("data");
       push_string(make_shared_binary_string(e->key.string, e->key.length));
     }
     break;

   case GDK_ENTER_NOTIFY:
     push_text("type");   push_text("enter_notify");
     goto enter_event;

   case GDK_LEAVE_NOTIFY:
     push_text("type");   push_text("leave_notify");
  enter_event:
     push_text("detail"); push_int(e->crossing.detail);
     push_text("mode");   push_int(e->crossing.mode );
     push_text("focus");   push_int(e->crossing.focus );
     push_text("state");   push_int(e->crossing.state );
     push_text("x_root");   push_float(e->crossing.x_root );
     push_text("y_root");   push_float(e->crossing.y_root );
     push_text("x");   push_float(e->crossing.x );
     push_text("y");   push_float(e->crossing.y );
     break;

   case GDK_FOCUS_CHANGE:
     push_text("type");   push_text("focus");
     push_text("in");     push_int( e->focus_change.in );
     break;

   case GDK_CONFIGURE:
     push_text("type");   push_text("configure");
     push_text("x"); push_int(e->configure.x);
     push_text("y"); push_int(e->configure.x);
     push_text("width"); push_int(e->configure.width);
     push_text("height"); push_int(e->configure.height);
     break;

   case GDK_MAP:
     push_text("type");   push_text("map");
     push_text("x"); push_int(e->configure.x);
     push_text("y"); push_int(e->configure.x);
     push_text("width"); push_int(e->configure.width);
     push_text("height"); push_int(e->configure.height);
     break;

   case GDK_UNMAP:
     push_text("type");   push_text("unmap");
     push_text("x"); push_int(e->configure.x);
     push_text("y"); push_int(e->configure.x);
     push_text("width"); push_int(e->configure.width);
     push_text("height"); push_int(e->configure.height);
     break;

   case GDK_PROPERTY_NOTIFY:
     push_text("type");   push_text("property");
     push_text("atom");   push_atom( e->property.atom );
     push_text("state");  push_int( e->property.state );
     break;

   case GDK_SELECTION_CLEAR:
     push_text("type");   push_text("selection_clear");
     goto selection_event;

   case GDK_SELECTION_REQUEST:
     push_text("type");   push_text("selection_request");
     goto selection_event;

   case GDK_SELECTION_NOTIFY:
     push_text("type");   push_text("selection_notify");
  selection_event:
     push_text("selection"); push_atom( e->selection.selection );
     push_text("target");    push_atom( e->selection.target );
     push_text("property");  push_atom( e->selection.property );
     push_text("requestor"); push_int( e->selection.requestor );
     break;

   case GDK_PROXIMITY_IN:
     push_text("type");   push_text("proximity_in");
     push_text( "deviceid" ); push_int( e->proximity.deviceid );
     break;
   case GDK_PROXIMITY_OUT:
     push_text("type");   push_text("proximity_out");
     push_text( "deviceid" ); push_int( e->proximity.deviceid );
     break;

   case GDK_CLIENT_EVENT:
     push_text("type"); push_text("client");
     push_text( "message_type" );
     push_atom( e->client.message_type );
     push_text("data_format"); push_int(e->client.data_format);
     push_text( "data" );
     switch( e->client.data_format )
     {
      case 8:
        push_string( make_shared_binary_string( e->client.data.b, 20 ) );
        break;
      case 16:
        push_string(make_shared_binary_string1(e->client.data.s, 10));
        break;
      case 32:
        push_Xpseudo32bitstring( e->client.data.l, 5 );
        break;
     }
     break;

   case GDK_VISIBILITY_NOTIFY:
     push_text("type"); push_text("visibility");
     push_text("state"); push_int( e->visibility.state );
     break;

   case GDK_NO_EXPOSE:
     push_text("type"); push_text("noexpose");
     break;


   case GDK_DRAG_ENTER:
     push_text("type"); push_text("drag_enter");
     goto dnd_event;
   case GDK_DRAG_LEAVE:
     push_text("type"); push_text("drag_leave");
     goto dnd_event;
   case GDK_DRAG_MOTION:
     push_text("type"); push_text("drag_motion");
     goto dnd_event;
   case GDK_DRAG_STATUS:
     push_text("type"); push_text("drag_status");
     goto dnd_event;
   case GDK_DROP_START:
     push_text("type"); push_text("drop_start");
     goto dnd_event;
   case GDK_DROP_FINISHED:
     push_text("type"); push_text("drop_finished");
   dnd_event:
     push_text( "send_event" ); push_int( e->dnd.send_event );
     push_text( "x_root" ); push_int( e->dnd.x_root );
     push_text( "y_root" ); push_int( e->dnd.y_root );
     push_text( "context" );
     push_gdkobject( e->dnd.context, DragContext);
     break;
  }
  f_aggregate_mapping( sp - osp );
}

enum { PUSHED_NOTHING, PUSHED_VALUE, NEED_RETURN, };

static int pgtk_push_selection_data_param( GtkArg *a )
{
  fprintf( stderr, " Pushing selection data object %p now...\n",
           GTK_VALUE_POINTER(*a));
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
  push_pgdkobject( GTK_VALUE_POINTER(*a), pgtk_CTreeNode_program);
  return PUSHED_VALUE;
}

static int pgtk_push_ctree_row_param( GtkArg *a )
{
  push_pgdkobject( GTK_VALUE_POINTER(*a), pgtk_CTreeRow_program);
  return PUSHED_VALUE;
}

static int pgtk_push_gdk_drag_context_param( GtkArg *a )
{
  push_pgdkobject( GTK_VALUE_POINTER(*a), pgtk_GdkDragContext_program);
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
  switch( a->type )
  {
   case GTK_TYPE_INT:   retval = (LONGEST)GTK_VALUE_INT( *a ); break;
   case GTK_TYPE_ENUM:  retval = (LONGEST)GTK_VALUE_ENUM( *a ); break;
   case GTK_TYPE_FLAGS: retval = (LONGEST)GTK_VALUE_FLAGS( *a ); break;
   case GTK_TYPE_BOOL:  retval = (LONGEST)GTK_VALUE_BOOL( *a ); break;
   case GTK_TYPE_LONG:  retval = (LONGEST)GTK_VALUE_LONG( *a ); break;
   case GTK_TYPE_CHAR:  retval = (LONGEST)GTK_VALUE_CHAR( *a ); break;
   default: retval = (LONGEST)GTK_VALUE_UINT( *a ); break;
  }
  if( retval < 0x7fffffff )
    push_int( retval );
  else
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
    push_text( t );
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
  unsigned int id;
  struct push_callback *next;
} push_callbacks[100], *push_cbtable[63];

static int last_used_callback = 0;

static void insert_push_callback( unsigned int i, int (*cb)(GtkArg *) )
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
#define CB(X,Y)  insert_push_callback( X, Y )
  CB( GTK_TYPE_INT,   pgtk_push_int_param );
  CB( GTK_TYPE_ENUM,  pgtk_push_int_param );
  CB( GTK_TYPE_FLAGS, pgtk_push_int_param );
  CB( GTK_TYPE_BOOL,  pgtk_push_int_param );
  CB( GTK_TYPE_UINT,  pgtk_push_int_param );
  CB( GTK_TYPE_LONG,  pgtk_push_int_param );
  CB( GTK_TYPE_CHAR,  pgtk_push_int_param );
  CB( GTK_TYPE_ACCEL_FLAGS, pgtk_push_int_param );
  CB( GTK_TYPE_GDK_MODIFIER_TYPE, pgtk_push_int_param );
  CB( GTK_TYPE_FLOAT,  pgtk_push_float_param );
  CB( GTK_TYPE_DOUBLE, pgtk_push_float_param );
  CB( GTK_TYPE_STRING, pgtk_push_string_param );
  CB( GTK_TYPE_OBJECT, pgtk_push_object_param );
  CB( GTK_TYPE_SELECTION_DATA, pgtk_push_selection_data_param );
  CB( GTK_TYPE_ACCEL_GROUP, pgtk_push_accel_group_param );
  CB( GTK_TYPE_CTREE_NODE,  pgtk_push_ctree_node_param );
  CB( GTK_TYPE_GDK_DRAG_CONTEXT, pgtk_push_gdk_drag_context_param );
  CB( GTK_TYPE_GDK_EVENT, pgtk_push_gdk_event_param );
  CB( GTK_TYPE_POINTER, NULL ); /* This might not be exactly what we want */
  CB( GTK_TYPE_INVALID, NULL ); /* This might not be exactly what we want */
}

static int push_param( GtkArg *param )
{
  unsigned int t = GTK_TYPE_SEQNO(param->type);
  struct push_callback *cb = push_cbtable[ t%63 ];
  while( cb && (cb->id != t) ) cb = cb->next;
  if( cb )
  {
    if( cb->callback )
      if( cb->callback( param )== NEED_RETURN)
        return 1;
  }
  else
  {
#ifdef DEBUG
    char *s = gtk_type_name( t );
    if(!s) s = "Unknown Type";
    fprintf( stderr, "No callback for %d (%s)\n",t, s);
    gtk_type_describe_heritage( t );
    gtk_type_describe_tree( t, 1 );
#endif
    if( GTK_FUNDAMENTAL_TYPE(param->type) != t )
    {
      GtkArg p  = *param;
      p.type = GTK_FUNDAMENTAL_TYPE(param->type);
      return push_param( &p );
    }
  }
  return 0;
}

int pgtk_signal_func_wrapper(GtkObject *obj,struct signal_data *d,
                             int nparams, GtkArg *params)
{
  int i, j=0, res, return_value = 0;
  struct svalue *osp = sp;

  if( !last_used_callback ) build_push_callbacks();

  push_svalue(&d->args);
  push_gtkobject( obj );

  for(i=0; !return_value && (i<nparams); i++)
    return_value = push_param( params+i );

  apply_svalue(&d->cb, sp-osp);
  res = sp[-1].u.integer;
  pop_stack();

  if( return_value )
    if( params[1].type == GTK_TYPE_POINTER)
    {
      gint *return_val;
      return_val = GTK_RETLOC_BOOL( params[ 1 ] );
      if(return_val)
        *return_val = res;
    }
  return res;
}

void pgtk_free_signal_data( struct signal_data *s)
{
  free_svalue( &s->cb );
  free_svalue( &s->args );
  free(s);
}
