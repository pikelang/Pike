struct object_wrapper 
{
  GtkObject *obj;
  int extra_int;
  void *extra_data;
};


struct signal_data
{
  struct svalue cb;
  struct svalue args;
};

struct my_pixel 
{
  unsigned char r;
  unsigned char g;
  unsigned char b;
};

extern int pigtk_is_setup;

void my_pop_n_elems( int n );
void my_ref_push_object( struct object *o );

int get_color_from_pikecolor( struct object *o, int *r, int *g, int *b );

int pgtk_signal_func_wrapper(GtkObject *obj,struct signal_data *d,
                             int nparams, GtkArg *params);
void pgtk_free_signal_data( struct signal_data *s);

void push_gdk_event(GdkEvent *e);

int pgtkbuttonfuncwrapper(GtkObject *obj, struct signal_data *d,  void *foo);
void *get_swapped_string(struct pike_string *s, int force_wide);
int signal_func_wrapper(GtkObject *obj, struct signal_data *d, 
                        int nparams, GtkArg *params);

#define pgtk__init_this_object() pgtk__init_object(fp->current_object)
void pgtk__init_object( struct object *o );

void *get_pgdkobject(struct object *from, struct program *type);
#define get_gdkobject( X, Y ) \
              (Gdk##Y *)get_pgdkobject( X, pgtk_Gdk##Y##_program )


GtkObject *get_pgtkobject(struct object *from, struct program *type);
#define get_gtkobject( from ) get_pgtkobject( from, pgtk_object_program )


void push_gtkobjectclass(void *obj, struct program *def);
#define push_gtkobject( o ) push_gtkobjectclass(o,pgtk_object_program)

void push_pgdkobject(void *obj, struct program *def);
#define push_gdkobject( X, Y ) push_pgdkobject( X, pgtk_Gdk##Y##_program )


GdkImage *gdkimage_from_pikeimage(struct object *img, int fast, GdkImage *i );
struct object *pikeimage_from_gdkimage( GdkImage *img );

#define THIS ((struct object_wrapper *)fp->current_storage)
#define THISO ((struct object_wrapper *)fp->current_storage)->obj

#define GTK_ACCEL_GROUP(X) ((void *)X)
#define GTK_STYLE(X) ((void *)X)

#define RETURN_THIS()   do{			\
  my_pop_n_elems(args);				\
  my_ref_push_object( fp->current_object );	\
} while(0)

struct my_pixel pgtk_pixel_from_xpixel( unsigned int pix, GdkImage *i );
typedef void *Gdk_Atom;
GdkAtom get_gdkatom( struct object *o );
void pgtk_get_mapping_arg( struct mapping *map,
                           char *name, int type, int madd,
                           void *dest, long *mask, int len );

void pgtk_index_stack( char *with );
void pgtk_get_image_module();


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

