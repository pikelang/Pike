

/* 
This file is incuded in search.c with the following defines set:

NAME The name of the match function. This is undef:ed at end of this file
INAME The name of the match c-function. This is nudef:ed at end of this file
PIXEL_VALUE_DISTANCE The inner loop code for each pixel. 
                     undef:ed at end of this file
NEEDLEAVRCODE This code calculate needle_average. Not undef:ed at end
NORMCODE code used for normalizing in the haystack. Not undef:ed at end
SCALE_MODIFY(x) This modifies the output in each pixel

 */


void INAME(INT32 args)
{
  struct object *o;
  struct image *img,*needle=0, *haystack_cert=0, *haystack_avoid=0, *haystack=0, 
    *needle_cert=0, *this;
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
  float scale;
  int needle_average=0;
  int needle_size=0;
  
  if (!THIS->img) { error("no image\n");  return; }
  this=THIS;
  haystacki=this->img;
  haystack=this;
  if (!args) { error("Missing arguments to image->"NAME"\n");  return; }
  else if (args<2) { error("To few arguments to image->"NAME"\n");  return; }
  else 
    {
      if (sp[-args].type==T_INT) 
	scale=sp[-args].u.integer;
      else if (sp[-args].type==T_FLOAT)
	scale=sp[-args].u.float_number;
      else
	error("Illegal argument 1 to image->"NAME"\n");

      if ((sp[1-args].type!=T_OBJECT)
	  || !(needle=
	       (struct image*)get_storage(sp[1-args].u.object,image_program)))
	error("Illegal argument 2 to image->"NAME"()\n");

      if ((needle->xsize>haystack->xsize)||
	  (needle->ysize>haystack->ysize))
	error("Haystack must be bigger than needle error in image->"NAME"()\n");
      needlei=needle->img;
      haystacki=haystack->img;

      if ((args==2)||(args==3))
	type=1;
      else
	{
	  if ((sp[2-args].type!=T_OBJECT)|| 
		   !(haystack_cert=
		     (struct image*)get_storage(sp[2-args].u.object,image_program)))
	    error("Illegal argument 3 to image->"NAME"()\n");
	  else
	    if ((haystack->xsize!=haystack_cert->xsize)||
		(haystack->ysize!=haystack_cert->ysize))
	      error("Argument 3 must be the same size as haystack error in image->"NAME"()\n");
	  
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
	    error("Illegal argument 4 to image->"NAME"()\n");
	  else
	    {
	      if ((needle_cert->xsize!=needle->xsize)||
		  (needle_cert->ysize!=needle->ysize))
		error("Needle_cert must be the same size as needle error in image->"NAME"()\n");
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
		error("Illegal argument 6 to image->"NAME"()\n");
	      if ((sp[4-args].type!=T_OBJECT)|| 
		  !(haystack_avoid=
		    (struct image*)get_storage(sp[4-args].u.object,image_program)))
		error("Illegal argument 5 to image->"NAME"()\n");
	      else
		if ((haystack->xsize!=haystack_avoid->xsize)||
		    (haystack->ysize!=haystack_avoid->ysize))
		  error("Haystack_avoid must be the same size as haystack error in image->"NAME"()\n");
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
      NEEDLEAVRCODE

#define DOUBLE_LOOP(IF_SOMETHING, CERTI1, CERTI2,R1,G1,B1)  \
      for(y=0; y<ys; y++) \
	for(x=0; x<xs-nxs; x++) \
	  { \
	    int i=y*this->xsize+x;  \
	    int sum=0; \
            int tempavr=0;\
	    IF_SOMETHING \
	      {\
	    NORMCODE\
	    tempavr=(int)(((float)tempavr)/(3*needle_size)); \
	    for(ny=0; ny<nys; ny++) \
	      for(nx=0; nx<nxs; nx++)  \
		{ \
		  int j=i+ny*xs+nx; \
		  { \
		  int h,n; \
		  PIXEL_VALUE_DISTANCE(r,CERTI1 R1, CERTI2 R1) \
		  PIXEL_VALUE_DISTANCE(g,CERTI1 G1, CERTI2 G1) \
		  PIXEL_VALUE_DISTANCE(b,CERTI1 B1, CERTI2 B1) \
		  }} \
	    imgi[i+(nys/2)*xs+(nxs/2)].r=\
              (int)(255.99/(1+((scale * SCALE_MODIFY(sum))))); \
              }\
	  }

#define IF_AVOID_IS_SMALL_ENOUGH if ((haystack_avoidi[i].r)>(foo)) { int k=0; imgi[k=i+(nys/2)*xs+(nxs/2)].r=0; imgi[k].g=100; imgi[k].b=0; } else
	 /* FIXME imgi[k].g=100; ?!? */

      if (type==1)
	DOUBLE_LOOP(if (1),1,1, , , )
      else if (type==2)
	DOUBLE_LOOP(if (1), haystack_certi[j], needle_certi[ny*nxs+nx],.r,.g,.b)      
      else if (type==3)
	DOUBLE_LOOP(IF_AVOID_IS_SMALL_ENOUGH,1,1, , , )
      else if (type==4)
	DOUBLE_LOOP(IF_AVOID_IS_SMALL_ENOUGH, haystack_certi[j], needle_certi[ny*nxs+nx],.r,.g,.b)


#undef IF_AVOID_IS_SMALL_ENOUGH_THEN
#undef PIXEL_VALUE_DISTANCE
#undef DOUBLE_LOOP
   
THREADS_DISALLOW();
    }
  o->refs++;
  push_object(o);
}
#undef NAME
#undef INAME

