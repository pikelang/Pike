/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: support.c,v 1.3 2005/11/04 19:07:34 grubba Exp $
*/

#include <version.h>
#include <bignum.h>
#include <stdarg.h>

void pgtk_encode_grey(struct image *i, unsigned char *dest, int bpp, int bpl);

void pgtk_verify_setup() {
  if (!pigtk_is_setup)
    Pike_error("You must call GTK2.setup_gtk( argv ) first\n");
}

void pgtk_verify_gnome_setup() {
  extern int gnome_is_setup;
  if (!gnome_is_setup)
    Pike_error("You must call Gnome2.init( app,version,argv ) first\n");
}

void pgtk_verify_inited() {
  if (!THIS->obj)
    Pike_error("Calling function in unitialized object\n");
}

void pgtk_verify_not_inited() {
  if (THIS->obj)
    Pike_error("Tried to initialize object twice\n");
}

void my_pop_n_elems(int n) /* anti-inline */
{
  pop_n_elems(n);
}

void my_ref_push_object(struct object *o) {
  ref_push_object(o);
}

void pgtk_return_this(int n) {
  pop_n_elems(n);
  ref_push_object(Pike_fp->current_object);
}

void pgtk_get_image_module() {
  push_constant_text("Image");
  SAFE_APPLY_MASTER("resolv_or_error",1);
}

void pgtk_index_stack(char *what) {
  push_text(what);
  f_index(2);
#ifdef PIKE_DEBUG
  if (Pike_sp[-1].type==PIKE_T_INT)
    Pike_error("Internal indexing error.\n");
#endif
}

int get_color_from_pikecolor(struct object *o, INT_TYPE *r, INT_TYPE *g, INT_TYPE *b) {
  struct color_struct *col;
  static struct program *pike_color_program;
  if (!pike_color_program) {
    pgtk_get_image_module();
    pgtk_index_stack("Color");
    pgtk_index_stack("Color");
    pike_color_program=program_from_svalue(--Pike_sp);
  }

  col=(struct color_struct *)get_storage(o,pike_color_program);
  if (!col)
    return 0;
  *r=col->rgbl.r/(COLORLMAX/65535);
  *g=col->rgbl.g/(COLORLMAX/65535);
  *b=col->rgbl.b/(COLORLMAX/65535);
  return 1;
}


GdkImage *gdkimage_from_pikeimage(struct object *img, int fast, GdkImage *i) {
  GdkColormap *col=gdk_colormap_get_system();
  GdkVisual *vis=gdk_visual_get_system();
  INT_TYPE x,y;

  /* 1a: create the actual image... */
  TIMER_INIT("Getting extents");
  apply(img,"xsize",0);
  apply(img,"ysize",0);
  get_all_args("internal",2,"%i%i",&x,&y);
  pop_n_elems(2);


  if (x==0 || y==0)
    Pike_error("Size of image must be > 0x0\n");
  if (i) {
    if ((i->width!=x) || (i->height!=y)) {
      gdk_image_destroy((void *)i);
      i=NULL;
    }
  }
  if (!i) {
    PFTIME("Create");
    i=(void *)gdk_image_new(fast,vis,x,y);
  }

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
      pgtk_encode_grey((void *)img->storage,i->mem,i->bpp,i->bpl);
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
      pgtk_encode_truecolor_masks((void *)img->storage,i->bpp*8,pad*8,
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
      MEMSET(allocated,0,sizeof(allocated));
      for (r=0; r<3; r++) for (g=0; g<4; g++) for (b=0; b<3; b++) {
	GdkColor color;
	color.red = (int)(r * (65535/2.0));
	color.green = (int)(g * (65535/3.0));
	color.blue = (int)(b * (65535/2.0));
	color.pixel = 0;
	if (gdk_color_alloc(col,&color))
          if (color.pixel<COLORMAP_SIZE)
            allocated[color.pixel]=1;
      }
      for (r=0; r<6; r++) for (g=0; g<7; g++) for (b=0; b<6; b++) {
	GdkColor color;
	color.red=(int)(r*(65535/5.0));
	color.green=(int)(g*(65535/6.0));
	color.blue=(int)(b*(65535/5.0));
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
      pgtk_get_image_module();
      pgtk_index_stack("colortable");
      /* on stack: array function */
      Pike_sp[0]=Pike_sp[-1];
      Pike_sp[-1]=Pike_sp[-2];
      Pike_sp[-2]=Pike_sp[0];
      /* on stack: function array */
      PFTIME("Creating colormap obj");
      apply_svalue(Pike_sp-2,1);
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
      pgtk_get_image_module();
      pgtk_index_stack("X");
      pgtk_index_stack("encode_pseudocolor");
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
      apply_svalue(Pike_sp-6,5);
      if (Pike_sp[-1].type!=PIKE_T_STRING) {
	gdk_image_destroy((void *)i);
	Pike_error("Failed to convert image\n");
      }
      PFTIME("Converting image");
      MEMCPY(i->mem,Pike_sp[-1].u.string->str,Pike_sp[-1].u.string->len);
      pop_stack(); /* string */
      pop_stack(); /* function */
    }
  }
  TIMER_END();
  return i;
}

