/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include <version.h>
#include <bignum.h>
#include <stdarg.h>

#ifndef INIT_VARIABLES
extern struct program *image_color_program;
extern struct program *image_program;
#endif

void pgtk2_encode_grey(struct image *i, unsigned char *dest, int bpp, int bpl);

void pgtk2_verify_setup() {
  if (!pgtk2_is_setup)
    Pike_error("You must call GTK2.setup_gtk( argv ) first\n");
}

void pgtk2_verify_gnome_setup() {
  extern int pgnome2_is_setup;
  if (!pgnome2_is_setup)
    Pike_error("You must call Gnome2.init( app,version,argv ) first\n");
}

void pgtk2_verify_obj_inited() {
  if (!THIS->obj)
    Pike_error("Calling function in unitialized object\n");
}

void pgtk2_verify_obj_not_inited() {
  if (THIS->obj)
    Pike_error("Tried to initialize object twice\n");
}

void pgtk2_verify_mixin_inited() {
  if (!MIXIN_THIS->obj)
    Pike_error("Calling function in unitialized object\n");
}

void pgtk2_verify_mixin_not_inited() {
  if (MIXIN_THIS->obj)
    Pike_error("Tried to initialize object twice\n");
}

void pgtk2_pop_n_elems(int n) /* anti-inline */
{
  pop_n_elems(n);
}

void pgtk2_ref_push_object(struct object *o) {
  ref_push_object(o);
}

void pgtk2_return_this(int n) {
  pop_n_elems(n);
  ref_push_object(Pike_fp->current_object);
}

void pgtk2_get_image_module() {
  push_constant_text("Image");
  SAFE_APPLY_MASTER("resolv_or_error",1);
}

void pgtk2_index_stack(char *what) {
  push_text(what);
  f_index(2);
#ifdef PIKE_DEBUG
  if (TYPEOF(Pike_sp[-1]) == PIKE_T_INT)
    Pike_error("Internal indexing error.\n");
#endif
}

int get_color_from_pikecolor(struct object *o, INT_TYPE *r, INT_TYPE *g, INT_TYPE *b) {
  struct color_struct *col;
  col=get_storage(o,image_color_program);
  if (!col)
    return 0;
  *r=col->rgbl.r/(COLORLMAX/65535);
  *g=col->rgbl.g/(COLORLMAX/65535);
  *b=col->rgbl.b/(COLORLMAX/65535);
  return 1;
}

GdkImage *gdkimage_from_pikeimage(struct object *img, int fast, GObject **pi) {
  GdkImage *i;
  GdkColormap *col=gdk_colormap_get_system();
  GdkVisual *vis=gdk_visual_get_system();
  struct image *img_data;
  INT_TYPE x,y;

  TIMER_INIT("Getting extents");

  img_data=get_storage(img, image_program);

  /* 1a: create the actual image... */
  x = img_data->xsize;
  y = img_data->ysize;


  if (x==0 || y==0)
    Pike_error("Size of image must be > 0x0\n");
  if (pi) {
    i = GDK_IMAGE(*pi);
    if (i != NULL && ((i->width!=x) || (i->height!=y))) {
      g_object_unref(i);
      i=NULL;
    }
  } else
    i=NULL;
  if (!i) {
    PFTIME("Create");
    i=(void *)gdk_image_new(fast,vis,x,y);
  }
  if (pi)
    *pi = G_OBJECT(i);

  if (!i)
    Pike_error("Failed to create gdkimage\n");

  /* 1b: do the work.. */

  if (vis->type==GDK_VISUAL_TRUE_COLOR || vis->type==GDK_VISUAL_STATIC_GRAY)
    /* no colormap.. */
  {
    int pad=0;
    int native_byteorder;
    PFTIME("Convert");
    if (vis->type==GDK_VISUAL_STATIC_GRAY)
      pgtk2_encode_grey(img_data,i->mem,i->bpp,i->bpl);
    else {
      if (i->bpl!=(i->bpp*x))
	switch(i->bpl & 3) {
	 case  0: pad = 4; break;
	 case  1: pad = 1; break;
	 case  2: pad = 2; break;
	 case  3: pad = 1; break;
	}
      else
	pad=0;
      pgtk2_encode_truecolor_masks(img_data,i->bpp*8,pad*8,
				   (i->byte_order!=1),vis->red_mask,
				   vis->green_mask,vis->blue_mask,
				   i->mem, i->bpl*y);
    }
  } else {
    static int colors_allocated=0;
    static struct object *pike_cmap;
    /* I hate this... colormaps, here we come.. */
    /* This is rather complicated, but:
       1/ build an array of the colors in the colormap
       2/ use that array to build a pike X-image colormap.
       3/ call Image.X.encode_pseudocolor( img, bpp, lpad, depth, colormp )
       4/ copy the actual data to the image..
    */
    if (!colors_allocated) {
#define COLORMAP_SIZE 256
      char allocated[COLORMAP_SIZE];
      int j,i,r,g,b;
      PFTIME("Creating colormap");
      colors_allocated=1;
      memset(allocated,0,sizeof(allocated));
      for (r=0; r<3; r++) for (g=0; g<4; g++) for (b=0; b<3; b++) {
	GdkColor color;
	color.red = (guint16)(r * (65535/2.0));
	color.green = (guint16)(g * (65535/3.0));
	color.blue = (guint16)(b * (65535/2.0));
	color.pixel = 0;
	if (gdk_color_alloc(col,&color))
          if (color.pixel<COLORMAP_SIZE)
            allocated[color.pixel]=1;
      }
      for (r=0; r<6; r++) for (g=0; g<7; g++) for (b=0; b<6; b++) {
	GdkColor color;
	color.red=(guint16)(r*(65535/5.0));
	color.green=(guint16)(g*(65535/6.0));
	color.blue=(guint16)(b*(65535/5.0));
	color.pixel=0;
	if (gdk_color_alloc(col,&color))
          if (color.pixel<COLORMAP_SIZE)
            allocated[color.pixel]=1;
      }

      for (i=0; i<COLORMAP_SIZE; i++) {
	if (allocated[i]) {
	  push_int(col->colors[i].red>>8);
	  push_int(col->colors[i].green>>8);
	  push_int(col->colors[i].blue>>8);
	  f_aggregate(3);
	} else
	  push_int(0);
      }
      f_aggregate(256);
      /* now on stack: the array with colors. */
      pgtk2_get_image_module();
      pgtk2_index_stack("colortable");
      /* on stack: array function */
      Pike_sp[0]=Pike_sp[-1];
      Pike_sp[-1]=Pike_sp[-2];
      Pike_sp[-2]=Pike_sp[0];
      /* on stack: function array */
      PFTIME("Creating colormap obj");
      safe_apply_svalue(Pike_sp-2, 1, 1);
      /* on stack: function cmap */
      get_all_args("internal",1,"%o",&pike_cmap);
      pike_cmap->refs+=100; /* lets keep this one.. :-) */
      push_int(8);
      push_int(8);
      push_int(8);
      apply(pike_cmap,"rigid",3);
      pop_stack();
      apply(pike_cmap,"ordered",0);
      pop_stack();
      pop_stack();
    }

    { /* now we have a colormap available. Happy happy joy joy! */
      struct pike_string *s;
      pgtk2_get_image_module();
      pgtk2_index_stack("X");
      pgtk2_index_stack("encode_pseudocolor");
      /* on stack: function */
      add_ref(img);
      push_object(img);
      push_int(i->bpp*8);
      {
	int pad=0;
	switch (i->bpl-(i->bpp*x)) {
	 case  0: pad = 0; break;
	 case  1: pad = 16; break;
	 default: pad = 32; break;
	}
	push_int(pad); /* extra padding.. */
      }
      push_int(i->depth);
      add_ref(pike_cmap);
      push_object(pike_cmap);
      /* on stack: function img bpp linepad depth cmap*/
      /*             6       5    4  3       2     1 */
      PFTIME("Dithering image");
      safe_apply_svalue(Pike_sp-6, 5, 1);
      if (TYPEOF(Pike_sp[-1]) != PIKE_T_STRING) {
	gdk_image_destroy((void *)i);
	Pike_error("Failed to convert image\n");
      }
      PFTIME("Converting image");
      memcpy(i->mem,Pike_sp[-1].u.string->str,Pike_sp[-1].u.string->len);
      pop_stack(); /* string */
      pop_stack(); /* function */
    }
  }
  TIMER_END();
  return i;
}

