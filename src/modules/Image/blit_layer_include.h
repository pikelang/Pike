/* $Id: blit_layer_include.h,v 1.1 1997/03/20 07:56:05 mirar Exp $ */
/* included w/ defines in blit.c, image_add_layer() */


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
	    if (lr->alpha!=0)
	    {
#ifndef ALL_IS_NOP
	       if (lr->method==LAYER_NOP)
	       {
#endif
#ifndef ALL_HAVE_MASK
		  if (lr->m) /* mask */
		  {
#endif
#ifndef ALL_HAVE_ALPHA_255
		     if (lr->alpha==255) /* only mask */
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
#ifndef ALL_HAVE_ALPHA_255
		     }
		     else /* alpha and mask */
		     {
			rgb.r=(unsigned char)
			   ((rgb.r*(255*255-lr->m->r*lr->alpha)
			     +lr->s->r*lr->m->r*lr->alpha)*q2);
			rgb.g=(unsigned char)
			   ((rgb.g*(255*255-lr->m->g*lr->alpha)
			     +lr->s->g*lr->m->g*lr->alpha)*q2);
			rgb.b=(unsigned char)
			   ((rgb.b*(255*255-lr->m->b*lr->alpha)
			     +lr->s->b*lr->m->b*lr->alpha)*q2);
		     }
#endif
		     lr->m++;
#ifndef ALL_HAVE_MASK
		  }
		  else /* no mask */
		  {
#ifndef ALL_HAVE_ALPHA_255
		     if (lr->alpha==255) /* no alpha (bam!) */
#endif
			rgb=lr->s[0];
#ifndef ALL_HAVE_ALPHA_255
		     else /* only alpha */
		     {
			rgb.r=(unsigned char)
			   ((rgb.r*(255-lr->alpha)
			     +lr->s->r*lr->alpha)*q);
			rgb.g=(unsigned char)
			   ((rgb.g*(255-lr->alpha)
			     +lr->s->g*lr->alpha)*q);
			rgb.b=(unsigned char)
			   ((rgb.b*(255-lr->alpha)
			     +lr->s->b*lr->alpha)*q);
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
		        res.r=max(lr->s->r,rgb.r);
		        res.g=max(lr->s->g,rgb.g);
		        res.b=max(lr->s->b,rgb.b);
			break;
		     case LAYER_MIN:
		        res.r=min(lr->s->r,rgb.r);
		        res.g=min(lr->s->g,rgb.g);
		        res.b=min(lr->s->b,rgb.b);
			break;
		     case LAYER_ADD:
		        res.r=(unsigned char)max(255,(lr->s->r+rgb.r));
		        res.g=(unsigned char)max(255,(lr->s->g+rgb.g));
		        res.b=(unsigned char)max(255,(lr->s->b+rgb.b));
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
#ifndef ALL_HAVE_ALPHA_255
		     if (lr->alpha==255) /* only mask */
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
#ifndef ALL_HAVE_ALPHA_255
		     }
		     else /* alpha and mask */
		     {
			rgb.r=(unsigned char)
			   ((rgb.r*(255*255-lr->m->r*lr->alpha)
			     +res.r*lr->m->r*lr->alpha)*q2);
			rgb.g=(unsigned char)
			   ((rgb.g*(255*255-lr->m->g*lr->alpha)
			     +res.g*lr->m->g*lr->alpha)*q2);
			rgb.b=(unsigned char)
			   ((rgb.b*(255*255-lr->m->b*lr->alpha)
			     +res.b*lr->m->b*lr->alpha)*q2);
		     }
#endif
		     lr->m++;
#ifndef ALL_HAVE_MASK
		  }
		  else /* no mask */
		  {
#ifndef ALL_HAVE_ALPHA_255
		     if (lr->alpha==255) /* no alpha (bam!) */
#endif
			rgb=res;
#ifndef ALL_HAVE_ALPHA_255
		     else /* only alpha */
		     {
			rgb.r=(unsigned char)
			   ((rgb.r*(255-lr->alpha)
			     +res.r*lr->alpha)*q);
			rgb.g=(unsigned char)
			   ((rgb.g*(255-lr->alpha)
			     +res.g*lr->alpha)*q);
			rgb.b=(unsigned char)
			   ((rgb.b*(255-lr->alpha)
			     +res.b*lr->alpha)*q);
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
