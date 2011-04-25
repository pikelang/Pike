/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/* included w/ defines in colortable.c */

/*
**! module Image
**! class colortable
*/

void NCTLU_FLAT_CUBICLES_NAME(rgb_group *s,
			      NCTLU_DESTINATION *d,
			      int n,
			      struct neo_colortable *nct,
			      struct nct_dither *dith,
			      int rowlen)
{
   struct nctlu_cubicles *cubs;
   struct nctlu_cubicle *cub;
   int red,green,blue;
   int redm,greenm,bluem;
   int redgreen;
   struct nct_flat_entry *fe=nct->u.flat.entries;
   int mindist;
   rgbl_group sf=nct->spacefactor;

   nct_dither_encode_function *dither_encode=dith->encode;
   nct_dither_got_function *dither_got=dith->got;
   nct_dither_line_function *dither_newline=dith->newline;
   int rowpos=0,cd=1,rowcount=0;

   cubs=&(nct->lu.cubicles);
   if (!(cubs->cubicles))
   {
      int n2=cubs->r*cubs->g*cubs->b;

CHRONO("init flat/cubicles");

      cub=cubs->cubicles=malloc(sizeof(struct nctlu_cubicle)*n2);
      
      if (!cub) Pike_error("out of memory\n");

      while (n2--) /* initiate all to empty */
      {
	 cub->n=0;
	 cub->index=NULL;
	 cub++;
      } /* yes, could be faster with a memset... */
   }

CHRONO("begin flat/cubicles");

   red=cubs->r;   redm=red-1;
   green=cubs->g; greenm=green-1;
   blue=cubs->b;  bluem=blue-1;
   redgreen=red*green;

   if (dith->firstline)
      (dith->firstline)NCTLU_LINE_ARGS;

   while (n--)
   {
      int r,g,b;
      struct lookupcache *lc;
      int m;
      int *ci;
      rgbl_group val;

      if (dither_encode)
      {
	 val=(dither_encode)(dith,rowpos,*s);
      }
      else
      {
	 val.r=s->r;
	 val.g=s->g;
	 val.b=s->b;
      }

      lc=nct->lookupcachehash+COLORLOOKUPCACHEHASHVALUE(val.r,val.g,val.b);
      if (lc->index!=-1 &&
	  lc->src.r==val.r &&
	  lc->src.g==val.g &&
	  lc->src.b==val.b)
      {
	 NCTLU_CACHE_HIT_WRITE;
	 goto done_pixel;
      }

      lc->src=*s;
      
      r=((val.r*red+redm)>>8);
      g=((val.g*green+greenm)>>8);
      b=((val.b*blue+bluem)>>8);

      cub=cubs->cubicles+r+g*red+b*redgreen;
      
      if (!cub->index) /* need to build that cubicle */
	 _build_cubicle(nct,r,g,b,red,green,blue,cub);

      /* now, compare with the colors in that cubicle */

      m=cub->n;
      ci=cub->index;

      mindist=256*256*100; /* max dist is 256²*3 */
      
      while (m--)
      {
	 int dist=sf.r*SQ(fe[*ci].color.r-val.r)+
	          sf.g*SQ(fe[*ci].color.g-val.g)+
	          sf.b*SQ(fe[*ci].color.b-val.b);
	 
	 if (dist<mindist)
	 {
	    lc->dest=fe[*ci].color;
	    mindist=dist;
	    lc->index=*ci;
	    NCTLU_CACHE_HIT_WRITE;
	 }
	 
	 ci++;
      }

done_pixel:
      if (dither_encode)
      {
         if (dither_got)
	   dither_got(dith,rowpos,*s,NCTLU_DITHER_GOT);
	 s+=cd; d+=cd; rowpos+=cd;
	 if (++rowcount==rowlen)
	 {
	    rowcount=0;
	    if (dither_newline)
	       (dither_newline)NCTLU_LINE_ARGS;
	 }
      }
      else
      {
	 d++;
	 s++;
      }
   }

CHRONO("end flat/cubicles");
}