int IS_OBJECT_PROGRAM(struct program *X);


void push_gobjectclass(void *obj, struct program *def) {
  struct object *o;
  if (!obj) {
    push_int(0);
    return;
  }
  if (IS_OBJECT_PROGRAM(def))
    if ((o=g_object_get_data(((void *)obj),"pike_object"))) {
      ref_push_object(o);
      return;
    }
  o=low_clone(def);
  call_c_initializers(o);
  ((struct object_wrapper *)o->storage)->obj=obj;
  pgtk__init_object(o);
  ref_push_object(o);
  return;
}


void push_pgdkobject(void *obj, struct program *def) {
  struct object *o;
  if (!obj) {
    push_int(0);
    return;
  }
  o=low_clone(def);
  call_c_initializers(o);
  ((struct object_wrapper *)o->storage)->obj=obj;
  ref_push_object(o);
  return;
}

GObject *get_pgobject(struct object *from, struct program *type) {
  struct object_wrapper * o;
  if (!from)
    return NULL;
  o=(struct object_wrapper *)get_storage(from,type);
  if (!o)
    return 0;
  return o->obj;
}

void *get_pgdkobject(struct object *from, struct program *type) {
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

void my_destruct(struct object *o) {
  struct object_wrapper *ow=
           (struct object_wrapper *)get_storage(o,pg_object_program);
  if (ow) /* This should always be true. But let's add a check anyway. */
    ow->obj=NULL;
  if (o->refs>1)
    destruct(o);
  free_object(o); /* ref added in __init_object below. */
}

void pgtk__init_object(struct object *o) {
  GObject *go=get_gobject(o);
  if (!go) /* Not a real GObject. Refhandling done elsewhere */
    return;
  o->refs++;
/*  gtk_object_set_data_full(go,"pike_object",(void*)o, (void*)my_destruct); */
  g_object_set_data_full(G_OBJECT(go),"pike_object",(void *)o,(void *)my_destruct);
}

void pgtk_get_mapping_arg(struct mapping *map,
                          char *name, int type, int madd,
                          void *dest, long *mask, int len) {
  struct svalue *s;
  if ((s=simple_mapping_string_lookup(map,name))) {
    if (s->type==type) {
      switch(type) {
       case PIKE_T_STRING:
#ifdef PIKE_DEBUG
         if (len!=sizeof(char *))
           Pike_fatal("oddities detected\n");
#endif
         MEMCPY(((char **)dest),&s->u.string->str,sizeof(char *));
         break;
       case PIKE_T_INT:
         if (len==2) {
           short i=(short)s->u.integer;
           MEMCPY(((short *)dest),&i,2);
         } else if (len==4)
           MEMCPY(((int *)dest),&s->u.integer,len);
         break;
       case PIKE_T_FLOAT:
         if (len==sizeof(FLOAT_TYPE))
           MEMCPY(((FLOAT_TYPE *)dest),&s->u.float_number,len);
         else if (len==sizeof(double)) {
           double d=s->u.float_number;
           MEMCPY(((double *)dest),&d,len);
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


struct my_pixel pgtk_pixel_from_xpixel(unsigned int pix, GdkImage *i) {
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
  push_pgdkobject((void *)a,pgdk__atom_program);
}

void push_Xpseudo32bitstring(void *f, int nelems) {
  if (sizeof(long)!=4) {
    long *q=(long *)f;
    int *res=(int *)xalloc(nelems*4),i;
    for (i=0; i<nelems; i++)
      res[i]=q[i];
    push_string(make_shared_binary_string2((const p_wchar2 *)res,nelems));
    xfree(res);
  } else {
    push_string(make_shared_binary_string2(f,nelems));
  }
}

/*
gint pgtk_buttonfuncwrapper(GObject *obj, struct signal_data *d, void *foo) {
  int res;
  push_svalue(&d->args);
  apply_svalue(&d->cb, 1);
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
    push_gdkobject(f,event);
  } else
    push_int(0);
}

enum { PUSHED_NOTHING, PUSHED_VALUE, NEED_RETURN, };

/*
static int pgtk_push_selection_data_param( GValue *a )
{
  push_pgdkobject( GTK_VALUE_POINTER(*a), pgtk_selection_data_program);
  return PUSHED_VALUE;
}
*/

static int pgtk_push_accel_group_param(GValue *a) {
  g_object_ref(g_value_get_pointer(a));
  push_gobjectclass(g_value_get_pointer(a),pgtk_accel_group_program);
  return PUSHED_VALUE;
}

/*
static int pgtk_push_ctree_node_param( GValue *a )
{
  push_pgdkobject( GTK_VALUE_POINTER(*a), pgtk_ctree_node_program);
  return PUSHED_VALUE;
}
*/

static int pgtk_push_gdk_drag_context_param(GValue *a) {
  push_gdkobject(g_value_get_pointer(a),drag_context);
  return PUSHED_VALUE;
}

static int pgtk_push_gdk_event_param(GValue *a) {
  push_gdk_event(g_value_get_boxed(a));
  return NEED_RETURN;
}

static int pgtk_push_int_param(GValue *a) {
  LONGEST retval;
  switch (G_VALUE_TYPE(a)) {
    case G_TYPE_INT:
      retval=(LONGEST)g_value_get_int(a); 
      break;
    case G_TYPE_ENUM:
      retval=(LONGEST)g_value_get_enum(a); 
      break;
    case G_TYPE_FLAGS:
      retval=(LONGEST)g_value_get_flags(a); 
      break;
    case G_TYPE_BOOLEAN:
      retval=(LONGEST)g_value_get_boolean(a); 
      break;
    case G_TYPE_LONG:
      retval=(LONGEST)g_value_get_long(a); 
      break;
    case G_TYPE_CHAR:
      retval=(LONGEST)g_value_get_char(a); 
      break;
    default:
      retval=(LONGEST)g_value_get_uint(a); 
      break;
  }
  push_int64(retval);
  return PUSHED_VALUE;
}

static int pgtk_push_float_param(GValue *a) {
  FLOAT_TYPE retval;
  if (G_VALUE_TYPE(a)==G_TYPE_FLOAT)
    retval=(FLOAT_TYPE)g_value_get_float(a);
  else
    retval=(FLOAT_TYPE)g_value_get_double(a);
  push_float(retval);
  return PUSHED_VALUE;
}

static int pgtk_push_string_param(GValue *a) {
  const gchar *t=g_value_get_string(a);
  if (t)
    PGTK_PUSH_GCHAR(t);
  else
    push_string(empty_pike_string);
  return PUSHED_VALUE;
}

static int pgtk_push_object_param(GValue *a) {
  GObject *obj=g_value_get_object(a);
  if (obj)
    push_gobject(((void *)g_value_get_object(a)));
  return PUSHED_VALUE;
}

static int pgtk_push_pike_object_param(GValue *a) {
  struct object *o=g_value_get_pointer(a);
  if (o)
    ref_push_object(o);
  return PUSHED_VALUE;
}

static int pgtk_push_gparamspec_param(GValue *a) {
}

static struct push_callback {
  int (*callback)(GValue *);
  GType id;
  struct push_callback *next;
} push_callbacks[100], *push_cbtable[63];

static int last_used_callback = 0;

static void insert_push_callback(GType i, int (*cb)(GValue *)) {
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
  CB(G_TYPE_OBJECT,		 pgtk_push_object_param);
  CB(PANGO_TYPE_TAB_ARRAY,	pgtk_push_object_param);
  CB(GTK_TYPE_TEXT_ATTRIBUTES,	pgtk_push_object_param);
  CB(GTK_TYPE_TREE_ITER,	pgtk_push_object_param);
  CB(GTK_TYPE_TREE_MODEL,	pgtk_push_object_param);
  CB(PANGO_TYPE_ATTR_LIST,	pgtk_push_object_param);
  CB(GTK_TYPE_TREE_PATH,	pgtk_push_object_param);
  CB(PANGO_TYPE_FONT_DESCRIPTION,	pgtk_push_object_param);
  CB(PANGO_TYPE_CONTEXT,	pgtk_push_object_param);
  CB(PANGO_TYPE_LAYOUT,		pgtk_push_object_param);

  CB( GTK_TYPE_ACCEL_GROUP,      pgtk_push_accel_group_param );
  /*#ifndef HAS_GTK_20*/
  CB( GDK_TYPE_DRAG_CONTEXT, pgtk_push_gdk_drag_context_param );
  /*#endif*/
  CB( GDK_TYPE_EVENT,        pgtk_push_gdk_event_param );

  CB( GTK_TYPE_ACCEL_FLAGS,      pgtk_push_int_param );
  CB( GDK_TYPE_MODIFIER_TYPE,pgtk_push_int_param );

  CB( G_TYPE_FLOAT,            pgtk_push_float_param );
  CB( G_TYPE_DOUBLE,           pgtk_push_float_param );

  CB( G_TYPE_STRING,           pgtk_push_string_param );

  CB( G_TYPE_INT,              pgtk_push_int_param );
  CB( G_TYPE_ENUM,             pgtk_push_int_param );
  CB( G_TYPE_FLAGS,            pgtk_push_int_param );
  CB( G_TYPE_BOOLEAN,          pgtk_push_int_param );
  CB( G_TYPE_UINT,             pgtk_push_int_param );
  CB( G_TYPE_LONG,             pgtk_push_int_param );
  CB( G_TYPE_ULONG,            pgtk_push_int_param );
  CB( G_TYPE_CHAR,             pgtk_push_int_param );

  CB( G_TYPE_NONE,    NULL ); 
  
  CB( G_TYPE_POINTER,  pgtk_push_pike_object_param );

  CB( G_TYPE_PARAM, pgtk_push_gparamspec_param );
/*
   CB( GTK_TYPE_SIGNAL,   NULL );
   CB( GTK_TYPE_INVALID,  NULL );
 *   This might not be exactly what we want */
}

void push_gvalue_r(GValue *param, GType t) {
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
    char *a="";
    if (!s) {
      a="Unknown child of ";
      s=g_type_name(g_type_parent(t));
      if (!s)
	s="unknown type";
    }
    fprintf(stderr,"** Warning: No push callback for type %d/%d (%s%s)\n",
             t,0,a,s);
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
int pgtk_signal_func_wrapper(struct signal_data *d, ...) {
  int i,return_value=0,j;
  struct svalue *osp=Pike_sp;
  GSignalQuery _opts;
  GSignalQuery *opts=&_opts;
  va_list ptr;

  if (!last_used_callback)
    build_push_callbacks();
  g_signal_query(d->signal_id,opts);
  va_start(ptr,d);
  for (i=j=0; i<opts->n_params; i++) {
    GValue v= {0, };
    gchar *error=NULL;
    g_value_init(&v,opts->param_types[i]);
    G_VALUE_COLLECT(&v,ptr,G_VALUE_NOCOPY_CONTENTS,&error);
    if (error) {
      g_free(error);
      continue;
    }
    push_gvalue_r(&v,opts->param_types[i]);
    j++;
  }
  if (j /*opts->n_params*/)
    f_aggregate(j /*opts->n_params*/);
  {
    GObject *obj=va_arg(ptr,GObject *);
    push_gobject(obj);
  }
  va_end(ptr);

  if (j /*opts->n_params*/)
    stack_swap();
  push_svalue(&d->args);
  if (j /*opts->n_params*/)
    apply_svalue(&d->cb,3);
  else
    apply_svalue(&d->cb,2);
  return_value=Pike_sp[-1].u.integer;
  pop_stack();
  return return_value;
}


void pgtk_free_signal_data(struct signal_data *s, GClosure *gcl) {
  free_svalue(&s->cb);
  free_svalue(&s->args);
  g_free(s);
}

void pgtk_push_gchar(const gchar *s) {
  push_text(s); 
  push_int(1);
  f_utf8_to_string(2);
}

gchar *pgtk_get_str(struct svalue *sv) {
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

void pgtk_free_str(gchar *s) {
  g_free(s);
}

void pgtk_default__sprintf(int args, int offset, int len) {
  int mode = 0;
  if (args>0 && Pike_sp[-args].type==PIKE_T_INT)
    mode=Pike_sp[-args].u.integer;
  my_pop_n_elems(args);
  if (mode!='O') {
    push_undefined();
    return;
  }
  push_string(make_shared_binary_string(__pgtk_string_data+offset,len));
}

void pgtk_clear_obj_struct(struct object *o) {
  MEMSET(Pike_fp->current_storage,0,sizeof(struct object_wrapper));
}


LONGEST pgtk_get_int(struct svalue *s) {
  if (s->type==PIKE_T_INT)
    return s->u.integer;
#ifdef AUTO_BIGNUM
  if (is_bignum_object_in_svalue(s)) {
    LONGEST res;
    int64_from_bignum(&res,s->u.object);
    return res;
  }
#endif
  if (s->type==PIKE_T_FLOAT)
    return (LONGEST)s->u.float_number;
  return 0;
}

int pgtk_is_int(struct svalue *s) {
  return ((s->type==PIKE_T_INT) ||
          (s->type==PIKE_T_FLOAT)
#ifdef AUTO_BIGNUM
          || is_bignum_object_in_svalue(s)
#endif
         );
}

/* double should be enough */
double pgtk_get_float(struct svalue *s) {
  if (s->type==PIKE_T_FLOAT)
    return s->u.float_number;
  if (s->type==PIKE_T_INT)
    return (double)s->u.integer;
#ifdef AUTO_BIGNUM
  if (is_bignum_object_in_svalue(s)) {
    FLOAT_TYPE f;
    push_text("float");
    apply(s->u.object,"cast",1);
    f=Pike_sp[-1].u.float_number;
    pop_stack();
    return (double)f;
  }
#endif
  return 0.0;
}

void pgtk_free_object(struct object *o) {
  free_object(o);
}

int pgtk_is_float(struct svalue *s) {
  return ((s->type==PIKE_T_FLOAT) ||
          (s->type==PIKE_T_INT)
#ifdef AUTO_BIGNUM
          || is_bignum_object_in_svalue(s)
#endif
         );
}

void pgtk_set_property(GObject *g, char *prop, struct svalue *sv) {
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
  if (sv->type==PIKE_T_OBJECT && get_gobject(sv->u.object) &&
		G_IS_OBJECT(get_gobject(sv->u.object))) {
    if (gps->value_type==GDK_TYPE_PIXMAP || gps->value_type==GTK_TYPE_WIDGET)
      g_object_set(g,prop,get_gobject(sv->u.object),NULL);
    return;
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
	gfloat f=pgtk_get_float(sv);
	g_object_set(g,prop,f,NULL);
      }
      break;
    case G_TYPE_DOUBLE:
      {
	gdouble f=pgtk_get_float(sv);
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
      Pike_error("Unable to handle type %s.\n",g_type_name(gps->value_type));
      break;
    default:
      g_object_set(g,prop,PGTK_GETINT(sv),NULL);
      break;
  }
}

void pgtk_get_property(GObject *g, char *prop) {
  GParamSpec *gps=g_object_class_find_property(G_OBJECT_GET_CLASS(g),prop);
  if (!gps)
    Pike_error("This object does not have a property called %s.\n",prop);
  if (!(gps->flags & G_PARAM_READABLE))
    Pike_error("This property is not readable.\n");
  pgtk__low_get_property(g,prop);
}

void pgtk__low_get_property(GObject *g, char *prop) {
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
  if (gps->value_type==GTK_TYPE_TREE_MODEL) {
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
      Pike_error("Unable to handle type %s.\n",g_type_name(gps->value_type));
      break;
  }
}

void pgtk_destroy_store_data(gpointer data) {
  struct store_data *sd=(struct store_data *)data;
  g_free(sd->types);
  g_free(sd);
}

void pgtk_set_gvalue(GValue *gv, GType gt, struct svalue *sv) {
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
    if (sv->type==PIKE_T_OBJECT && get_gobject(sv->u.object) &&
		G_IS_OBJECT(get_gobject(sv->u.object)))
      g_value_set_object(gv,get_gobject(sv->u.object));
    return;
  }
  if (gt==GDK_TYPE_COLOR) {
    if (sv->type==PIKE_T_OBJECT && get_gdkobject(sv->u.object,color))
      g_value_set_boxed(gv,get_gdkobject(sv->u.object,color));
    return;
  }
  if (gt==GDK_TYPE_RECTANGLE) {
    if (sv->type==PIKE_T_OBJECT && get_gdkobject(sv->u.object,rectangle))
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
      g_value_set_char(gv,(gchar)PGTK_GETINT(sv));
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
      g_value_set_float(gv,(gfloat)pgtk_get_float(sv));
      break;
    case G_TYPE_DOUBLE:
      g_value_set_double(gv,(gdouble)pgtk_get_float(sv));
      break;
    case G_TYPE_STRING:
      {
	char *s=PGTK_GETSTR(sv);
	g_value_set_string(gv,s);
	PGTK_FREESTR(s);
      }
      break;
    case G_TYPE_OBJECT:
      if (sv->type==PIKE_T_OBJECT && get_gobject(sv->u.object) &&
		G_IS_OBJECT(get_gobject(sv->u.object)))
	g_value_set_object(gv,get_gobject(sv->u.object));
      else
	g_value_set_object(gv,NULL);
      break;
    case G_TYPE_POINTER:
      if (sv->type==PIKE_T_OBJECT) {
	g_value_set_pointer(gv,sv->u.object);
	add_ref(sv->u.object);
      } else
	g_value_set_pointer(gv,NULL);
      break;
    default:
      Pike_error("Unable to handle type %d - %s.\n",gt,g_type_name(gt));
  }
}

int pgtk_tree_sortable_callback(GtkTreeModel *model, GtkTreeIter *a,
				GtkTreeIter *b, struct signal_data *d) {
  int res;
  push_gobjectclass(model,pgtk_tree_model_program);
  push_gobjectclass(a,pgtk_tree_iter_program);
  push_gobjectclass(b,pgtk_tree_iter_program);
  push_svalue(&d->args);
  apply_svalue(&d->cb,4);
  res=Pike_sp[-1].u.integer;
  pop_stack();
  return res;
}

GObject *pgtk_create_new_obj_with_properties(GType type, struct mapping *m) {
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
    if (k->ind.type==PIKE_T_STRING) {
      gchar *s=PGTK_GETSTR(&k->ind);
      pspec=g_object_class_find_property(class,s);
      if (!pspec) {
	PGTK_FREESTR(s);
	continue;
      }
/*      g_value_init(&params[i].value,G_PARAM_SPEC_VALUE_TYPE(pspec)); */
      pgtk_set_gvalue(&params[i].value,G_PARAM_SPEC_VALUE_TYPE(pspec),&k->val);
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