int pgtk2_is_object_program(struct program *X);


void push_gobjectclass(void *obj, struct program *def) {
  struct object *o;
  if (!obj) {
    push_int(0);
    return;
  }
  if (pgtk2_is_object_program(def))
    if ((o=g_object_get_data(((void *)obj),"pike_object"))) {
      ref_push_object(o);
      return;
    }
  o=fast_clone_object(def);
  ((struct object_wrapper *)o->storage)->obj=obj;
  pgtk2__init_object(o);

  /* Extra ref already added in pgtk2__init_object */
  push_object(o);
  return;
}


void push_pgdk2object(void *obj, struct program *def, int owned) {
  struct object *o;
  if (!obj) {
    push_int(0);
    return;
  }
  o=fast_clone_object(def);
  ((struct object_wrapper *)o->storage)->obj=obj;
  ((struct object_wrapper *)o->storage)->owned = owned;
  push_object(o);
  return;
}

GObject *get_pg2object(struct object *from, struct program *type) {
  struct object_wrapper * o;
  if (!from)
    return NULL;
  o=get_storage(from,type);
  if (!o)
    return 0;
  return o->obj;
}

void *get_pgdk2object(struct object *from, struct program *type) {
  void *f;
  if (!from)
    return NULL;
  if (type)
    f=get_storage( from, type );
  else
    f=from->storage; /* Add a warning? */
  if (!f)
    return 0;
  return (void *)((struct object_wrapper *)f)->obj;
}

void pgtk2_destruct(struct object *o) {
  struct object_wrapper *ow=get_storage(o,pg2_object_program);
  if (ow) /* This should always be true. But let's add a check anyway. */
    ow->obj=NULL;
  if (o->refs>1)
    destruct(o);
  free_object(o); /* ref added in __init_object below. */
}

void pgtk2__init_object(struct object *o) {
  GObject *go=get_gobject(o);
  if (!go) /* Not a real GObject. Refhandling done elsewhere */
    return;
  o->refs++;
/*  gtk_object_set_data_full(go,"pike_object",(void*)o, (void*)pgtk2_destruct); */
  g_object_set_data_full(G_OBJECT(go),"pike_object",(void *)o,(void *)pgtk2_destruct);
}

void pgtk2_get_mapping_arg(struct mapping *map,
                          char *name, int type, int madd,
                          void *dest, long *mask, int len) {
  struct svalue *s;
  if ((s=simple_mapping_string_lookup(map,name))) {
    if (TYPEOF(*s) == type) {
      switch(type) {
       case PIKE_T_STRING:
#ifdef PIKE_DEBUG
         if (len!=sizeof(char *))
           Pike_fatal("oddities detected\n");
#endif
         memcpy(dest,&s->u.string->str,sizeof(char *));
         break;
       case PIKE_T_INT:
         if (len==2) {
           short i=(short)s->u.integer;
           memcpy(dest,&i,2);
         } else if (len==4)
           memcpy(dest,&s->u.integer,len);
         break;
       case PIKE_T_FLOAT:
         if (len==sizeof(FLOAT_TYPE))
           memcpy(dest,&s->u.float_number,len);
         else if (len==sizeof(double)) {
           double d=s->u.float_number;
           memcpy(dest,&d,len);
         }
         break;
      }
      if (mask)
        *mask|=madd;
    }
  }
}

GdkAtom get_gdkatom(struct object *o) {
  if (get_gdkobject(o,_atom))
    return (GdkAtom)get_gdkobject(o,_atom);
  apply(o,"get_atom", 0);
  get_all_args("internal_get_atom",1,"%o",&o);
  if (get_gdkobject(o,_atom)) {
    GdkAtom r=(GdkAtom)get_gdkobject(o,_atom);
    pop_stack();
    return r;
  }
  Pike_error("Got non GDK2.Atom object to get_gdkatom()\n");
}


