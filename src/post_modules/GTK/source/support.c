#include <version.h>

struct image;

void my_pop_n_elems( int n ) /* anti-inline */
{
  pop_n_elems( n );
}

void my_ref_push_object( struct object *o )
{
  ref_push_object( o );
}

void pgtk_encode_truecolor_masks(struct image *i,
                                 int bitspp,
                                 int pad,
                                 int byteorder,
                                 unsigned int red_mask,
                                 unsigned int green_mask,
                                 unsigned int blue_mask,
                                 unsigned char *buffer, 
                                 int debuglen);


typedef struct 
{
   unsigned char r,g,b;
} p_rgb_group;

typedef struct
{
   INT32 r,g,b;
} p_rgbl_group;


struct p_color_struct
{
   p_rgb_group rgb;
   p_rgbl_group rgbl;
   struct pike_string *name;
};

void pgtk_get_image_module()
{
  push_constant_text("Image"); 
  push_int(0);
  SAFE_APPLY_MASTER("resolv", 2);
  if (sp[-1].type!=T_OBJECT) error("No Image module.\n");
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

#define COLORLMAX 0x7fffffff
int get_color_from_pikecolor( struct object *o, int *r, int *g, int *b )
{
  struct p_color_struct *col;
  static struct program *pike_color_program;
  if(!pike_color_program)
  {
    pgtk_get_image_module();
    pgtk_index_stack( "Color" );
    pgtk_index_stack( "Color" );
    pike_color_program = program_from_svalue(--sp);
  }
  
  col = (struct p_color_struct *)get_storage( o, pike_color_program );
  if(!col) return 0;
  *r = col->rgbl.r/(COLORLMAX/65535);
  *g = col->rgbl.g/(COLORLMAX/65535);
  *b = col->rgbl.b/(COLORLMAX/65535);
  return 1;
}



void pgtk_encode_grey(struct image *i, unsigned char *dest, int bpp, int bpl );

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
/*       fprintf(stderr, "New %s%dx%d image\n", (fast?"fast ":""), x, y); */
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
                                   (i->byte_order!=MSBFirst), vis->red_mask, 
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


static void adjust_refs( GtkWidget *w, void *f, void *q)
{
  struct object *o;
  if( !(o=gtk_object_get_data(GTK_OBJECT(w), "pike_object")) )
    return;

  if( w->parent )
  {
    o->refs++;
  }
  else
  {
    free_object( o ); 
  }
}

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
         if(len != sizeof(char *))
           fatal("oddities detected\n");
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
         if(len == sizeof(float))
           MEMCPY(((float *)dest), &s->u.float_number,len);
         else if(len == sizeof(double))
         {
           double d = s->u.float_number;
           MEMCPY(((double *)dest), &d,len);
         }
         break;
      }
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
  res.r = res.g= res.b = 0;
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
     push_text("time");    push_int(e->motion.time);
     push_text("x");       push_float(e->motion.x);
     push_text("y");       push_float(e->motion.y);
     push_text("pressure");push_float(e->motion.pressure);
     push_text("xtilt");   push_float(e->motion.xtilt);
     push_text("ytilt");   push_float(e->motion.ytilt);
     push_text("state");   push_int(e->motion.state);
     push_text("time");    push_int(e->motion.time);
     push_text("deviceid");push_int(e->motion.deviceid);
     push_text("x_root");  push_float(e->motion.x_root);
     push_text("y_root");  push_int(e->motion.y_root);
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
     push_text("y_root");  push_int(e->button.y_root);
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
     break;

   case GDK_FOCUS_CHANGE:
     push_text("type");   push_text("focus");
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
     break;
   case GDK_UNMAP:
     push_text("type");   push_text("unmap");
     break;

   case GDK_PROPERTY_NOTIFY:
     push_text("type");   push_text("property");
     break;

   case GDK_SELECTION_CLEAR:
     push_text("type");   push_text("selection_clear");
     break;

   case GDK_SELECTION_REQUEST:
     push_text("type");   push_text("selection_request");
     break;

   case GDK_SELECTION_NOTIFY:
     push_text("type");   push_text("selection_notify");
     break;

   case GDK_PROXIMITY_IN:
     push_text("type");   push_text("proximity_in");
     break;
   case GDK_PROXIMITY_OUT:
     push_text("type");   push_text("proximity_out");
     break;

   case GDK_CLIENT_EVENT:
     push_text("type"); push_text("client");
     break;

   case GDK_VISIBILITY_NOTIFY:
     push_text("type"); push_text("visibility");
     break;

   case GDK_NO_EXPOSE:
     push_text("type"); push_text("noexpose");
     break;


   case GDK_DRAG_ENTER:
   case GDK_DRAG_LEAVE:
   case GDK_DRAG_MOTION:
   case GDK_DRAG_STATUS:
   case GDK_DROP_START:
   case GDK_DROP_FINISHED:
     break;
  }
  f_aggregate_mapping( sp - osp );
}


