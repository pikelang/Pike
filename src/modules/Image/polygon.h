
#ifdef PCOORD
#undef PCOORD
#endif
#ifdef PFLOAT
#undef PFLOAT
#endif

#define PCOORD struct polygon_coord
#define PFLOAT double

struct polygon_coord { PFLOAT x,y; };

struct polygon
{
   PCOORD min,max;
};


void init_polygon_programs(void);
void exit_polygon(void);