struct my_pixel pgtk2_pixel_from_xpixel(unsigned int pix, GdkImage *i) {
  static GdkColormap *col;
  GdkColor * c;
  struct my_pixel res;
  int l;
  if (!col)
    col=gdk_colormap_get_system();
  *((int  *)&res)=0;
  switch(i->visual->type) {
   case GDK_VISUAL_GRAYSCALE:
   case GDK_VISUAL_PSEUDO_COLOR:
     for (l=0; l<col->size; l++)
       if (col->colors[l].pixel==pix) /* 76 */
       {
	 res.r=col->colors[l].red/257;
	 res.g=col->colors[l].green/257;
	 res.b=col->colors[l].blue/257;
	 break;
       }
     break;

   case GDK_VISUAL_STATIC_COLOR:
   case GDK_VISUAL_TRUE_COLOR:
   case GDK_VISUAL_DIRECT_COLOR:
     /* Well well well.... */
     res.r=((pix&i->visual->red_mask)
	    >>i->visual->red_shift)
	    <<(8-i->visual->red_prec);
     res.g=((pix&i->visual->green_mask)
	    >>i->visual->green_shift)
	    <<(8-i->visual->green_prec);
     res.b=((pix&i->visual->blue_mask)
	    >>i->visual->blue_shift)
	    <<(8-i->visual->blue_prec);
     break;
   case GDK_VISUAL_STATIC_GRAY:
     res.r=res.g=res.b=(pix*256)/1<<i->visual->depth;
     break;
  }
  return res;
}

void push_atom(GdkAtom a) {
  /* this should really be inserted in the GDK.Atom mapping. */
    push_pgdk2object((void *)a,pgdk2__atom_program,0);
}

void push_Xpseudo32bitstring(void *f, int nelems) {
  if (sizeof(long)!=4) {
    long *q=(long *)f;
    int *res=xalloc(nelems*4),i;
    for (i=0; i<nelems; i++)
      res[i]=q[i];
    push_string(make_shared_binary_string2((const p_wchar2 *)res,nelems));
    xfree(res);
  } else {
    push_string(make_shared_binary_string2(f,nelems));
  }
}

/*
gint pgtk2_buttonfuncwrapper(GObject *obj, struct signal_data *d, void *foo) {
  int res;
  push_svalue(&d->args);
  safe_apply_svalue(&d->cb, 1, 1);
  res=Pike_sp[-1].u.integer;
  pop_stack();
  return res;
}
*/

void push_gdk_event(GdkEvent *e) {
  if (e) {
    GdkEvent *f=g_malloc(sizeof(GdkEvent));
    if (f==NULL) {
      push_int(0);
      return;
    }
    *f=*e;
    push_gdkobject(f,event,1);
  } else
    push_int(0);
}

enum { PUSHED_NOTHING, PUSHED_VALUE, NEED_RETURN, };

static int pgtk2_push_accel_group_param(const GValue *a) {
  g_object_ref(g_value_get_pointer(a));
  push_gobjectclass(g_value_get_pointer(a),pgtk2_accel_group_program);
  return PUSHED_VALUE;
}

/*
static int pgtk2_push_ctree_node_param(const GValue *a )
{
  push_pgdk2object( GTK_VALUE_POINTER(*a), pgtk2_ctree_node_program);
  return PUSHED_VALUE;
}
*/

static int pgtk2_push_gdk_event_param(const GValue *a) {
  push_gdk_event(g_value_get_boxed(a));
  return NEED_RETURN;
}

static int pgtk2_push_gdk_rectangle_param(const GValue *a) {
  GdkRectangle *r = (GdkRectangle *) g_value_get_boxed(a);
  push_static_text("x"); push_int(r->x);
  push_static_text("y"); push_int(r->y);
  push_static_text("width"); push_int(r->width);
  push_static_text("height"); push_int(r->height);
  f_aggregate_mapping(8);
  return PUSHED_VALUE;
}

static int pgtk2_push_int_param(const GValue *a) {
  INT64 retval;
  switch (G_VALUE_TYPE(a)) {
    case G_TYPE_UINT:
      retval=(INT64)g_value_get_uint(a);
      break;
    case G_TYPE_INT64:
      retval=(INT64)g_value_get_int64(a);
      break;
    case G_TYPE_UINT64:
      retval=(INT64)g_value_get_uint64(a);
      break;
    case G_TYPE_INT:
      retval=(INT64)g_value_get_int(a);
      break;
    case G_TYPE_FLAGS:
      retval=(INT64)g_value_get_flags(a);
      break;
    case G_TYPE_BOOLEAN:
      retval=(INT64)g_value_get_boolean(a);
      break;
    case G_TYPE_LONG:
      retval=(INT64)g_value_get_long(a);
      break;
    case G_TYPE_CHAR:
#ifdef HAVE_G_VALUE_GET_SCHAR
      retval=(INT64)g_value_get_schar(a);
#else
      retval=(INT64)g_value_get_char(a);
#endif
      break;
    default:
      retval=(INT64)g_value_get_uint(a);
      break;
  }
  push_int64(retval);
  return PUSHED_VALUE;
}

static int pgtk2_push_enum_param(const GValue *a) {
  /* This can't be handled by push_int_param as the type of
  an enumeration is some subclass of G_TYPE_ENUM, rather than
  actually being G_TYPE_ENUM exactly. */
  push_int64((INT64)g_value_get_enum(a));
  return PUSHED_VALUE;
}

static int pgtk2_push_float_param(const GValue *a) {
  FLOAT_TYPE retval;
  if (G_VALUE_TYPE(a)==G_TYPE_FLOAT)
    retval=(FLOAT_TYPE)g_value_get_float(a);
  else
    retval=(FLOAT_TYPE)g_value_get_double(a);
  push_float(retval);
  return PUSHED_VALUE;
}

static int pgtk2_push_string_param(const GValue *a) {
  const gchar *t=g_value_get_string(a);
  if (t)
    PGTK_PUSH_GCHAR(t);
  else
    push_string(empty_pike_string);
  return PUSHED_VALUE;
}

