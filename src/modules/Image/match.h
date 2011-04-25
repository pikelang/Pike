/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/* 
This file is incuded in search.c with the following defines set:

NAME The name of the match function. This is undef:ed at end of this file
INAME The name of the match c-function. This is nudef:ed at end of this file
PIXEL_VALUE_DISTANCE The inner loop code for each pixel. 
                     undef:ed at end of this file
NEEDLEAVRCODE If this is set, needle_average is calculated. 
              Not undef:ed at end
NORMCODE code used for normalizing in the haystack. Not undef:ed at end
SCALE_MODIFY(x) This modifies the output in each pixel

 */


void INAME(INT32 args)
{
  struct object *o;
  struct image *img,*needle=0, *haystack_cert=0, *haystack_avoid=0, 
    *haystack=0, *needle_cert=0, *this;
  rgb_group *imgi=0, *needlei=0, *haystack_certi=0, *haystack_avoidi=0, 
    *haystacki=0, *needle_certi=0, *thisi=0;

  int type=0; /* type==1 : (int|float scale, needle)
		 type==2 : (int|float scale, needle, 
                            haystack_cert, needle_cert)
		 type==3 : (int|float scale, needle, haystack_avoid, int foo)
		 type==4 : (int|float scale, needle, 
                            haystack_cert, needle_cert, 
		            haystack_avoid, int foo) */

  int xs,ys, y, x; /* for this & img */
  int nxs,nys, ny, nx; /* for neddle */
  int foo=0;
  double scale = 1.0;
  int needle_average=0;
  int needle_size=0;
  
  if (!THIS->img) { Pike_error("no image\n");  return; }
  this=THIS;
  haystacki=this->img;
  haystack=this;
  if (!args) { Pike_error("Missing arguments to image->"NAME"\n");  return; }
  else if (args<2) { Pike_error("Too few arguments to image->"NAME"\n");  return; }
  else 
    {
      if (sp[-args].type==T_INT) 
	scale = (double)sp[-args].u.integer;
      else if (sp[-args].type==T_FLOAT)
	scale = sp[-args].u.float_number;
      else
	Pike_error("Illegal argument 1 to image->"NAME"\n");

      if ((sp[1-args].type!=T_OBJECT)
	  || !(needle=
	       (struct image*)get_storage(sp[1-args].u.object,image_program)))
	Pike_error("Illegal argument 2 to image->"NAME"()\n");

      if ((needle->xsize>haystack->xsize)||
	  (needle->ysize>haystack->ysize))
	Pike_error("Haystack must be bigger than needle, error in image->"NAME"()\n");
      needlei=needle->img;
      haystacki=haystack->img;

      if ((args==2)||(args==3))
	type=1;
      else
	{
	  if ((sp[2-args].type!=T_OBJECT)|| 
		   !(haystack_cert=
		     (struct image*)get_storage(sp[2-args].u.object,image_program)))
	    Pike_error("Illegal argument 3 to image->"NAME"()\n");
	  else
	    if ((haystack->xsize!=haystack_cert->xsize)||
		(haystack->ysize!=haystack_cert->ysize))
	      Pike_error("Argument 3 must be the same size as haystack error in image->"NAME"()\n");
	  
	  if ((sp[3-args].type==T_INT))
	    {
	      foo=sp[3-args].u.integer;
	      type=3;
	      haystack_avoid=haystack_cert;
	      haystack_cert=0;
	    }
	  else if ((sp[3-args].type!=T_OBJECT)|| 
		   !(needle_cert=
		     (struct image*)get_storage(sp[3-args].u.object,image_program)))
	    Pike_error("Illegal argument 4 to image->"NAME"()\n");
	  else
	    {
	      if ((needle_cert->xsize!=needle->xsize)||
		  (needle_cert->ysize!=needle->ysize))
		Pike_error("Needle_cert must be the same size as needle error in image->"NAME"()\n");
	      type=2;
	    }
	  if (args>=6)
	    {
	      if (sp[5-args].type==T_INT)
		{
		  foo=sp[5-args].u.integer;
		  type=4;
		}
	      else 
		Pike_error("Illegal argument 6 to image->"NAME"()\n");
	      if ((sp[4-args].type!=T_OBJECT)|| 
		  !(haystack_avoid=
		    (struct image*)get_storage(sp[4-args].u.object,image_program)))
		Pike_error("Illegal argument 5 to image->"NAME"()\n");
	      else
		if ((haystack->xsize!=haystack_avoid->xsize)||
		    (haystack->ysize!=haystack_avoid->ysize))
		  Pike_error("Haystack_avoid must be the same size as haystack error in image->"NAME"()\n");
	    }
	}
      push_int(this->xsize);
      push_int(this->ysize);
      o=clone_object(image_program,2);
      img=(struct image*)get_storage(o,image_program);
      imgi=img->img;


      pop_n_elems(args);	  
      

      if (haystack_cert)
	haystack_certi=haystack_cert->img;
      if (haystack_avoid)
	haystack_avoidi=haystack_avoid->img;
      if (needle_cert)
	needle_certi=needle_cert->img;

THREADS_ALLOW();  
      nxs=needle->xsize;
      nys=needle->ysize;
      xs=this->xsize;
      ys=this->ysize-nys;

      /* This sets needle_average to something nice :-) */ 
      /* match and match_phase don't use this */
#ifdef NEEDLEAVRCODE
      needle_size=nxs*nys;
      for(x=0; x<needle_size; x++)
	needle_average+=needlei[x].r+needlei[x].g+needlei[x].b;
      needle_average=(int)(((float)needle_average)/(3*needle_size));

#define NORMCODE for(ny=0; ny<nys; ny++) \
	       for(nx=0; nx<nxs; nx++)  \
		 { \
		   int j=i+ny*xs+nx; \
		   tempavr+=haystacki[j].r+haystacki[j].g+ \
		       haystacki[j].b; \
                 }
#else
#define NORMCODE
#endif

#define DOUBLE_LOOP(AVOID_IS_TOO_BIG, CERTI1, CERTI2,R1,G1,B1)  \
  for(y=0; y<ys; y++) \
    for(x=0; x<xs-nxs; x++) \
    { \
      int i=y*this->xsize+x;  \
      int sum=0; \
      int tempavr=0;\
      if (AVOID_IS_TOO_BIG) \
      {\
        int k=0; \
        imgi[k=i+(nys/2)*xs+(nxs/2)].r=0;\
        imgi[k].g=100; imgi[k].b=0;\
      }\
      else\
      {\
	NORMCODE;\
	tempavr = DOUBLE_TO_INT(((double)tempavr)/(3*needle_size)); \
	for(ny=0; ny<nys; ny++) \
	  for(nx=0; nx<nxs; nx++)  \
	  { \
	    int j=i+ny*xs+nx; \
            int h=0;\
            int n=0;\
	    sum+=(MAXIMUM(CERTI1 R1, CERTI1 R1) * PIXEL_VALUE_DISTANCE(r)); \
	    sum+=(MAXIMUM(CERTI1 G1, CERTI1 G1) * PIXEL_VALUE_DISTANCE(g)); \
	    sum+=(MAXIMUM(CERTI1 B1, CERTI1 B1) * PIXEL_VALUE_DISTANCE(b)); \
	  } \
	imgi[i+(nys/2)*xs+(nxs/2)].r=\
          DOUBLE_TO_INT(255.99/(1.0+((((double)scale) * SCALE_MODIFY((double)sum))))); \
      }\
   }


#define AVOID_IS_TOO_BIG ((haystack_avoidi[i].r)>(foo)) 

      if (type==1)
	DOUBLE_LOOP(0,1,1, *1, *1, *1)
      else if (type==2)
	DOUBLE_LOOP(0, haystack_certi[j], needle_certi[ny*nxs+nx],.r,.g,.b)
      else if (type==3)
	DOUBLE_LOOP(AVOID_IS_TOO_BIG,1,1, *1, *1, *1)
      else if (type==4)
	DOUBLE_LOOP(AVOID_IS_TOO_BIG, haystack_certi[j], needle_certi[ny*nxs+nx],.r,.g,.b)

#undef NORMCODE
#undef AVOID_IS_TOO_BIG
#undef PIXEL_VALUE_DISTANCE
#undef DOUBLE_LOOP
   
THREADS_DISALLOW();
    }
  push_object(o);
}
#undef NAME
#undef INAME
