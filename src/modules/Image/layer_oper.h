/* template for operator layer row function */

static void LM_FUNC(rgb_group *s,rgb_group *l,rgb_group *d,
		    rgb_group *sa,rgb_group *la,rgb_group *da,
		    int len,float alpha)
{
   if (alpha==0.0)
   {
      MEMCPY(s,d,sizeof(rgb_group)*len);
      MEMCPY(sa,da,sizeof(rgb_group)*len);
      return; 
   }
   else if (alpha==1.0)
   {
      if (!la)  /* no layer alpha => full opaque */
	 while (len--)
	 {
	    d->r=L_TRUNC(L_OPER(COMBINE(s->r,sa->r),l->r));
	    d->g=L_TRUNC(L_OPER(COMBINE(s->g,sa->g),l->g));
	    d->b=L_TRUNC(L_OPER(COMBINE(s->b,sa->b),l->b));
	    *da=white;
	    l++; s++; sa++; da++; d++;
	 }
      else
	 while (len--)
	 {
	    d->r=L_TRUNC(L_OPER(COMBINE(s->r,sa->r),COMBINE(l->r,la->r)));
	    d->g=L_TRUNC(L_OPER(COMBINE(s->g,sa->g),COMBINE(l->g,la->g)));
	    d->b=L_TRUNC(L_OPER(COMBINE(s->b,sa->b),COMBINE(l->b,la->b)));
	    da->r=COMBINE_ALPHA_SUM(la->r,sa->r);
	    da->g=COMBINE_ALPHA_SUM(la->g,sa->g);
	    da->b=COMBINE_ALPHA_SUM(la->b,sa->b);
	    l++; s++; la++; sa++; da++; d++;
	 }
   }
   else
   {
      if (!la)  /* no layer alpha => full opaque */
	 while (len--)
	 {
	    d->r=L_TRUNC(L_OPER(COMBINE(s->r,sa->r),COMBINE_A(l->r,alpha)));
	    d->g=L_TRUNC(L_OPER(COMBINE(s->g,sa->g),COMBINE_A(l->g,alpha)));
	    d->b=L_TRUNC(L_OPER(COMBINE(s->b,sa->b),COMBINE_A(l->b,alpha)));
	    da->r=COMBINE_ALPHA_SUM_V(COLORMAX,sa->r,alpha);
	    da->g=COMBINE_ALPHA_SUM_V(COLORMAX,sa->g,alpha);
	    da->b=COMBINE_ALPHA_SUM_V(COLORMAX,sa->b,alpha);
	    l++; s++; sa++; da++; d++;
	 }
      else
	 while (len--)
	 {
	    d->r=L_TRUNC(L_OPER(COMBINE(s->r,sa->r),
				COMBINE_V(l->r,la->r,alpha)));
	    d->g=L_TRUNC(L_OPER(COMBINE(s->g,sa->g),
				COMBINE_V(l->g,la->g,alpha)));
	    d->b=L_TRUNC(L_OPER(COMBINE(s->g,sa->b),
				COMBINE_V(l->b,la->b,alpha)));
	    da->r=COMBINE_ALPHA_SUM_V(la->r,sa->r,alpha);
	    da->g=COMBINE_ALPHA_SUM_V(la->g,sa->g,alpha);
	    da->b=COMBINE_ALPHA_SUM_V(la->b,sa->b,alpha);
	    l++; s++; la++; sa++; da++; d++;
	 }
   }
}