int pgtk_signal_func_wrapper(GtkObject *obj,struct signal_data *d,
                             int nparams, GtkArg *params)
{
  int i, j=0, res, return_value = 0;
  push_svalue(&d->args);
  push_gtkobject( obj );
  for(i=0; i<nparams; i++)
  {
    switch( params[i].type )
    {
     case GTK_TYPE_INT:
     case GTK_TYPE_ENUM:
     case GTK_TYPE_FLAGS:
       push_int( (int)GTK_VALUE_INT(params[i]) );
       break;
     case GTK_TYPE_BOOL:
       push_int( (int)GTK_VALUE_BOOL(params[i]) );
       break;
     case GTK_TYPE_UINT:
       push_int( (int)GTK_VALUE_UINT(params[i]) );
       break;
     case GTK_TYPE_LONG:
       push_int( (int)GTK_VALUE_LONG(params[i]) );
       break;

     case GTK_TYPE_CHAR:
       push_int( (int)((unsigned char)GTK_VALUE_CHAR( params[i] )) );
       break;

     case GTK_TYPE_ULONG:
       push_int( (int)GTK_VALUE_ULONG(params[i]) );
       break;
     case GTK_TYPE_INVALID:
     case GTK_TYPE_NONE:
       push_int( 0 );
       break;
     case GTK_TYPE_FLOAT:
       push_float( (float)GTK_VALUE_FLOAT(params[i]) );
       break;
     case GTK_TYPE_DOUBLE:
       push_float( (float)GTK_VALUE_DOUBLE(params[i]) );
       break;

     case GTK_TYPE_POINTER: /* These probably need fixing.. / Hubbe */
     case GTK_TYPE_FOREIGN:

     case GTK_TYPE_STRING:
       if(GTK_VALUE_STRING( params[i] ))
	 push_text( GTK_VALUE_STRING( params[i] ) );
       else
	 push_text( "" );
       break;

     case GTK_TYPE_OBJECT:
       push_gtkobject( GTK_VALUE_OBJECT( params[i] ) );
       break;

     default:
       {
	 char *n = gtk_type_name( params[i].type );
         if(n)
         {
           int ok = 0;
           switch(n[3])
           {
            case 'A':
              if(!strcmp(n, "GtkAccelFlags")) 
              {
                push_int( GTK_VALUE_UINT( params[i] ) );
                ok=1;
              }           
              else if(!strcmp(n, "GtkAccelGroup")) 
              {
                ok=1;
                gtk_accel_group_ref( (void *)GTK_VALUE_POINTER(params[i]) );
                push_pgdkobject( GTK_VALUE_POINTER(params[i]), 
                                 pgtk_accel_group_program); 
              }
              break;

            case 'C':
              if(!strcmp(n, "GtkCTreeNode"))
              {
                ok=1;
                push_pgdkobject( GTK_VALUE_POINTER(params[i]), 
                                 pgtk_CTreeNode_program);
              } 
              else if(!strcmp(n, "GtkCTreeRow"))
              {
                ok=1;
                push_pgdkobject( GTK_VALUE_POINTER(params[i]), 
                                 pgtk_CTreeRow_program);
              }
              break;

            case 'D':
              if(!strcmp(n, "GdkDragContext")) 
              {
                ok=1;
                push_pgdkobject( GTK_VALUE_POINTER(params[i]), 
                                 pgtk_GdkDragContext_program); 
              }
              break;

            case 'E':
              if(!strcmp(n, "GdkEvent"))
              {
                return_value = 1;
                ok=1;
                push_gdk_event( GTK_VALUE_POINTER( params[i] ) );
              } 
              break;

            case 'M':
              if(!strcmp(n, "GdkModifierType")) 
              {
                ok=1;
                push_int( GTK_VALUE_UINT( params[i] ) );
              }
              break;

            case 'S':
              if(!strcmp(n, "GtkSelectionData")) 
              {
                ok=1;
                push_pgdkobject( GTK_VALUE_POINTER(params[i]), 
                                 pgtk_selection_data_program);
              }
              break;

           }
           if(!ok)
           {
             fprintf(stderr,"Unknown param type: %s<value=%p>\n",
                     n, GTK_VALUE_POINTER(params[i]));
             push_text( n );
           }
         }
	 else
	   push_int( 0 );
       }
    }
    j++;
  }
  apply_svalue(&d->cb, 2+j);
  res = sp[-1].u.integer;
  pop_stack();
  if( return_value )
  { 
    if( params[1].type == GTK_TYPE_POINTER)
    {
      gint *return_val;
      return_val = GTK_RETLOC_BOOL( params[ 1 ] );
      if(return_val)
        *return_val = res;
    }
  }
  return res;
}

void pgtk_free_signal_data( struct signal_data *s)
{
  free_svalue( &s->cb );
  free_svalue( &s->args );
  free(s);
}
