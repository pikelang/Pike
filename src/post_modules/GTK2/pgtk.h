/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/* Sort of unnessesary, and decreases code-size with 140Kb */
#define GTK_NO_CHECK_CASTS
#include "pgtk_config.h"
#include "program.h"
#include "pike_types.h"
#include "interpret.h"
#include "module_support.h"
#include "array.h"
#include "backend.h"
#include "stralloc.h"
#include "mapping.h"
#include "object.h"
#include "bignum.h"
#include "pike_threads.h"
#include "builtin_functions.h"
#include "operators.h"
#include "sprintf.h"

/*
#include <glib.h>
#include <glib-object.h>
*/
#include <gtk/gtk.h>
#if defined(HAVE_GNOMEUI)
# include <gnome.h>
/*# include <libgnorba/gnorba.h> */
#elif defined(HAVE_GNOME)
# include <libgnome/libgnome.h>
#endif

#ifdef HAVE_GNOMECANVAS
#include <libgnomecanvas/libgnomecanvas.h>
#endif

#ifdef HAVE_GTKEXTRA_GTKEXTRA_H
# include <gtkextra/gtkextra.h>
#endif

#include <signal.h>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#if defined(HAVE_GDK_GDKX_H)
#include <gdk/gdkx.h>
#elif defined(HAVE_GDK_GDKWIN32_H)
#include <gdk/gdkwin32.h>
#elif defined(HAVE_GDK_GDKQUARTZ_H)
#include <gdk/gdkquartz.h>
#else
#error Must have one of gdkx.h, gdkwin32.h, or gdkquartz.h available
#endif

/*
#undef GTK_STYLE
#define GTK_STYLE(X) ((GtkStyle *)X)
*/

/*
#undef GTK_ACCEL_GROUP
#define GTK_ACCEL_GROUP(X) ((void *)X)
*/
#include "modules/Image/image.h"
/* #include "image.h" */

struct object_wrapper {
  GObject *obj;
  int extra_int;
  void *extra_data;
  int owned;
};

struct mixin_wrapper {
  ptrdiff_t offset;
};

struct signal_data {
  struct svalue cb;
  struct svalue args;
  int signal_id;
};

struct store_data {
  GType *types;
  int n_cols;
};

struct my_pixel {
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char padding;
};

/* Prototypes.h is generated in the build directory. */
#include "prototypes.h"

extern struct pike_string * pgtk2_pstr_vector[];
extern const char __pgtk2_string_data[];
struct program *pgtk2_type_to_program(GObject *widget);
void pgtk2_pop_n_elems(int n);
#define my_pop_n_elems pgtk2_pop_n_elems
void pgtk2_ref_push_object(struct object *o);
#define my_ref_push_object pgtk2_ref_push_object
void pgtk2_return_this(int n);
void pgtk2_push_atom(GdkAtom a);
#define push_atom pgtk2_push_atom

void pgtk2_verify_setup();
void pgtk2_verify_gnome_setup();
void pgtk2_verify_obj_inited();
void pgtk2_verify_obj_not_inited();
void pgtk2_verify_mixin_inited();
void pgtk2_verify_mixin_not_inited();

void pgtk2_push_Xpseudo32bitstring(void *f, int nelems);
#define push_Xpseudo32bitstring pgtk2_push_Xpseudo32bitstring

int pgtk2_get_color_from_pikecolor(struct object *o, INT_TYPE *r, INT_TYPE *g, INT_TYPE *b);
#define get_color_from_pikecolor pgtk2_get_color_from_pikecolor

/*int pgtk2_signal_func_wrapper(struct signal_data *d, ...); */
void pgtk2_signal_func_wrapper(struct signal_data *d,
			       gpointer go,
			       guint n_params,
			       const GValue *param_values,
			       GValue *return_value);

void pgtk2_free_signal_data(struct signal_data *s, GClosure *gcl);
void pgtk2_free_object(struct object *o);

void pgtk2_push_gdk_event(GdkEvent *e);
#define push_gdk_event pgtk2_push_gdk_event

int pgtk2_buttonfuncwrapper(GObject *obj, struct signal_data *d,  void *foo);
/*
int signal_func_wrapper(GtkObject *obj, struct signal_data *d,
                        int nparams, GValue *params);
*/

#define pgtk2__init_this_object() pgtk2__init_object(Pike_fp->current_object)
void pgtk2__init_object(struct object *o);

void *get_pgdk2object(struct object *from, struct program *type);
#define get_gdkobject(X,Y) (void *)get_pgdk2object(X,pgdk2_##Y##_program)