void NCTLU_FLAT_FULL_NAME(rgb_group *s,
			  NCTLU_DESTINATION *d,
			  int n,
			  struct neo_colortable *nct,
			  struct nct_dither *dith,
			  int rowlen)
{
   /* no need to build any data, we're using full scan */

   rgbl_group sf=nct->spacefactor;
   ptrdiff_t mprim=nct->u.flat.numentries;
   struct nct_flat_entry *feprim=nct->u.flat.entries;

   nct_dither_encode_function *dither_encode=dith->encode;
   nct_dither_got_function *dither_got=dith->got;
   nct_dither_line_function *dither_newline=dith->newline;
   int rowpos=0,cd=1,rowcount=0;

   CHRONO("begin flat/full map");

   if (dith->firstline)
      (dith->firstline)NCTLU_LINE_ARGS;

   while (n--)
   {
      int rgbr,rgbg,rgbb;
      int mindist;
      ptrdiff_t m;
      struct nct_flat_entry *fe;
      struct lookupcache *lc;
	 
      if (dither_encode)
      {
	 rgbl_group val;
	 val=dither_encode(dith,rowpos,*s);
	 rgbr=val.r;
	 rgbg=val.g;
	 rgbb=val.b;
      }
      else
      {
	 rgbr=s->r;
	 rgbg=s->g;
	 rgbb=s->b;
      }

      /* cached? */
      lc=nct->lookupcachehash+COLORLOOKUPCACHEHASHVALUE(rgbr,rgbg,rgbb);
      if (lc->index!=-1 &&
	  lc->src.r==rgbr &&
	  lc->src.g==rgbg &&
	  lc->src.b==rgbb)
      {
	 NCTLU_CACHE_HIT_WRITE;
	 goto done_pixel;
      }

      lc->src=*s;

      mindist=256*256*100; /* max dist is 256²*3 */
      
      fe=feprim;
      m=mprim;
      
      while (m--)
	 if (fe->no!=-1)
	 {
	    int dist=
	       sf.r*SQ(fe->color.r-rgbr)+
	       sf.g*SQ(fe->color.g-rgbg)+
	       sf.b*SQ(fe->color.b-rgbb);
	 
	    if (dist<mindist)
	    {
	       lc->dest=fe->color;
	       mindist=dist;
	       lc->index = DO_NOT_WARN((int)fe->no);
	       NCTLU_CACHE_HIT_WRITE;
	    }
	 
	    fe++;
	 }
	 else fe++;

done_pixel:
      if (dither_encode)
      {
         if (dither_got)
	   dither_got(dith,rowpos,*s,NCTLU_DITHER_GOT);
	 s+=cd; d+=cd; rowpos+=cd;
	 if (++rowcount==rowlen)
	 {
	    rowcount=0;
	    if (dither_newline) 
	       (dither_newline)NCTLU_LINE_ARGS;
	 }
      }
      else
      {
	 d++;
	 s++;
      }
   }

   CHRONO("end flat/full map");
}

void NCTLU_FLAT_RIGID_NAME(rgb_group *s,
			   NCTLU_DESTINATION *d,
			   int n,
			   struct neo_colortable *nct,
			   struct nct_dither *dith,
			   int rowlen)
{
   ptrdiff_t mprim = nct->u.flat.numentries;
   struct nct_flat_entry *feprim = nct->u.flat.entries;

   nct_dither_encode_function *dither_encode=dith->encode;
   nct_dither_got_function *dither_got=dith->got;
   nct_dither_line_function *dither_newline=dith->newline;
   int rowpos=0,cd=1,rowcount=0;
   int *index;
   int r,g,b;
   int i;

   if (!nct->lu.rigid.index)
   {
      CHRONO("init flat/rigid map");
      build_rigid(nct);
   }
   index=nct->lu.rigid.index;
   r=nct->lu.rigid.r;
   g=nct->lu.rigid.g;
   b=nct->lu.rigid.b;

   CHRONO("begin flat/rigid map");

   if (dith->firstline)
      (dith->firstline)NCTLU_LINE_ARGS;

   while (n--)
   {
      rgbl_group val;
	 
      if (dither_encode)
      {
	 val=dither_encode(dith,rowpos,*s);
      }
      else
      {
	 val.r=s->r;
	 val.g=s->g;
	 val.b=s->b;
      }

      i=index[((val.r*r)>>8)+
	     r*(((val.g*g)>>8)+
		((val.b*b)>>8)*g)];
      NCTLU_RIGID_WRITE;

      if (dither_encode)
      {
        if (dither_got)
          dither_got(dith,rowpos,*s,NCTLU_DITHER_RIGID_GOT);
        s+=cd; d+=cd; rowpos+=cd;
        if (++rowcount==rowlen)
        {
          rowcount=0;
          if (dither_newline) 
            (dither_newline)NCTLU_LINE_ARGS;
        }
      }
      else
      {
	 d++;
	 s++;
      }
   }

   CHRONO("end flat/rigid map");
}