static int pgtk2_push_object_param(const GValue *a) {
  GObject *obj;
  gpointer *gp;
  if (g_type_is_a(G_VALUE_TYPE(a),G_TYPE_BOXED)) {
    gp=g_value_get_boxed(a);
    if (G_VALUE_HOLDS(a,g_type_from_name("GdkColor"))) {
      push_gdkobject(gp,color,0);
    } else if (G_VALUE_HOLDS(a,g_type_from_name("GtkTreePath"))) {
      push_pgdk2object(gp,pgtk2_tree_path_program,0);
    } else if (G_VALUE_HOLDS(a,g_type_from_name("GtkTextIter"))) {
      push_pgdk2object(gp,pgtk2_text_iter_program,0);
    } else if (G_VALUE_HOLDS(a,g_type_from_name("GtkSelectionData"))) {
      push_pgdk2object(gp,pgtk2_selection_data_program,0);
    } else if (G_VALUE_HOLDS(a,g_type_from_name("GdkRectangle"))) {
      push_gdkobject(gp,rectangle,0);
    } else if (G_VALUE_HOLDS(a,g_type_from_name("GdkRegion"))) {
      push_gdkobject(gp,region,0);
    } else {
      /* Don't know how to push this sort of object, so push its name */
      PGTK_PUSH_GCHAR(G_VALUE_TYPE_NAME(a));
    }
  } else {
    obj=g_value_get_object(a);
    if (obj)
      push_gobject(obj);
  }
  return PUSHED_VALUE;
}

static int pgtk2_push_pike_object_param(const GValue *a) {
  push_int64((INT64)g_value_get_pointer(a));
  return PUSHED_VALUE;
}

static int pgtk2_push_gparamspec_param(const GValue *a) {
    push_int(0);
    return PUSHED_VALUE;
}

static struct push_callback {
  int (*callback)(const GValue *);
  GType id;
  struct push_callback *next;
} push_callbacks[100], *push_cbtable[63];

static int last_used_callback = 0;

static void insert_push_callback(GType i, int (*cb)(const GValue *)) {
  struct push_callback *new=push_callbacks+last_used_callback++;
  struct push_callback *old=push_cbtable[i%63];
  new->id=i;
  new->callback=cb;
  if (old)
    new->next=old;
  push_cbtable[i%63]=new;
}

static void build_push_callbacks() {
#define CB(X,Y)  insert_push_callback(X,Y);
  CB(G_TYPE_OBJECT,		 pgtk2_push_object_param);
  CB(PANGO_TYPE_TAB_ARRAY,	pgtk2_push_object_param);
  CB(GTK_TYPE_TEXT_ATTRIBUTES,	pgtk2_push_object_param);
  CB(GTK_TYPE_TREE_ITER,	pgtk2_push_object_param);
  CB(GTK_TYPE_TREE_MODEL,	pgtk2_push_object_param);
  CB(PANGO_TYPE_ATTR_LIST,	pgtk2_push_object_param);
  CB(GTK_TYPE_TREE_PATH,	pgtk2_push_object_param);
  CB(PANGO_TYPE_FONT_DESCRIPTION,	pgtk2_push_object_param);
  CB(PANGO_TYPE_CONTEXT,	pgtk2_push_object_param);
  CB(PANGO_TYPE_LAYOUT,		pgtk2_push_object_param);

  CB( GTK_TYPE_ACCEL_GROUP,      pgtk2_push_accel_group_param );
  CB( GDK_TYPE_EVENT,        pgtk2_push_gdk_event_param );
  CB( GDK_TYPE_RECTANGLE, pgtk2_push_gdk_rectangle_param );

  CB( GTK_TYPE_ACCEL_FLAGS,      pgtk2_push_int_param );
  CB( GDK_TYPE_MODIFIER_TYPE,pgtk2_push_int_param );

  CB( G_TYPE_FLOAT,            pgtk2_push_float_param );
  CB( G_TYPE_DOUBLE,           pgtk2_push_float_param );

  CB( G_TYPE_STRING,           pgtk2_push_string_param );

  CB( G_TYPE_INT,              pgtk2_push_int_param );
  CB( G_TYPE_INT64,              pgtk2_push_int_param );
  CB( G_TYPE_UINT64,              pgtk2_push_int_param );
  CB( G_TYPE_ENUM,             pgtk2_push_enum_param );
  CB( G_TYPE_FLAGS,            pgtk2_push_int_param );
  CB( G_TYPE_BOOLEAN,          pgtk2_push_int_param );
  CB( G_TYPE_UINT,             pgtk2_push_int_param );
  CB( G_TYPE_LONG,             pgtk2_push_int_param );
  CB( G_TYPE_ULONG,            pgtk2_push_int_param );
  CB( G_TYPE_CHAR,             pgtk2_push_int_param );

  CB( G_TYPE_NONE,    NULL );

  CB( G_TYPE_POINTER,  pgtk2_push_pike_object_param );

  CB( G_TYPE_PARAM, pgtk2_push_gparamspec_param );
  CB( G_TYPE_BOXED, pgtk2_push_object_param );
/*
   CB( GTK_TYPE_SIGNAL,   NULL );
   CB( GTK_TYPE_INVALID,  NULL );
 *   This might not be exactly what we want */
}

void push_gvalue_r(const GValue *param, GType t) {
  int i;
  struct push_callback *cb=push_cbtable[t%63];

  while (cb && (cb->id!=t))
    cb=cb->next;

  if (!cb) /* find parent type */
    for (i=0; i<last_used_callback; i++)
      if (g_type_is_a(t,push_callbacks[i].id))
        cb=push_callbacks+i;

  if (cb) {
    if (cb->callback)
      cb->callback(param);
    return;
  } else {
    const char *s=(char *)g_type_name(t);
    if( s && (s[0] == 'g') ) // FIXME: How to get these types from GTK?
    {
	switch( s[1] )
	{
	    case 'c':
		if( !strcmp( s, "gchararray" ) )
		{
		    pgtk2_push_string_param(param);
		    return;
		}
		break;
	    case 'f':
	    case 'd':
		if( !strcmp( s, "gfloat" ) )
		{
		    push_float( g_value_get_float( param ) );
		    return;
		}
		if( !strcmp( s, "gdouble" ) )
		{
		    push_float( g_value_get_double( param ) );
		    return;
		}
		break;
	    case 'i':
	    case 'u':
		if( !strcmp( s, "gint" ) )
		{
		    push_int(g_value_get_int(param));
		    return;
		}
		else if( !strcmp( s, "guint" ) )
		{
		    push_int64(g_value_get_uint(param));
		    return;
		}
		break;
	}
    }
    {
	char *a="";
	if (!s) {
	    a="Unknown child of ";
	    s=g_type_name(g_type_parent(t));
	    if (!s)
		s="unknown type";
	}
        Pike_error("No push callback for type %lu (%s%s)\n",
                   (unsigned long)t, a, s);
    }
  }
  return;
}

