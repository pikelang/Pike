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
   MEMCPY(da,sa,sizeof(rgb_group)*len); /* always copy alpha channel */
#define da da da /* protect */
   if (alpha==0.0)
   {
#ifdef LAYER_DUAL
      MEMCPY(d,s,sizeof(rgb_group)*len);
#endif
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
	   if (la->r==0 && la->g==0 && la->b==0)
	     *d=*s;
	   else
	     L_CHANNEL_DO(*s,*l,*d,*la);
	   l++; s++; la++; d++; sa++; 
	 }
   }
   else
   {
      if (!la)  /* no layer alpha => alpha opaque */
	 while (len--)
	 {
	    L_CHANNEL_DO_V(*s,*l,*d,white,alpha);
	    l++; s++; la++; d++; sa++; 
	 }
      else
	 while (len--)
	 {
	    L_CHANNEL_DO_V(*s,*l,*d,white,alpha);
	    l++; s++; la++; d++; sa++; 
	 }
   }
}
#undef da
