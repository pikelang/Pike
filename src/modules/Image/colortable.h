#define COLORLOOKUPCACHEHASHSIZE 207

typedef unsigned long nct_weight_t;

struct nct_flat_entry /* flat colorentry */
{
   rgb_group color;
   nct_weight_t weight;
   signed long no;
};

struct nct_scale
{
   struct nct_scale *next;
   rgb_group low,high;
   rgbl_group vector; /* high-low */
   float invsqvector; /* |vector|² */
   INT32 realsteps;
   int steps;
   float mqsteps;     /* 1.0/(steps-1) */
   int no[1];  /* actually no[steps] */
};

struct neo_colortable
{
   enum nct_type 
   { 
      NCT_NONE, /* no colors */
      NCT_FLAT, /* flat with weight */
      NCT_CUBE  /* cube with additions */
   } type;
   enum nct_lookup_mode /* see union "lu" below */
   {
      NCT_TREE, /* tree lookup */
      NCT_CUBICLES, /* cubicle lookup */
      NCT_FULL /* scan all values */
   } lookup_mode;

   union
   {
      struct nct_flat
      {
	 int numentries;
	 struct nct_flat_entry *entries;
      } flat;
      struct nct_cube
      {
	 nct_weight_t weight;
	 int r,g,b; /* steps of sides */
	 struct nct_scale *firstscale;
	 INT32 disttrig; /* (sq) distance trigger */
	 int numentries;
      } cube;
   } u;

   rgbl_group spacefactor; 
      /* rgb space factors, normally 2,3,1 */

   struct lookupcache
   {
      rgb_group src;
      rgb_group dest;
      int index;
   } lookupcachehash[COLORLOOKUPCACHEHASHSIZE];

   union /* of pointers!! */
   {
      struct nctlu_cubicles
      {
	 int r,g,b; /* size */
	 int accur; /* accuracy, default 2 */
	 struct nctlu_cubicle
	 {
	    int n; 
	    int *index; /* NULL if not initiated */
	 } *cubicles; /* [r*g*b], index as [ri+(gi+bi*g)*r] */
      } cubicles;
      struct nctlu_tree
      {
	 struct nctlu_treenode
	 {
	    int splitvalue;
	    enum { SPLIT_R,SPLIT_G,SPLIT_B,SPLIT_DONE } split_direction;
	    int less,more;
	 } *nodes; /* shoule be colors×2 */
      } tree;
   } lu;
};


/* exported methods */

void image_colortable_get_index_line(struct neo_colortable *nct,
				     rgb_group *s,
				     unsigned char *buf,
				     int len);

int image_colortable_size(struct neo_colortable *nct);

void image_colortable_write_rgb(struct neo_colortable *nct,
				unsigned char *dest);