void NCTLU_CUBE_NAME(rgb_group *s,
		     NCTLU_DESTINATION *d,
		     int n,
		     struct neo_colortable *nct,
		     struct nct_dither *dith,
		     int rowlen)
{
   int red,green,blue;
   int hred,hgreen,hblue;
   int redm,greenm,bluem;
   float redf,greenf,bluef;
   struct nct_cube *cube=&(nct->u.cube);
   rgbl_group sf=nct->spacefactor;
   int rowpos=0,cd=1,rowcount=0;
   rgbl_group rgb;
   nct_dither_encode_function *dither_encode=dith->encode;
   nct_dither_got_function *dither_got=dith->got;
   nct_dither_line_function *dither_newline=dith->newline;

   red=cube->r; 	hred=red/2;      redm=red-1;
   green=cube->g;	hgreen=green/2;  greenm=green-1;
   blue=cube->b; 	hblue=blue/2;    bluem=blue-1;

   redf = DO_NOT_WARN((float)(255.0/redm));
   greenf = DO_NOT_WARN((float)(255.0/greenm));
   bluef = DO_NOT_WARN((float)(255.0/bluem));

   CHRONO("begin cube map");

   if (!cube->firstscale && red && green && blue)
   {
      if (!dither_encode)
	 while (n--)
	 {
	    NCTLU_CUBE_FAST_WRITE(s);
	    d++;
	    s++;
	 }
      else
      {
	 if (dith->firstline)
	    (dith->firstline)NCTLU_LINE_ARGS;
	 while (n--)
	 {
	    rgb=dither_encode(dith,rowpos,*s);
	    NCTLU_CUBE_FAST_WRITE(&rgb);
	    if (dither_got)
	       NCTLU_CUBE_FAST_WRITE_DITHER_GOT(&rgb);
	    s+=cd; d+=cd; rowpos+=cd;
	    if (++rowcount==rowlen)
	    {
	       rowcount=0;
	       if (dither_newline) 
		  (dither_newline)NCTLU_LINE_ARGS;
	    }
	 }
      }
   }
   else
   {
      if (dith->firstline)
	 (dith->firstline)NCTLU_LINE_ARGS;

      while (n--) /* similar to _find_cube_dist() */
      {
	 struct nct_scale *sc;
	 int mindist;
	 int i;
	 int nc;
	 struct lookupcache *lc;
	 rgbl_group val;

	 if (dither_encode)
	 {
	    val=dither_encode(dith,rowpos,*s);
	 }
	 else
	 {
	    val.r=s->r;
	    val.g=s->g;
	    val.b=s->b;
	 }

	 lc=nct->lookupcachehash+COLORLOOKUPCACHEHASHVALUE(val.r,val.g,val.b);
	 if (lc->index!=-1 &&
	     lc->src.r==val.r &&
	     lc->src.g==val.g &&
	     lc->src.b==val.b)
	 {
	    NCTLU_CACHE_HIT_WRITE;
	    goto done_pixel;
	 }

	 lc->src=*s;

	 if (red && green && blue)
	 {
	    lc->dest.r=((int)(((val.r*red+hred)>>8)*redf));
	    lc->dest.g=((int)(((val.g*green+hgreen)>>8)*greenf));
	    lc->dest.b=((int)(((val.b*blue+hblue)>>8)*bluef));

	    lc->index=i=
	       ((val.r*red+hred)>>8)+
	       (((val.g*green+hgreen)>>8)+
		((val.b*blue+hblue)>>8)*green)*red;

	    NCTLU_CACHE_HIT_WRITE;

	    mindist=
	       sf.r*SQ(val.r-lc->dest.r)+
	       sf.g*SQ(val.g-lc->dest.g)+
	       sf.b*SQ(val.b-lc->dest.b);
	 }
	 else
	 {
	    mindist=10000000;
	    i=0;
	 }

	 if (mindist>=cube->disttrig)
	 {
	    /* check scales to get better distance if possible */

	    nc=cube->r*cube->g*cube->b;
	    sc=cube->firstscale;
	    while (sc)
	    {
	       /* what step is closest? project... */

	      i = DOUBLE_TO_INT((sc->steps *
				 (((int)val.r-(int)sc->low.r)*sc->vector.r +
				  ((int)val.g-(int)sc->low.g)*sc->vector.g +
				  ((int)val.b-(int)sc->low.b)*sc->vector.b)) *
				sc->invsqvector);

	       if (i<0) i=0; else if (i>=sc->steps) i=sc->steps-1;
	       if (sc->no[i]>=nc) 
	       {
		  double f= i * sc->mqsteps;
		  int drgbr = sc->low.r + DOUBLE_TO_INT(sc->vector.r*f);
		  int drgbg = sc->low.g + DOUBLE_TO_INT(sc->vector.g*f);
		  int drgbb = sc->low.b + DOUBLE_TO_INT(sc->vector.b*f);

		  int ldist=sf.r*SQ(val.r-drgbr)+
		     sf.g*SQ(val.g-drgbg)+sf.b*SQ(val.b-drgbb);

		  if (ldist<mindist)
		  {
		     lc->dest.r=(unsigned char)drgbr;
		     lc->dest.g=(unsigned char)drgbg;
		     lc->dest.b=(unsigned char)drgbb;
		     lc->index=sc->no[i];
		     mindist=ldist;
		     NCTLU_CACHE_HIT_WRITE;
		  }
	       }
	    
	       nc+=sc->realsteps;
	    
	       sc=sc->next;
	    }
	 }
done_pixel:
         if (dither_encode)
         {
            if (dither_got)
	       dither_got(dith,rowpos,*s,NCTLU_DITHER_GOT);
	    s+=cd; d+=cd; rowpos+=cd;
	    if (++rowcount==rowlen)
	    {
	       rowcount=0;
	       if (dither_newline)
		  (dither_newline)NCTLU_LINE_ARGS;
	    }
	 }
	 else
	 {
	    d++;
	    s++;
	 }
      }
   }
   CHRONO("end cube map");
}

