/* $Id: colortable_lookup.h,v 1.4 1997/11/07 06:06:08 mirar Exp $ */
/* included w/ defines in colortable.c */

/*
**! module Image
**! note
**!	$Id: colortable_lookup.h,v 1.4 1997/11/07 06:06:08 mirar Exp $
**! class colortable
*/



static void NCTLU_FLAT_CUBICLES_NAME(rgb_group *s,
				     NCTLU_DESTINATION *d,
				     int n,
				     struct neo_colortable *nct,
				     struct nct_dither *dith,
				     int rowlen)
{
   struct nctlu_cubicles *cubs;
   struct nctlu_cubicle *cub;
   int red,green,blue;
   int hred,hgreen,hblue;
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
      
      if (!cub) error("out of memory\n");

      while (n2--) /* initiate all to empty */
      {
	 cub->n=0;
	 cub->index=NULL;
	 cub++;
      } /* yes, could be faster with a memset... */
   }

CHRONO("begin flat/cubicles");

   red=cubs->r;   hred=red/2;
   green=cubs->g; hgreen=green/2;
   blue=cubs->b;  hblue=blue/2;
   redgreen=red*green;

   if (dith->firstline)
      (dith->firstline)NCTLU_LINE_ARGS;

   while (n--)
   {
      int rgbr,rgbg,rgbb;
      int r,g,b;
      struct lookupcache *lc;
      int m;
      int *ci;

      if (dither_encode)
      {
	 rgbl_group val;
	 val=(dither_encode)(dith,rowpos,*s);
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
      
      r=((rgbr*red+hred)>>8);
      g=((rgbg*green+hgreen)>>8);
      b=((rgbb*blue+hblue)>>8);

      cub=cubs->cubicles+r+g*red+b*redgreen;
      
      if (!cub->index) /* need to build that cubicle */
	 _build_cubicle(nct,r,g,b,red,green,blue,cub);

      /* now, compare with the colors in that cubicle */

      m=cub->n;
      ci=cub->index;

      mindist=256*256*100; /* max dist is 256²*3 */
      
      while (m--)
      {
	 int dist=sf.r*SQ(fe[*ci].color.r-rgbr)+
	          sf.g*SQ(fe[*ci].color.g-rgbg)+
	          sf.b*SQ(fe[*ci].color.b-rgbb);
	 
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
      if (dither_got)
      {
	 (dither_got)(dith,rowpos,*s,NCTLU_DITHER_GOT);
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

static void NCTLU_FLAT_FULL_NAME(rgb_group *s,
				 NCTLU_DESTINATION *d,
				 int n,
				 struct neo_colortable *nct,
				 struct nct_dither *dith,
				 int rowlen)
{
   /* no need to build any data, we're using full scan */

   rgbl_group sf=nct->spacefactor;
   int mprim=nct->u.flat.numentries;
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
      int m;
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
      {
	 int dist=sf.r*SQ(fe->color.r-rgbr)+
	          sf.g*SQ(fe->color.g-rgbg)+
	          sf.b*SQ(fe->color.b-rgbb);
	 
	 if (dist<mindist)
	 {
	    lc->dest=fe->color;
	    mindist=dist;
	    lc->index=fe->no;
	    NCTLU_CACHE_HIT_WRITE;
	 }
	 
	 fe++;
      }

done_pixel:
      if (dither_got)
      {
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

static void NCTLU_CUBE_NAME(rgb_group *s,
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

   redf=255.0/redm;
   greenf=255.0/greenm;
   bluef=255.0/bluem;

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
	 int rgbr,rgbg,rgbb;
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

	 if (red && green && blue)
	 {
	    lc->dest.r=((int)(((rgbr*red+hred)>>8)*redf));
	    lc->dest.g=((int)(((rgbg*green+hgreen)>>8)*greenf));
	    lc->dest.b=((int)(((rgbb*blue+hblue)>>8)*bluef));

	    lc->index=i=
	       ((rgbr*red+hred)>>8)+
	       (((rgbg*green+hgreen)>>8)+
		((rgbb*blue+hblue)>>8)*green)*red;

	    NCTLU_CACHE_HIT_WRITE;

	    mindist=
	       sf.r*SQ(rgbr-lc->dest.r)+
	       sf.g*SQ(rgbg-lc->dest.g)+
	       sf.b*SQ(rgbb-lc->dest.b);
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

	       i=(int)
		  (( sc->steps *
		     ( ((int)rgbr-(int)sc->low.r)*sc->vector.r +
		       ((int)rgbg-(int)sc->low.g)*sc->vector.g +
		       ((int)rgbb-(int)sc->low.b)*sc->vector.b ) ) *
		   sc->invsqvector);

	       if (i<0) i=0; else if (i>=sc->steps) i=sc->steps-1;
	       if (sc->no[i]>=nc) 
	       {
		  float f=i*sc->mqsteps;
		  int drgbr=sc->low.r+(int)(sc->vector.r*f);
		  int drgbg=sc->low.g+(int)(sc->vector.g*f);
		  int drgbb=sc->low.b+(int)(sc->vector.b*f);

		  int ldist=sf.r*SQ(rgbr-drgbr)+
		     sf.g*SQ(rgbg-drgbg)+sf.b*SQ(rgbb-drgbb);

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
	 if (dither_got)
	 {
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