#include <gobject/gvaluecollector.h>

/* This function makes a few assumptions about how signal handlers are
 * called in GTK. I could not find any good documentation about that,
 * and the source is somewhat obscure (for good reasons, it's a highly
 * non-trivial thing to do)
 *
 * But, the thing that this code asumes that I am most unsure about is that
 * params[nparams] should be set to the return value. It does seem to work,
 * though.
 */
/* This function is implement by the functions in the gobject api */
void pgtk2_signal_func_wrapper(struct signal_data *d,
			       gpointer go,
			       guint n_params,
			       const GValue *param_values,
			       GValue *return_value) {
  unsigned int i;

  if (!last_used_callback)
    build_push_callbacks();
  push_gobject(G_OBJECT(go));
  for (i=0; i<n_params; i++) {
    pgtk2_push_gvalue_rt(&(param_values[i]));
  }
  push_svalue(&d->args);
  safe_apply_svalue(&d->cb, 2+n_params, 1);
  if (return_value && G_VALUE_TYPE(return_value) != 0 )
      pgtk2_set_value(return_value,&Pike_sp[-1]);
  pop_stack();
}

void pgtk2_free_signal_data(struct signal_data *s, GClosure *gcl) {
  free_svalue(&s->cb);
  free_svalue(&s->args);
  g_free(s);
}

void pgtk2_push_gchar(const gchar *s) {
  if (s) {
    push_text(s);
    push_int(1);
    f_utf8_to_string(2);
  } else
    push_int(0);
}

gchar *pgtk2_get_str(struct svalue *sv) {
  gchar *res;

  push_svalue(sv);
  push_int(1);
  f_string_to_utf8(2);

  res=(gchar *)g_malloc(Pike_sp[-1].u.string->len+1);
  if (res==NULL) {
    pop_stack();
    return NULL;
  }
  memcpy(res,STR0(Pike_sp[-1].u.string),Pike_sp[-1].u.string->len+1);
  pop_stack();

  return res;
}

void pgtk2_free_str(gchar *s) {
  g_free(s);
}

void pgtk2_get_string_arg_with_sprintf( INT32 args )
{
  if( args < 1 )
    Pike_error("Too few arguments, %d required, got %d\n", 1, args);

  if( TYPEOF(Pike_sp[-args]) != PIKE_T_STRING )
    Pike_error("Illegal argument %d, expected string\n", 0);

  if( args > 1 )
    f_sprintf(args);

  f_string_to_utf8(1);
}

void pgtk2_default__sprintf(int args, int offset, int len) {
  int mode = 0;
  if (args>0 && TYPEOF(Pike_sp[-args]) == PIKE_T_INT)
    mode=Pike_sp[-args].u.integer;
  pgtk2_pop_n_elems(args);
  if (mode!='O') {
    push_undefined();
    return;
  }
  push_string(make_shared_binary_string(__pgtk2_string_data+offset,len));
}

void pgtk2_clear_obj_struct(struct object *o) {
  memset(Pike_fp->current_storage,0,sizeof(struct object_wrapper));
}

void pgtk2_setup_mixin(struct object *o, struct program *p) {
  ptrdiff_t offset;
  offset = low_get_storage(o->prog, p);
  if(offset == -1)
    Pike_error("This class can not be instantiated on its own.\n");
  ((struct mixin_wrapper *)Pike_fp->current_storage)->offset = offset;
}

INT64 pgtk2_get_int(struct svalue *s) {
  if (TYPEOF(*s) == PIKE_T_INT)
    return s->u.integer;
  if (is_bignum_object_in_svalue(s)) {
    INT64 res;
    int64_from_bignum(&res,s->u.object);
    return res;
  }
  if (TYPEOF(*s) == PIKE_T_FLOAT)
    return (INT64)s->u.float_number;
  return 0;
}

int pgtk2_is_int(struct svalue *s) {
  return ((TYPEOF(*s) == PIKE_T_INT) ||
          (TYPEOF(*s) == PIKE_T_FLOAT) ||
          is_bignum_object_in_svalue(s));
}

/* double should be enough */
double pgtk2_get_float(struct svalue *s) {
  if (TYPEOF(*s) == PIKE_T_FLOAT)
    return s->u.float_number;
  if (TYPEOF(*s) == PIKE_T_INT)
    return (double)s->u.integer;
  if (is_bignum_object_in_svalue(s)) {
    FLOAT_TYPE f;
    ref_push_type_value(float_type_string);
    stack_swap();
    f_cast();
    f=Pike_sp[-1].u.float_number;
    pop_stack();
    return (double)f;
  }
  return 0.0;
}

void pgtk2_free_object(struct object *o) {
  free_object(o);
}

int pgtk2_is_float(struct svalue *s) {
  return ((TYPEOF(*s) == PIKE_T_FLOAT) ||
          (TYPEOF(*s) == PIKE_T_INT) ||
          is_bignum_object_in_svalue(s));
}

