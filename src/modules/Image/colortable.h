/*
**! module Image
**! note
**!	$Id: colortable.h,v 1.11 1998/01/08 16:57:05 mirar Exp $
*/

#ifdef PIKE_IMAGE_COLORTABLE_H
#error colortable.h included twice
#endif

#define PIKE_IMAGE_COLORTABLE_H

#ifndef PIKE_IMAGE_IMAGE_H
#error colortable.h needs image.h
#endif /* !PIKE_IMAGE_IMAGE_H */




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

   union 
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
   } lu;

   enum nct_dither_type
   {
      NCTD_NONE,
      NCTD_FLOYD_STEINBERG,
      NCTD_RANDOMCUBE,
      NCTD_RANDOMGREY,
      NCTD_ORDERED
   } dither_type;

   union
   {
      struct 
      {
	 float downback;
	 float down;
         float downforward;
	 float forward;
	 int dir;
      } floyd_steinberg;
      struct nctd_randomcube
      {
	 int r,g,b;
      } randomcube;
      struct nctd_ordered
      {
	 int xs,ys;
	 int *rdiff,*gdiff,*bdiff;
	 int rx,ry,gx,gy,bx,by;
	 int row;
      } ordered;
   } du;
};

struct nct_dither;

typedef rgbl_group nct_dither_encode_function(struct nct_dither *dith,
					      int rowpos,
					      rgb_group s);

typedef void nct_dither_got_function(struct nct_dither *dith,
				     int rowpos,
				     rgb_group s,
				     rgb_group d);

typedef void nct_dither_line_function(struct nct_dither *dith,
				      int *rowpos,
				      rgb_group **s,
				      rgb_group **drgb,
				      unsigned char **d8bit,
				      unsigned short **d16bit,
				      long **d32bit,
				      int *cd);

struct nct_dither
{
   enum nct_dither_type type;
   nct_dither_encode_function *encode;
   nct_dither_got_function *got; /* got must be set if encode is set */
   nct_dither_line_function *newline;
   nct_dither_line_function *firstline;
   int rowlen;
   union
   {
      struct nct_dither_floyd_steinberg
      {
	 rgbd_group *errors;
	 rgbd_group *nexterrors;
	 float downback;
	 float down;
         float downforward;
	 float forward;
	 int dir;
	 int currentdir;
      } floyd_steinberg;
      struct nctd_randomcube randomcube;
      struct nctd_ordered ordered;
   } u;
};

/* exported methods */

void image_colortable_get_index_line(struct neo_colortable *nct,
				     rgb_group *s,
				     unsigned char *buf,
				     int len,
				     struct nct_dither *dith);

int image_colortable_size(struct neo_colortable *nct);

void image_colortable_write_rgb(struct neo_colortable *nct,
				unsigned char *dest);

int image_colortable_initiate_dither(struct neo_colortable *nct,
/* 0 upon out of memory */	     struct nct_dither *dith,
				     int rowlen);

void image_colortable_free_dither(struct nct_dither *dith);

int image_colortable_index_8bit_image(struct neo_colortable *nct,
				      rgb_group *s,
				      unsigned char *d,
				      int len,
				      int rowlen);

int image_colortable_index_16bit_image(struct neo_colortable *nct,
				      rgb_group *s,
				      unsigned short *d,
				      int len,
				      int rowlen);

void image_colortable_internal_floyd_steinberg(struct neo_colortable *nct);

int image_colortable_map_image(struct neo_colortable *nct,
			       rgb_group *s,
			       rgb_group *d,
			       int len,
			       int rowlen);

void image_colortable_cast_to_array(struct neo_colortable *nct);


