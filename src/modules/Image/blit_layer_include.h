/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/* included w/ defines in blit.c, image_add_layer() */

/*
**! module Image
**! class Image
*/


   i=(y2-y1+1);
   while (i--)
   {
      j=(x2-x1+1);

      while (j--)
      {
	 rgb_group rgb;
	 struct img_layer *lr=layer;
	 rgb=*s;
	 for (l=0; l<layers; l++,lr++)
	    if (lr->opaque!=0)
	    {
#ifndef ALL_IS_NOP
	       if (lr->method==LAYER_NOP)
	       {
#endif
#ifndef ALL_HAVE_MASK
		  if (lr->m) /* mask */
		  {
#endif
#ifndef ALL_HAVE_OPAQUE_255
		     if (lr->opaque==255) /* only mask */
		     {
#endif
			if (lr->m->r==255)
			   rgb.r=lr->s->r;
			else if (lr->m->r!=0)
			   rgb.r=(unsigned char)
			      ((rgb.r*(255-lr->m->r)
				+lr->s->r*lr->m->r)*q);
			if (lr->m->g==255)
			   rgb.g=lr->s->g;
			else if (lr->m->g!=0)
			   rgb.g=(unsigned char)
			      ((rgb.g*(255-lr->m->g)
				+lr->s->g*lr->m->g)*q);
			if (lr->m->b==255)
			   rgb.b=lr->s->b;
			else if (lr->m->b!=0)
			   rgb.b=(unsigned char)
			      ((rgb.b*(255-lr->m->b)
				+lr->s->b*lr->m->b)*q);
#ifndef ALL_HAVE_OPAQUE_255
		     }
		     else /* opaque and mask */
		     {
			rgb.r=(unsigned char)
			   ((rgb.r*(255*255-lr->m->r*lr->opaque)
			     +lr->s->r*lr->m->r*lr->opaque)*q2);
			rgb.g=(unsigned char)
			   ((rgb.g*(255*255-lr->m->g*lr->opaque)
			     +lr->s->g*lr->m->g*lr->opaque)*q2);
			rgb.b=(unsigned char)
			   ((rgb.b*(255*255-lr->m->b*lr->opaque)
			     +lr->s->b*lr->m->b*lr->opaque)*q2);
		     }
#endif
		     lr->m++;
#ifndef ALL_HAVE_MASK
		  }
		  else /* no mask */
		  {
#ifndef ALL_HAVE_OPAQUE_255
		     if (lr->opaque==255) /* no opaque (bam!) */
#endif
			rgb=lr->s[0];
#ifndef ALL_HAVE_OPAQUE_255
		     else /* only opaque */
		     {
			rgb.r=(unsigned char)
			   ((rgb.r*(255-lr->opaque)
			     +lr->s->r*lr->opaque)*q);
			rgb.g=(unsigned char)
			   ((rgb.g*(255-lr->opaque)
			     +lr->s->g*lr->opaque)*q);
			rgb.b=(unsigned char)
			   ((rgb.b*(255-lr->opaque)
			     +lr->s->b*lr->opaque)*q);
		     }
#endif
		  }
#endif 
#ifndef ALL_IS_NOP
	       }
	       else 
	       {
		  rgb_group res;

		  if (!lr->m || lr->m->r || lr->m->g || lr->m->b)
		  switch (lr->method)
		  {
		     case LAYER_MAX:
		        res.r=MAXIMUM(lr->s->r,rgb.r);
		        res.g=MAXIMUM(lr->s->g,rgb.g);
		        res.b=MAXIMUM(lr->s->b,rgb.b);
			break;
		     case LAYER_MIN:
		        res.r=MINIMUM(lr->s->r,rgb.r);
		        res.g=MINIMUM(lr->s->g,rgb.g);
		        res.b=MINIMUM(lr->s->b,rgb.b);
			break;
		     case LAYER_ADD:
		        res.r=(unsigned char)MAXIMUM(255,(lr->s->r+rgb.r));
		        res.g=(unsigned char)MAXIMUM(255,(lr->s->g+rgb.g));
		        res.b=(unsigned char)MAXIMUM(255,(lr->s->b+rgb.b));
			break;
		     case LAYER_MULT:
		        res.r=(unsigned char)((lr->s->r*rgb.r)*q);
		        res.g=(unsigned char)((lr->s->g*rgb.g)*q);
		        res.b=(unsigned char)((lr->s->b*rgb.b)*q);
			break;
		     case LAYER_DIFF:
		        res.r=absdiff(lr->s->r,rgb.r);
		        res.g=absdiff(lr->s->g,rgb.g);
		        res.b=absdiff(lr->s->b,rgb.b);
			break;
		     default:
	  	        res=rgb;
		  }
		  else res=rgb;

#ifndef ALL_HAVE_MASK
		  if (lr->m) /* mask */
		  {
#endif
#ifndef ALL_HAVE_OPAQUE_255
		     if (lr->opaque==255) /* only mask */
		     {
#endif
			if (lr->m->r==255)
			   rgb.r=res.r;
			else if (lr->m->r!=0)
			   rgb.r=(unsigned char)
			      ((rgb.r*(255-lr->m->r)
				+res.r*lr->m->r)*q);
			if (lr->m->g==255)
			   rgb.g=res.g;
			else if (lr->m->g!=0)
			   rgb.g=(unsigned char)
			      ((rgb.g*(255-lr->m->g)
				+res.g*lr->m->g)*q);
			if (lr->m->b==255)
			   rgb.b=res.b;
			else if (lr->m->b!=0)
			   rgb.b=(unsigned char)
			      ((rgb.b*(255-lr->m->b)
				+res.b*lr->m->b)*q);
#ifndef ALL_HAVE_OPAQUE_255
		     }
		     else /* opaque and mask */
		     {
			rgb.r=(unsigned char)
			   ((rgb.r*(255*255-lr->m->r*lr->opaque)
			     +res.r*lr->m->r*lr->opaque)*q2);
			rgb.g=(unsigned char)
			   ((rgb.g*(255*255-lr->m->g*lr->opaque)
			     +res.g*lr->m->g*lr->opaque)*q2);
			rgb.b=(unsigned char)
			   ((rgb.b*(255*255-lr->m->b*lr->opaque)
			     +res.b*lr->m->b*lr->opaque)*q2);
		     }
#endif
		     lr->m++;
#ifndef ALL_HAVE_MASK
		  }
		  else /* no mask */
		  {
#ifndef ALL_HAVE_OPAQUE_255
		     if (lr->opaque==255) /* no opaque (bam!) */
#endif
			rgb=res;
#ifndef ALL_HAVE_OPAQUE_255
		     else /* only opaque */
		     {
			rgb.r=(unsigned char)
			   ((rgb.r*(255-lr->opaque)
			     +res.r*lr->opaque)*q);
			rgb.g=(unsigned char)
			   ((rgb.g*(255-lr->opaque)
			     +res.g*lr->opaque)*q);
			rgb.b=(unsigned char)
			   ((rgb.b*(255-lr->opaque)
			     +res.b*lr->opaque)*q);
		     }
#endif
		  }
#endif
	       }
#endif
	       lr->s++;
	    }
	 *(d++)=rgb;
	 s++;
      }

      s+=mod;
      for (l=0; l<layers; l++)
      {
	 layer[l].s+=mod;
	 if (layer[l].m) layer[l].m+=mod;
      }
   }
