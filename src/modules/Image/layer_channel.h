/* template for operator layer row function */

static void LM_FUNC(rgb_group *s,rgb_group *l,rgb_group *d,
		    rgb_group *sa,rgb_group *la,rgb_group *da,
		    int len,double alpha)
{
   MEMCPY(da,sa,sizeof(rgb_group)*len); /* always copy alpha channel */
   if (alpha==0.0)
   {
      MEMCPY(d,s,sizeof(rgb_group)*len);
      return; 
   }
   else if (alpha==1.0)
   {
      if (!la)  /* no layer alpha => full opaque (channel replacement) */
	 while (len--)
	 {
	    L_CHANNEL_DO(*s,*l,*d,white);
	    l++; s++; d++;
	 }
      else
	 while (len--)
	 {
	    if (la->r==COLORMAX && la->g==COLORMAX && la->b==COLORMAX)
	       L_CHANNEL_DO(*s,*l,*d,*la);
	    else if (la->r==0 && la->g==0 && la->b==0)
	    {
	       *d=*s;
	       *da=*sa;
	    }
	    else
	    {
	       L_CHANNEL_DO(*s,*l,*d,*la);
	       ALPHA_ADD(s,d,d,sa,la,da,r);
	       ALPHA_ADD(s,d,d,sa,la,da,g);
	       ALPHA_ADD(s,d,d,sa,la,da,b);
	    }

	    l++; s++; la++; d++; sa++; da++;
	 }
   }
   else
   {
      if (!la)  /* no layer alpha => alpha opaque */
	 while (len--)
	 {
	    L_CHANNEL_DO_V(*s,*l,*d,white,alpha);
	    ALPHA_ADD(s,d,d,sa,la,da,r);
	    ALPHA_ADD(s,d,d,sa,la,da,g);
	    ALPHA_ADD(s,d,d,sa,la,da,b);
	    l++; s++; d++; sa++; da++;
	 }
      else
	 while (len--)
	 {
	    L_CHANNEL_DO_V(*s,*l,*d,white,alpha);
	    ALPHA_ADD(s,d,d,sa,la,da,r);
	    ALPHA_ADD(s,d,d,sa,la,da,g);
	    ALPHA_ADD(s,d,d,sa,la,da,b);
	    l++; s++; la++; d++; sa++; da++;
	 }
   }
}
