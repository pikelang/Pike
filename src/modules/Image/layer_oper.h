/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/* template for operator layer row function */

static void LM_FUNC(rgb_group *s,rgb_group *l,rgb_group *d,
		    rgb_group *sa,rgb_group *la,rgb_group *da,
		    int len,double alpha)
{
   union { COLORTYPE r,g,b; } tz,*t=&tz;

#ifndef L_LOGIC
   if (alpha==0.0)
   {
#ifdef LAYER_DUAL
      MEMCPY(d,s,sizeof(rgb_group)*len);
      MEMCPY(da,sa,sizeof(rgb_group)*len);
#endif
      return; 
   }
   else if (alpha==1.0)
   {
#ifdef L_COPY_ALPHA
      MEMCPY(da,sa,sizeof(rgb_group)*len);
#define da da da da /* error */
#endif
      if (!la)  /* no layer alpha => full opaque */
      {
#ifdef L_MMX_OPER
#ifdef TRY_USE_MMX
	extern int try_use_mmx;
	if(try_use_mmx)
	{
	  int num=sizeof(rgb_group) * len;
	  unsigned char *source=(unsigned char *)s;
	  unsigned char *dest=(unsigned char *)d;
	  unsigned char *sourcel=(unsigned char *)l;
	  
	  while (num-->0 && (7&(int)dest))
	  {
	    *dest=L_TRUNC(L_OPER(*source,*sourcel));
	    source++;
	    sourcel++;
	    dest++;
	  }
	  
	  
	  while(num > 16)
	  {
	    movq_m2r(*source, mm0);
	    source+=8;
	    movq_m2r(*source, mm1);
	    source += 8;
	    L_MMX_OPER(*sourcel, mm0);
	    sourcel+=8;
	    L_MMX_OPER(*sourcel, mm1);
	    sourcel+=8;
	    movq_r2m(mm0,*dest);
            dest += 8;
	    movq_r2m(mm1,*dest);
            dest += 8;
            num-=16;
	  }

	  if (num > 8)
	  {
	    movq_m2r(*source, mm0);
	    L_MMX_OPER(*sourcel, mm0);
	    movq_r2m(mm0,*dest);
	    source+=8;
	    sourcel+=8;
	    dest+=8;
	    num-=8;
	  }

          emms();
	  while (num-->0)
	  {
	    *dest=L_TRUNC(L_OPER(*source,*sourcel));
	    source++;
	    sourcel++;
	    dest++;
	  }
	}
	else
#endif
#endif
	   while (len--)
	   {
	      d->r=L_TRUNC(L_OPER(s->r,l->r));
	      d->g=L_TRUNC(L_OPER(s->g,l->g));
	      d->b=L_TRUNC(L_OPER(s->b,l->b));
#ifndef L_COPY_ALPHA
	      *da=white; da++; 
#endif
	      l++; s++; sa++; d++; 
	   }
      }
      else
	 while (len--)
	 {
	    if (la->r==COLORMAX && la->g==COLORMAX && la->b==COLORMAX)
	    {
	       d->r=L_TRUNC(L_OPER(s->r,l->r));
	       d->g=L_TRUNC(L_OPER(s->g,l->g));
	       d->b=L_TRUNC(L_OPER(s->b,l->b));
#ifndef L_COPY_ALPHA
#ifdef L_USE_SA
	       *da=*sa;
#else
	       *da=white;
#endif
#endif
	    }
	    else if (la->r==0 && la->g==0 && la->b==0)
	    {
	       *d=*s;
#ifndef L_COPY_ALPHA
	       *da=*sa;
#endif
	    }
	    else
	    {
#ifndef L_COPY_ALPHA
	       t->r=L_TRUNC(L_OPER(s->r,l->r));
	       ALPHA_ADD(s,t,d,sa,la,da,r);
	       t->g=L_TRUNC(L_OPER(s->g,l->g));
	       ALPHA_ADD(s,t,d,sa,la,da,g);
	       t->b=L_TRUNC(L_OPER(s->b,l->b));
	       ALPHA_ADD(s,t,d,sa,la,da,b);
#ifdef L_USE_SA
	       *da=*sa;
#endif
#else
	       t->r=L_TRUNC(L_OPER(s->r,l->r));
	       ALPHA_ADD_nA(s,t,d,sa,la,da,r);
	       t->g=L_TRUNC(L_OPER(s->g,l->g));
	       ALPHA_ADD_nA(s,t,d,sa,la,da,g);
	       t->b=L_TRUNC(L_OPER(s->b,l->b));
	       ALPHA_ADD_nA(s,t,d,sa,la,da,b);
#endif
	    }
	    l++; s++; la++; sa++; d++; 
#ifndef L_COPY_ALPHA
	    da++;
#endif
	 }
   }
   else
   {
#ifdef L_COPY_ALPHA
#undef da
      MEMCPY(da,sa,sizeof(rgb_group)*len);
#define da da da
#endif
      if (!la)  /* no layer alpha => full opaque */
	 while (len--)
	 {
#ifndef L_COPY_ALPHA
	    t->r=L_TRUNC(L_OPER(s->r,l->r));
	    ALPHA_ADD_V_NOLA(s,t,d,sa,da,alpha,r);
	    t->g=L_TRUNC(L_OPER(s->g,l->g));
	    ALPHA_ADD_V_NOLA(s,t,d,sa,da,alpha,g);
	    t->b=L_TRUNC(L_OPER(s->b,l->b));
	    ALPHA_ADD_V_NOLA(s,t,d,sa,da,alpha,b);
#ifdef L_USE_SA
	    *da=*sa;
#endif
	    da++;
#else
	    t->r=L_TRUNC(L_OPER(s->r,l->r));
	    ALPHA_ADD_V_NOLA_nA(s,t,d,sa,da,alpha,r);
	    t->g=L_TRUNC(L_OPER(s->g,l->g));
	    ALPHA_ADD_V_NOLA_nA(s,t,d,sa,da,alpha,g);
	    t->b=L_TRUNC(L_OPER(s->b,l->b));
	    ALPHA_ADD_V_NOLA_nA(s,t,d,sa,da,alpha,b);

#endif
	    l++; s++; sa++; d++; 
	 }
      else
	 while (len--)
	 {
#ifndef L_COPY_ALPHA
	    t->r=L_TRUNC(L_OPER(s->r,l->r));
	    ALPHA_ADD_V(s,t,d,sa,la,da,alpha,r);
	    t->g=L_TRUNC(L_OPER(s->g,l->g));
	    ALPHA_ADD_V(s,t,d,sa,la,da,alpha,g);
	    t->b=L_TRUNC(L_OPER(s->b,l->b));
	    ALPHA_ADD_V(s,t,d,sa,la,da,alpha,b);
#ifdef L_USE_SA
	    *da=*sa;
#endif
	    da++; 
#else
	    t->r=L_TRUNC(L_OPER(s->r,l->r));
	    ALPHA_ADD_V_nA(s,t,d,sa,la,da,alpha,r);
	    t->g=L_TRUNC(L_OPER(s->g,l->g));
	    ALPHA_ADD_V_nA(s,t,d,sa,la,da,alpha,g);
	    t->b=L_TRUNC(L_OPER(s->b,l->b));
	    ALPHA_ADD_V_nA(s,t,d,sa,la,da,alpha,b);
#undef da
#endif
	    l++; s++; la++; sa++; d++;
	 }
   }
#else /* L_LOGIC */
   if (alpha==0.0)
   {
      smear_color(d,L_TRANS,len);
      smear_color(da,L_TRANS,len);
      return; 
   }
   else 
   {
      if (!la)  /* no layer alpha => full opaque */
	 while (len--)
	 {
	    *da=*d=L_LOGIC(L_OPER(s->r,l->r),
			   L_OPER(s->g,l->g),
			   L_OPER(s->b,l->b));
	    l++; s++; sa++; d++; da++; 
	 }
      else
	 while (len--)
	 {
	    if (la->r==0 && la->g==0 && la->b==0)
	       *da=*d=L_TRANS;
	    else
	       *da=*d=L_LOGIC(L_OPER(s->r,l->r),
			      L_OPER(s->g,l->g),
			      L_OPER(s->b,l->b));
	    l++; s++; la++; sa++; d++; da++; 
	 }
   }
#endif /* L_LOGIC */
}
