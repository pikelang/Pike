/* template for operator layer row function */

static void LM_FUNC(rgb_group *s,rgb_group *l,rgb_group *d,
		    rgb_group *sa,rgb_group *la,rgb_group *da,
		    int len,double alpha)
{
#ifndef L_LOGIC
   if (alpha==0.0)
   {
      MEMCPY(d,s,sizeof(rgb_group)*len);
      MEMCPY(da,sa,sizeof(rgb_group)*len);
      return; 
   }
   else if (alpha==1.0)
   {
      if (!la)  /* no layer alpha => full opaque */
	 while (len--)
	 {
	    d->r=L_TRUNC(L_OPER(s->r,l->r));
	    d->g=L_TRUNC(L_OPER(s->g,l->g));
	    d->b=L_TRUNC(L_OPER(s->b,l->b));
	    *da=white;
	    l++; s++; sa++; da++; d++;
	 }
      else
	 while (len--)
	 {
	    if (la->r==COLORMAX && la->g==COLORMAX && la->b==COLORMAX)
	    {
	       d->r=L_TRUNC(L_OPER(s->r,l->r));
	       d->g=L_TRUNC(L_OPER(s->g,l->g));
	       d->b=L_TRUNC(L_OPER(s->b,l->b));
#ifdef L_USE_SA
	       *da=*sa;
#else
	       *da=white;
#endif
	    }
	    else if (la->r==0 && la->g==0 && la->b==0)
	    {
	       *d=*s;
	       *da=*sa;
	    }
	    else
	    {
	       d->r=L_TRUNC(L_OPER(s->r,l->r));
	       ALPHA_ADD(s,d,d,sa,la,da,r);
	       d->g=L_TRUNC(L_OPER(s->g,l->g));
	       ALPHA_ADD(s,d,d,sa,la,da,g);
	       d->b=L_TRUNC(L_OPER(s->b,l->b));
	       ALPHA_ADD(s,d,d,sa,la,da,b);
#ifdef L_USE_SA
	       *da=*sa;
#endif
	    }
	    l++; s++; la++; sa++; da++; d++;
	 }
   }
   else
   {
      if (!la)  /* no layer alpha => full opaque */
	 while (len--)
	 {
	    d->r=L_TRUNC(L_OPER(s->r,l->r));
	    ALPHA_ADD_V_NOLA(s,d,d,sa,da,alpha,r);
	    d->g=L_TRUNC(L_OPER(s->g,l->g));
	    ALPHA_ADD_V_NOLA(s,d,d,sa,da,alpha,g);
	    d->b=L_TRUNC(L_OPER(s->b,l->b));
	    ALPHA_ADD_V_NOLA(s,d,d,sa,da,alpha,b);
#ifdef L_USE_SA
	    *da=*sa;
#endif
	    l++; s++; sa++; da++; d++;
	 }
      else
	 while (len--)
	 {
	    d->r=L_TRUNC(L_OPER(s->r,l->r));
	    ALPHA_ADD_V(s,d,d,sa,la,da,alpha,r);
	    d->g=L_TRUNC(L_OPER(s->g,l->g));
	    ALPHA_ADD_V(s,d,d,sa,la,da,alpha,g);
	    d->b=L_TRUNC(L_OPER(s->b,l->b));
	    ALPHA_ADD_V(s,d,d,sa,la,da,alpha,b);
#ifdef L_USE_SA
	    *da=*sa;
#endif
	    l++; s++; la++; sa++; da++; d++;
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
	    l++; s++; sa++; da++; d++;
	 }
      else
	 while (len--)
	 {
	    if (la->r==0 && la->g==0 && la->b==0)
	       *d=*da=L_TRANS;
	    else
	       *da=*d=L_LOGIC(L_OPER(s->r,l->r),
			      L_OPER(s->g,l->g),
			      L_OPER(s->b,l->b));
	    l++; s++; la++; sa++; da++; d++;
	 }
   }
#endif /* L_LOGIC */
}

