/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/* This file is incuded in search.c with the following defines set:
NEIG                is 1, zx, zx+1 or zx-1
IMAGE_PHASE image_phase(h|v|hv|vh)    Name of the function 
 */

void IMAGE_PHASE(INT32 args)
{
  struct object *o;
  struct image *img,
    *this;
  rgb_group *imgi=0,*thisi=0;

  int y, x; /* for this & img */
  int yz, xz; /* for this & img */
  int ys, xs; /* for this & img */
  
  if (!THIS->img) { Pike_error("no image\n");  return; }
  this=THIS;
  thisi=this->img;

  
  push_int(this->xsize);
  push_int(this->ysize);
  o=clone_object(image_program,2);
  img=(struct image*)get_storage(o,image_program);
  imgi=img->img;

  pop_n_elems(args);

THREADS_ALLOW();  
     xz=this->xsize;
     yz=this->ysize;

     xs=this->xsize-1;
     ys=this->ysize-1;


#define DALOOP(R) \
     for(y=1; y<ys; y++) \
       for(x=1; x<xs; x++) \
	 {\
	   int i;\
	   int V,H;\
	   i=y*xs+x;\
	   V=thisi[i-(NEIG)].R-thisi[i].R;\
	   H=thisi[i+(NEIG)].R-thisi[i].R;\
	   if ((V==0)&&(H==0))\
	     {\
	       /*\
		 In this case a check to see what there are at the sides \
		 should be done\
	       */\
	       imgi[i].R=0;\
	     }\
	   else \
	     {\
	       if (V==0) imgi[i].R=32;\
	       else if (H==0) imgi[i].R=256-32;\
	       else\
		 {\
		   if (abs(V)>abs(H))\
		     if (V<0)\
		       imgi[i].R=DOUBLE_TO_CHAR(0.5+224+(((float)H)/\
						         (0-V))*32.0);\
		     else\
		       imgi[i].R=DOUBLE_TO_CHAR(0.5+96+(((float)H)/\
					                (V))*32.0);\
		   else\
		     if (H<0)\
		       imgi[i].R=DOUBLE_TO_CHAR(0.5+32+(((float)V)/\
					                (0-H))*32.0);\
		     else\
		       imgi[i].R=DOUBLE_TO_CHAR(0.5+160+(((float)V)/\
						         (H))*32.0);\
		 }\
	     }\
	 }

  DALOOP(r)
  DALOOP(g)
  DALOOP(b)

#undef DALOOP

THREADS_DISALLOW();

  ref_push_object(o);
}