void pgtk2_set_property(GObject *g, char *prop, struct svalue *sv) {
  GParamSpec *gps;
  GType v;
  gps=g_object_class_find_property(G_OBJECT_GET_CLASS(g),prop);
  if (!gps)
    Pike_error("This object does not have a property called %s.\n",prop);
  if (!(gps->flags & G_PARAM_WRITABLE))
    Pike_error("This property is not writable.\n");
/*
  if (gps->value_type==PANGO_TYPE_STYLE ||
	gps->value_type==GTK_TYPE_WRAP_MODE ||
	gps->value_type==GTK_TYPE_JUSTIFICATION ||
	gps->value_type==PANGO_TYPE_UNDERLINE ||
	gps->value_type==GTK_TYPE_TEXT_DIRECTION) {
    g_object_set(g,prop,PGTK_GETINT(sv),NULL);
    return;
  }
*/
  if (TYPEOF(*sv) == PIKE_T_OBJECT) {
    GObject *go=get_gobject(sv->u.object);
    if (go && G_IS_OBJECT(go)) {
      if (gps->value_type==GDK_TYPE_PIXMAP || gps->value_type==GTK_TYPE_WIDGET)
        g_object_set(g,prop,go,NULL);
      return;
    }
  }
#define do_type(X) do { X i=PGTK_GETINT(sv); g_object_set(g,prop,i,NULL); } while(0)
  switch (gps->value_type) {
    case G_TYPE_INT:
    case G_TYPE_FLAGS:
    case G_TYPE_ENUM:
      do_type(gint);
      break;
    case G_TYPE_UINT:
      do_type(guint);
      break;
    case G_TYPE_BOOLEAN:
      do_type(gboolean);
      break;
    case G_TYPE_CHAR:
      do_type(gchar);
      break;
    case G_TYPE_UCHAR:
      do_type(guchar);
      break;
    case G_TYPE_LONG:
      do_type(glong);
      break;
    case G_TYPE_ULONG:
      do_type(gulong);
      break;
    case G_TYPE_INT64:
      do_type(gint64);
      break;
    case G_TYPE_UINT64:
      do_type(guint64);
      break;
    case G_TYPE_FLOAT:
      {
	gfloat f=pgtk2_get_float(sv);
	g_object_set(g,prop,f,NULL);
      }
      break;
    case G_TYPE_DOUBLE:
      {
	gdouble f=pgtk2_get_float(sv);
	g_object_set(g,prop,f,NULL);
      }
      break;
    case G_TYPE_STRING:
      {
	char *s=PGTK_GETSTR(sv);
	g_object_set(g,prop,s,NULL);
	PGTK_FREESTR(s);
      }
      break;
    case G_TYPE_OBJECT:
      g_object_set(g,prop,get_gobject(sv->u.object),NULL);
      break;
    case G_TYPE_POINTER:
    case G_TYPE_BOXED:
    case G_TYPE_PARAM:
      {
	if (gps->value_type==g_type_from_name("GdkColor")) {
	  GdkColor *gc;
	  gc=(GdkColor *)get_gdkobject(sv->u.object,color);
	  g_object_set(g,prop,gc,NULL);
	} else
	Pike_error("Unable to handle type %s.\n",g_type_name(gps->value_type));
      }
      break;
    default:
      g_object_set(g,prop,PGTK_GETINT(sv),NULL);
      break;
  }
}

void pgtk2_get_property(GObject *g, char *prop) {
  GParamSpec *gps=g_object_class_find_property(G_OBJECT_GET_CLASS(g),prop);
  if (!gps)
    Pike_error("This object does not have a property called %s.\n",prop);
  if (!(gps->flags & G_PARAM_READABLE))
    Pike_error("This property is not readable.\n");
  pgtk2__low_get_property(g,prop);
}

void pgtk2__low_get_property(GObject *g, char *prop) {
  GParamSpec *gps=g_object_class_find_property(G_OBJECT_GET_CLASS(g),prop);
#define get_type(type) do { \
	type i;	\
	g_object_get(g,prop,&i,NULL);	\
	} while(0)
  if (G_TYPE_IS_OBJECT(gps->value_type)) {
    GObject *o;
    g_object_get(g,prop,&o,NULL);
    push_gobject(o);
    return;
  }
/*  if (gps->value_type==GTK_TYPE_TREE_MODEL) { */
  if (G_TYPE_IS_INTERFACE(gps->value_type)) {
    GObject *o;
    g_object_get(g,prop,&o,NULL);
    push_gobject(o);
    return;
  }
  switch (gps->value_type) {
    case G_TYPE_INT:
    case G_TYPE_FLAGS:
    case G_TYPE_ENUM:
      {
	gint i;
	g_object_get(g,prop,&i,NULL);
	push_int(i);
      }
      break;
    case G_TYPE_UINT:
      {
	guint i;
	g_object_get(g,prop,&i,NULL);
	push_int(i);
      }
      break;
    case G_TYPE_BOOLEAN:
      {
	gboolean i;
	g_object_get(g,prop,&i,NULL);
	push_int(i);
      }
      break;
    case G_TYPE_CHAR:
      {
	gchar i;
	g_object_get(g,prop,&i,NULL);
	push_int(i);
      }
      break;
    case G_TYPE_UCHAR:
      {
	guchar i;
	g_object_get(g,prop,&i,NULL);
	push_int(i);
      }
      break;
    case G_TYPE_LONG:
      {
	glong i;
	g_object_get(g,prop,&i,NULL);
	push_int(i);
      }
      break;
    case G_TYPE_ULONG:
      {
	gulong i;
	g_object_get(g,prop,&i,NULL);
	push_int(i);
      }
      break;
    case G_TYPE_INT64:
      {
	gint64 i;
	g_object_get(g,prop,&i,NULL);
	push_int(i);
      }
      break;
    case G_TYPE_UINT64:
      {
	guint64 i;
	g_object_get(g,prop,&i,NULL);
	push_int(i);
      }
      break;
    case G_TYPE_FLOAT:
      {
	gfloat f;
	g_object_get(g,prop,&f,NULL);
	push_float(f);
      }
      break;
    case G_TYPE_DOUBLE:
      {
	gdouble f;
	g_object_get(g,prop,&f,NULL);
	push_float(f);
      }
      break;
    case G_TYPE_STRING:
      {
	gchar *s;
	g_object_get(g,prop,&s,NULL);
	if (s)
	  PGTK_PUSH_GCHAR(s);
	else
          push_string(empty_pike_string);
	g_free(s);
      }
      break;
    case G_TYPE_OBJECT:
      {
	GObject *o;
	g_object_get(g,prop,&o,NULL);
	push_gobject(o);
      }
      break;
    case G_TYPE_BOXED:
    case G_TYPE_POINTER:
    case G_TYPE_PARAM:
    default:
      {
	if (gps->value_type==g_type_from_name("GdkColor")) {
	  GdkColor *gc;
          int args = -1;
	  gc=g_malloc(sizeof(GdkColor));
	  if (gc==NULL)
            SIMPLE_OUT_OF_MEMORY_ERROR(NULL, sizeof(GdkColor));
	  g_object_get(g,prop,gc,NULL);
	  push_gdkobject(gc,color,1);
	} else {
	  Pike_error("Unable to handle type %s.\n",g_type_name(gps->value_type));
	}
      }
      break;
  }
}

