
#ifdef PCOORD
#undef PCOORD
#endif
#ifdef PFLOAT
#undef PFLOAT
#endif

#define PCOORD struct polygon_coord
#define PFLOAT double

struct polygon_coord { PFLOAT x,y; };

struct line_list
{
   struct vertex *above,*below;
   float dx,dy;
   struct line_list *next;
   float xmin,xmax,yxmin,yxmax; /* temporary storage */
};

struct vertex
{
   PCOORD c;
   struct vertex *next;    /* total list, sorted downwards */
   struct line_list *below,*above; /* childs */
   int done;
};

#define VY(V,X) ((V)->above->y+(V)->dy*((X)-(V)->above->x))

struct polygon
{
   PCOORD min,max;
   struct vertex *top;
};


void init_polygon_programs(void);
void exit_polygon(void);