void (*NCTLU_SELECT_FUNCTION(struct neo_colortable *nct))
   (rgb_group *s,
    NCTLU_DESTINATION *d,
    int n,
    struct neo_colortable *nct,
    struct nct_dither *dith,
    int rowlen)
{
   switch (nct->type)
   {
      case NCT_CUBE: 
#ifdef COLORTABLE_DEBUG
	 fprintf(stderr,
		 "COLORTABLE " DEFINETOSTR(NCTLU_SELECT_FUNCTION) ":"
		 DEFINETOSTR(NCTLU_DESTINATION) " => "
		 DEFINETOSTR(NCTLU_CUBE_NAME) "\n");
#endif /* COLORTABLE_DEBUG */
	 return &NCTLU_CUBE_NAME;
      case NCT_FLAT:
         switch (nct->lookup_mode)
	 {
	    case NCT_FULL:
#ifdef COLORTABLE_DEBUG
	      fprintf(stderr,
		      "COLORTABLE " DEFINETOSTR(NCTLU_SELECT_FUNCTION) ":"
		      DEFINETOSTR(NCTLU_DESTINATION) " => "
		      DEFINETOSTR(NCTLU_FLAT_FULL_NAME) "\n");
#endif /* COLORTABLE_DEBUG */
	       return &NCTLU_FLAT_FULL_NAME;
	    case NCT_RIGID:
#ifdef COLORTABLE_DEBUG
	      fprintf(stderr,
		      "COLORTABLE " DEFINETOSTR(NCTLU_SELECT_FUNCTION) ":"
		      DEFINETOSTR(NCTLU_DESTINATION) " => "
		      DEFINETOSTR(NCTLU_FLAT_RIGID_NAME) "\n");
#endif /* COLORTABLE_DEBUG */
	       return &NCTLU_FLAT_RIGID_NAME;
	    case NCT_CUBICLES:
#ifdef COLORTABLE_DEBUG
	      fprintf(stderr,
		      "COLORTABLE " DEFINETOSTR(NCTLU_SELECT_FUNCTION) ":"
		      DEFINETOSTR(NCTLU_DESTINATION) " => "
		      DEFINETOSTR(NCTLU_FLAT_CUBICLES_NAME) "\n");
#endif /* COLORTABLE_DEBUG */
	       return &NCTLU_FLAT_CUBICLES_NAME;
	 }
      default:
	 Pike_fatal("lookup select (%s:%d) couldn't find the lookup mode\n",
	       __FILE__,__LINE__);
   }
   /* NOT_REACHED */
   return 0;
}

int NCTLU_EXECUTE_FUNCTION(struct neo_colortable *nct,
			   rgb_group *s,
			   NCTLU_DESTINATION *d,
			   int len,
			   int rowlen)
{
   struct nct_dither dith;

   if (nct->type==NCT_NONE) return 0;

   image_colortable_initiate_dither(nct,&dith,rowlen);
   (NCTLU_SELECT_FUNCTION(nct))(s,d,len,nct,&dith,rowlen);
   image_colortable_free_dither(&dith);

   return 1;
}