void pgtk2_destroy_store_data(gpointer data) {
  struct store_data *sd=(struct store_data *)data;
  g_free(sd->types);
  g_free(sd);
}



void pgtk2_set_gvalue(GValue *gv, GType gt, struct svalue *sv) {
  if (!G_IS_VALUE(gv))
    g_value_init(gv,gt);
  if (G_TYPE_IS_ENUM(gt)) {
    g_value_set_enum(gv,(gint)PGTK_GETINT(sv));
    return;
  }
/*  if (G_TYPE_IS_OBJECT(gt)) { */
  if (G_TYPE_IS_OBJECT(gt) ||
#ifdef HAVE_GTK22
      gt==GDK_TYPE_DISPLAY || gt==GDK_TYPE_SCREEN ||
#endif
      gt==GDK_TYPE_PIXBUF || gt==GDK_TYPE_PIXMAP || gt==GDK_TYPE_IMAGE ||
      gt==GDK_TYPE_WINDOW || gt==GDK_TYPE_VISUAL ||
      gt==GDK_TYPE_DRAWABLE || gt==GDK_TYPE_GC) {
    if (TYPEOF(*sv) == PIKE_T_OBJECT) {
      GObject *go;
      go=get_gobject(sv->u.object);
      if (go && G_IS_OBJECT(go))
	g_value_set_object(gv,go);
      return;
    }
  }
  if (gt==GDK_TYPE_COLOR) {
    if (TYPEOF(*sv) == PIKE_T_OBJECT && get_gdkobject(sv->u.object,color))
      g_value_set_boxed(gv,get_gdkobject(sv->u.object,color));
    return;
  }
  if (gt==GDK_TYPE_RECTANGLE) {
    if (TYPEOF(*sv) == PIKE_T_OBJECT && get_gdkobject(sv->u.object,rectangle))
      g_value_set_boxed(gv,get_gdkobject(sv->u.object,rectangle));
    return;
  }
  switch (gt) {
    case G_TYPE_INT:
      g_value_set_int(gv,(gint)PGTK_GETINT(sv));
      break;
    case G_TYPE_UINT:
      g_value_set_uint(gv,(guint)PGTK_GETINT(sv));
      break;
    case G_TYPE_CHAR:
#ifdef HAVE_G_VALUE_SET_SCHAR
      g_value_set_schar(gv,(gchar)PGTK_GETINT(sv));
#else
      g_value_set_char(gv,(gchar)PGTK_GETINT(sv));
#endif
      break;
    case G_TYPE_UCHAR:
      g_value_set_uchar(gv,(guchar)PGTK_GETINT(sv));
      break;
    case G_TYPE_LONG:
      g_value_set_long(gv,(glong)PGTK_GETINT(sv));
      break;
    case G_TYPE_ULONG:
      g_value_set_ulong(gv,(gulong)PGTK_GETINT(sv));
      break;
    case G_TYPE_INT64:
      g_value_set_int64(gv,(gint64)PGTK_GETINT(sv));
      break;
    case G_TYPE_UINT64:
      g_value_set_uint64(gv,(guint64)PGTK_GETINT(sv));
      break;
    case G_TYPE_ENUM:
      g_value_set_enum(gv,(gint)PGTK_GETINT(sv));
      break;
    case G_TYPE_FLAGS:
      g_value_set_flags(gv,(gint)PGTK_GETINT(sv));
      break;
    case G_TYPE_BOOLEAN:
      g_value_set_boolean(gv,(gboolean)PGTK_GETINT(sv));
      break;
    case G_TYPE_FLOAT:
      g_value_set_float(gv,(gfloat)pgtk2_get_float(sv));
      break;
    case G_TYPE_DOUBLE:
      g_value_set_double(gv,(gdouble)pgtk2_get_float(sv));
      break;
    case G_TYPE_STRING:
      if (TYPEOF(*sv) == PIKE_T_STRING)
      {
	  push_svalue( sv );
	  f_string_to_utf8(1);
	  g_value_set_string(gv,CGSTR0(Pike_sp[-1].u.string));
	  pop_stack();
      } else
	g_value_set_string(gv,"");
      break;
    case G_TYPE_OBJECT:
      if (TYPEOF(*sv) == PIKE_T_OBJECT) {
	GObject *go=get_gobject(sv->u.object);
	if (go && G_IS_OBJECT(go))
	  g_value_set_object(gv,go);
	else
	  g_value_set_object(gv,NULL);
      } else
	g_value_set_object(gv,NULL);
      break;
    case G_TYPE_POINTER:
      if (TYPEOF(*sv) == PIKE_T_OBJECT) {
	g_value_set_pointer(gv,sv->u.object);
 	add_ref(sv->u.object);
      } else
	g_value_set_pointer(gv,NULL);
      break;
      case 0: // void
          break;
    default:
        Pike_error("Unable to handle type %lu - %s.\n",
                   (unsigned long)gt,
                   g_type_name(gt) ?g_type_name(gt): "unnamed" );
  }
}

int pgtk2_tree_sortable_callback(GtkTreeModel *model, GtkTreeIter *a,
				 GtkTreeIter *b, struct signal_data *d) {
  int res;
/*  push_gobjectclass(model,pgtk2_tree_model_program); */
  push_gobject(model);
  push_pgdk2object(a,pgtk2_tree_iter_program,0);
  push_pgdk2object(b,pgtk2_tree_iter_program,0);
  push_svalue(&d->args);
  safe_apply_svalue(&d->cb, 4, 1);
  res=Pike_sp[-1].u.integer;
  pop_stack();
  return res;
}

GObject *pgtk2_create_new_obj_with_properties(GType type, struct mapping *m) {
  GParamSpec *pspec;
  GObject *obj;
  GObjectClass *class;
  GParameter *params;
  struct keypair *k;
  int e;
  int i=0,j;

  class=g_type_class_ref(type);
  if (class==NULL)
    Pike_error("Could not get a reference to type %s.\n",g_type_name(type));
  params=g_new0(GParameter,m_sizeof(m));
  NEW_MAPPING_LOOP(m->data) {
    if (TYPEOF(k->ind) == PIKE_T_STRING) {
      gchar *s=PGTK_GETSTR(&k->ind);
      pspec=g_object_class_find_property(class,s);
      if (!pspec) {
	PGTK_FREESTR(s);
	continue;
      }
/*      g_value_init(&params[i].value,G_PARAM_SPEC_VALUE_TYPE(pspec)); */
      pgtk2_set_gvalue(&params[i].value,G_PARAM_SPEC_VALUE_TYPE(pspec),&k->val);
      params[i++].name=s;
    }
  }
  obj=g_object_newv(type,i,params);
  for (j=0; j<i; j++) {
    PGTK_FREESTR((gchar *)params[j].name);
    g_value_unset(&params[j].value);
  }
  g_free(params);
  g_type_class_unref(class);
  return obj;
}