GObject *get_pg2object(struct object *from, struct program *type);
/*
#define get_pgtkobject(X,Y) get_pg2object(X,Y)
*/

#define get_gobject(from) get_pg2object(from,pg2_object_program)
/*
#define get_gtkobject(from) get_pg2object(from,pg2_object_program)
*/

void pgtk2_push_gobjectclass(void *obj, struct program *def);
#define push_gobjectclass pgtk2_push_gobjectclass
/*
#define push_gtkobjectclass(X,Y) pgtk2_push_gobjectclass(X,Y)
*/
#define push_gobject(o) pgtk2_push_gobjectclass(o,pgtk2_type_to_program((void *)o))
/*
#define push_gtkobject(o) pgtk2_push_gobjectclass(o,pgtk2_type_to_program((void*)o))
*/

void pgtk2_clear_obj_struct(struct object *o);
void pgtk2_setup_mixin(struct object *o, struct program *p);
void pgtk2_default__sprintf(int n, int a, int l);
void pgtk2_get_string_arg_with_sprintf( INT32 args );

void push_pgdk2object(void *obj, struct program *def, int owned);
#define push_gdkobject(X,Y,Z) push_pgdk2object(X,pgdk2_##Y##_program,Z)


GdkImage *pgtk2_gdkimage_from_pikeimage(struct object *img, int fast, GObject **pi);
struct object *pgtk2_pikeimage_from_gdkimage(GdkImage *img);
#define gdkimage_from_pikeimage pgtk2_gdkimage_from_pikeimage
#define pikeimage_from_gdkimage pgtk2_pikeimage_from_gdkimage

#ifdef THIS
#undef THIS
#endif

#ifdef CLASS_TYPE
#undef CLASS_TYPE
#endif

#define OBJ_STORAGE (Pike_fp->current_storage)
#define MIXIN_STORAGE (Pike_fp->current_object->storage + ((struct mixin_wrapper *)OBJ_STORAGE)->offset)

/* Default when CLASS_TYPE is not defined */
#define CLASS_TYPE_STORAGE OBJ_STORAGE
#define STORAGE_FOR(X) PIKE_CONCAT(X,_STORAGE)
#define STORAGE STORAGE_FOR(CLASS_TYPE)


#define THIS ((struct object_wrapper *)STORAGE)
#define THISO ((struct object_wrapper *)STORAGE)->obj

#define MIXIN_THIS ((struct object_wrapper *)MIXIN_STORAGE)

#define pgtk2_verify_MIXIN_inited pgtk2_verify_mixin_inited
#define pgtk2_verify_MIXIN_not_inited pgtk2_verify_mixin_not_inited
#define pgtk2_verify_OBJ_inited pgtk2_verify_obj_inited
#define pgtk2_verify_OBJ_not_inited pgtk2_verify_obj_not_inited

/* Default when CLASS_TYPE is not defined */
#define pgtk2_verify_CLASS_TYPE_inited pgtk2_verify_OBJ_inited
#define pgtk2_verify_CLASS_TYPE_not_inited pgtk2_verify_OBJ_not_inited

#define pgtk2_verify_inited_for(X) PIKE_CONCAT3(pgtk2_verify_,X,_inited)
#define pgtk2_verify_not_inited_for(X) PIKE_CONCAT3(pgtk2_verify_,X,_not_inited)
#define pgtk2_verify_inited pgtk2_verify_inited_for(CLASS_TYPE)
#define pgtk2_verify_not_inited pgtk2_verify_not_inited_for(CLASS_TYPE)

#define RETURN_THIS()  pgtk2_return_this(args)

struct my_pixel pgtk2_pixel_from_xpixel(unsigned int pix, GdkImage *i);
typedef void *Gdk_Atom;
GdkAtom pgtk2_get_gdkatom(struct object *o);
#define get_gdkatom pgtk2_get_gdkatom
void pgtk2_get_mapping_arg(struct mapping *map,
			   char *name, int type, int madd,
			   void *dest, long *mask, int len);

void pgtk2_index_stack(char *with);
void pgtk2_get_image_module();

void pgtk2_encode_truecolor_masks(struct image *i,
				  int bitspp,
				  int pad,
				  int byteorder,
				  unsigned int red_mask,
				  unsigned int green_mask,
				  unsigned int blue_mask,
				  unsigned char *buffer,
				  int debuglen);


