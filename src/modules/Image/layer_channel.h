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
	    L_CHANNEL_DO(*s,*l,*d,*la);
	    l++; s++; la++; d++;
	 }
   }
   else
   {
      if (!la)  /* no layer alpha => full opaque */
	 while (len--)
	 {
	    L_CHANNEL_DO_V(*s,*l,*d,white,alpha);
	    l++; s++; d++;
	 }
      else
	 while (len--)
	 {
	    L_CHANNEL_DO_V(*s,*l,*d,white,alpha);
	    l++; s++; la++; d++;
	 }
   }
}