void pgtk2_marshaller(GClosure *closure,
		      GValue *return_value,
		      guint n_params,
		      const GValue *param_values,
		      gpointer invocation_hint,
		      gpointer marshal_data) {
  typedef void (*pgtk2_marshal_func)(gpointer data1,
				     gpointer data2,
				     guint n_params,
				     const GValue *param_values,
				     GValue *return_value);
  pgtk2_marshal_func callback;
  GCClosure *cc=(GCClosure *)closure;
  gpointer data1,data2;

  if (G_CCLOSURE_SWAP_DATA(closure)) {
    data1=closure->data;
    data2=g_value_peek_pointer(param_values+0);
  } else {
    data1=g_value_peek_pointer(param_values+0);
    data2=closure->data;
  }

  callback=(pgtk2_marshal_func)(marshal_data?marshal_data:cc->callback);
  callback(data1,data2,n_params-1,param_values+1,return_value);
}

int pgtk2_tree_view_row_separator_func(GtkTreeModel *model,
				       GtkTreeIter *iter,
				       struct signal_data *d) {
  int res;
  push_gobject(model);
  push_gobjectclass(iter,pgtk2_tree_iter_program);
  push_svalue(&d->args);
  safe_apply_svalue(&d->cb, 3, 1);
  res=Pike_sp[-1].u.integer;
  pop_stack();
  return res;
}


int pgtk2_entry_completion_match_func( GtkEntryCompletion *x,
                                       const gchar *key,
                                       GtkTreeIter *iter,
                                       struct signal_data *d)
{
  int res;
  push_gobject(x);
  pgtk2_push_gchar( key );
  push_gobjectclass(iter,pgtk2_tree_iter_program);
  safe_apply_svalue( &d->cb, 3, 1 );
  res = Pike_sp[-1].u.integer;
  pop_stack();
  return res;
}

void add_property_docs(GType type, GString *str) {
  GObjectClass *class;
  GParamSpec **props;
  guint n=0,i;
  gboolean has_prop=FALSE;
  G_CONST_RETURN gchar *blurb=NULL;

  class=g_type_class_ref(type);
  props=g_object_class_list_properties(class,&n);

  for (i=0; i<n; i++) {
    if (props[i]->owner_type!=type)
      continue; /* these are from a parent type */

    if (!has_prop) {
      g_string_append_printf(str,"Properties from %s:\n",g_type_name(type));
      has_prop=TRUE;
    }
    g_string_append_printf(str,"  %s - %s: %s\n",
		g_param_spec_get_name(props[i]),
		g_type_name(props[i]->value_type),
		g_param_spec_get_nick(props[i]));
    blurb=g_param_spec_get_blurb(props[i]);
    if (blurb)
      g_string_append_printf(str,"    %s\n",blurb);
  }
  g_free(props);
  if (has_prop)
    g_string_append(str,"\n");
  g_type_class_unref(class);
}

void add_signal_docs(GType type, GString *str) {
  GTypeClass *class=NULL;
  guint *signal_ids,n=0,i;

  if (G_TYPE_IS_CLASSED(type))
    class=g_type_class_ref(type);
  signal_ids=g_signal_list_ids(type,&n);

  if (n>0) {
    g_string_append_printf(str,"Signals from %s:\n",g_type_name(type));
    for (i=0; i<n; i++) {
      GSignalQuery q;
      guint j;

      g_signal_query(signal_ids[i],&q);
      g_string_append(str,"  ");
      g_string_append(str,q.signal_name);
      g_string_append(str," (");
      for (j=0; j<q.n_params; j++) {
	g_string_append(str,g_type_name(q.param_types[j]));
	if (j!=q.n_params-1)
	  g_string_append(str,", ");
      }
      g_string_append(str,")");
      if (q.return_type && q.return_type!=G_TYPE_NONE) {
	g_string_append(str," -> ");
	g_string_append(str,g_type_name(q.return_type));
      }
      g_string_append(str,"\n");
    }
    g_free(signal_ids);
    g_string_append(str,"\n");
  }
  if (class)
    g_type_class_unref(class);
}

void pgtk2_get_doc(GObject *o, struct svalue *dest) {
  GType type=0;
  GString *str;

/*
  if (o)
    type=G_OBJECT_TYPE(G_OBJECT(o)->obj);
  else
    return;
*/
  type=G_OBJECT_TYPE(o);
  str=g_string_new_len(NULL,512);

  if (g_type_is_a(type,G_TYPE_INTERFACE))
    g_string_append_printf(str,"Interface %s\n\n",g_type_name(type));
  else if (g_type_is_a(type,G_TYPE_OBJECT))
    g_string_append_printf(str,"Object %s\n\n",g_type_name(type));

  if (g_type_is_a(type,G_TYPE_OBJECT)) {
    GType parent=G_TYPE_OBJECT;
    GArray *parents=g_array_new(FALSE,FALSE,sizeof(GType));
    int ip;

    while (parent) {
      g_array_append_val(parents,parent);
      parent=g_type_next_base(type,parent);
    }
    for (ip=parents->len-1; ip>=0; --ip) {
      GType *interfaces;
      guint n,i;

      parent=g_array_index(parents,GType,ip);
      add_signal_docs(parent,str);
      add_property_docs(parent,str);

      interfaces=g_type_interfaces(parent,&n);
      for (i=0; i<n; i++)
	add_signal_docs(interfaces[i],str);
      g_free(interfaces);
    }
    g_array_free(parents,TRUE);
  }
  push_string(make_shared_binary_string(str->str,str->len));
  g_string_free(str,TRUE);
  if (dest) {
    assign_svalue_no_free(dest, Pike_sp - 1);
    pop_stack();
  }
}