#if defined(PGTK_DEBUG) && defined(HAVE_GETHRTIME)
# define TIMER_INIT(X) do { long long cur,last,start; start = gethrtime(); last=start;fprintf(stderr, "%20s ... ",(X))
# define TIMER_END() cur=gethrtime();fprintf(stderr, "%4.1fms (%4.1fms)\n\n",(cur-last)/1000000.0,(cur-start)/1000000.0);} while(0);
# define PFTIME(X) cur=gethrtime();fprintf(stderr, "%4.1fms (%4.1fms)\n%20s ... ",(cur-last)/1000000.0,(cur-start)/1000000.0,(X));last=cur;
# define DEBUG_PF(X) printf X
#else
# define TIMER_INIT(X)
# define PFTIME(X)
# define TIMER_END()
# define DEBUG_PF(X)
#endif

void pgtk2_push_gchar(const gchar *s);
gchar *pgtk2_get_str(struct svalue *sv);
void pgtk2_free_str(gchar *s);
# define PGTK_ISSTR(X) (TYPEOF(*(X)) == PIKE_T_STRING)
# define PGTK_GETSTR(X)  pgtk2_get_str(X)
# define PGTK_FREESTR(X) pgtk2_free_str(X)
# define PGTK_PUSH_GCHAR(X) pgtk2_push_gchar(X)
/*
#else
# define PGTK_ISSTR( X ) ((TYPEOF(*(X)) == PIKE_T_STRING)&&((X)->u.string->size_shift==0))
# define PGTK_GETSTR(X)  ((char*)((X)->u.string->str))
# define PGTK_FREESTR(X)
# define PGTK_PUSH_GCHAR(X) push_text( X )
#endif
*/

#define GSTR0(X) (gchar*)STR0(X)
#define CGSTR0(X) (const gchar*)STR0(X)

/* Somewhat more complex than one could expect. Consider bignums. */
INT64 pgtk2_get_int(struct svalue *s);
int pgtk2_is_int(struct svalue *s);

# define PGTK_ISINT(X)    ((TYPEOF(*(X)) == PIKE_T_INT) || pgtk2_is_int(X))
# define PGTK_GETINT(X)   pgtk2_get_int(X)
# define PGTK_PUSH_INT(X) push_int64((INT64)(X))

/* Can convert from int to float, and, if bignum is present, bignum to
 * float.
 */
double pgtk2_get_float(struct svalue *s);
int pgtk2_is_float(struct svalue *s);
#define PGTK_ISFLT(X) pgtk2_is_float(X)
#define PGTK_GETFLT(X) pgtk2_get_float(X)
int pgtk2_last_event_time();

#define PSTR (char*)__pgtk2_string_data
#define PGTK_CHECK_TYPE(ob,t) (g_type_is_a(G_TYPE_FROM_INSTANCE(ob),(t)))

#define setprop(x,y) g_object_set(THIS->obj,x,y,NULL)
#define getprop(x,y) g_object_get(THIS->obj,x,y,NULL)

void pgtk2_set_property(GObject *g, char *prop, struct svalue *sv);
void pgtk2__low_get_property(GObject *g, char *prop);
void pgtk2_get_property(GObject *g, char *prop);
void pgtk2_destroy_store_data(gpointer data);
void pgtk2_set_gvalue(GValue *gv, GType gt, struct svalue *sv);
int pgtk2_tree_sortable_callback(GtkTreeModel *model, GtkTreeIter *a,
				 GtkTreeIter *b, struct signal_data *d);
GObject *pgtk2_create_new_obj_with_properties(GType type, struct mapping *m);

#define INIT_WITH_PROPS(X) do {	\
	struct mapping *m;	\
        get_all_args(NULL,args,"%m",&m);	\
	THIS->obj=pgtk2_create_new_obj_with_properties(X,m);	\
      } while(0)
void pgtk2_push_gvalue_r(const GValue *param, GType t);
#define push_gvalue_r pgtk2_push_gvalue_r

#ifdef HAVE_CAIRO
/* from picairo */
#include <cairo.h>
struct cairo_mod_context {
  cairo_t *ctx;
};
#endif

void pgtk2_marshaller(GClosure *closure, GValue *return_value,
		      guint n_param_values, const GValue *param_values,
		      gpointer invocation_hint, gpointer marshal_data);

#define pgtk2_set_value(X,Y) pgtk2_set_gvalue((X),G_VALUE_TYPE(X),Y)
#define pgtk2_push_gvalue_rt(X) pgtk2_push_gvalue_r((X),G_VALUE_TYPE(X))

int pgtk2_tree_view_row_separator_func(GtkTreeModel *model,
				       GtkTreeIter *iter,
				       struct signal_data *d);

void pgtk2_get_doc(GObject *o, struct svalue *dest);
