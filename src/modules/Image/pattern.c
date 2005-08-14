/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: pattern.c,v 1.32 2005/08/14 02:26:25 nilsson Exp $
*/

/*
**! module Image
**! class Image
*/

#include "global.h"

#include <math.h>
#include <ctype.h>

#include "pike_macros.h"
#include "object.h"
#include "interpret.h"
#include "svalue.h"
#include "pike_error.h"
#include "threads.h"
#include "builtin_functions.h"

#include "image.h"


#define sp Pike_sp

extern struct program *image_program;
#ifdef THIS
#undef THIS
#endif

#define THIS ((struct image *)(Pike_fp->current_storage))
#define THISOBJ (Pike_fp->current_object)

#define testrange(x) MAXIMUM(MINIMUM((x),255),0)

/**************** noise ************************/

#define NOISE_PTS 1024
#define NOISE_PX 173
#define NOISE_PY 263
#define NOISE_PZ 337
#define NOISE_PHI 0.6180339

static const double noise_p1[NOISE_PTS]=
{0.7687, 0.7048, 0.2400, 0.8877, 0.3604, 0.3365, 0.4929, 0.6689, 
 0.5958, 0.1747, 0.8518, 0.1367, 0.8194, 0.2953, 0.7487, 0.8822, 
 0.8200, 0.4410, 0.5080, 0.2866, 0.4414, 0.1916, 0.0896, 0.1759, 
 0.0235, 0.9063, 0.8467, 0.3697, 0.9539, 0.2161, 0.3376, 0.4781, 
 0.0404, 0.7452, 0.3082, 0.2999, 0.7496, 0.7981, 0.3566, 0.2403, 
 0.4941, 0.5717, 0.1580, 0.7082, 0.6773, 0.4202, 0.5161, 0.0228, 
 0.2133, 0.9290, 0.3990, 0.1545, 0.5137, 0.4264, 0.2689, 0.9402, 
 0.0390, 0.1291, 0.0186, 0.3314, 0.8891, 0.0748, 0.5900, 0.7282, 
 0.2568, 0.2610, 0.9559, 0.8816, 0.8173, 0.7489, 0.8003, 0.3485, 
 0.1316, 0.5136, 0.3941, 0.1695, 0.2983, 0.1769, 0.4636, 0.6973, 
 0.2022, 0.8917, 0.4715, 0.0473, 0.9375, 0.1862, 0.2337, 0.5616, 
 0.9899, 0.9458, 0.4315, 0.6101, 0.3923, 0.1765, 0.8713, 0.8660, 
 0.9788, 0.6965, 0.7949, 0.8544, 0.3404, 0.2833, 0.6467, 0.6696, 
 0.7099, 0.3645, 0.7927, 0.6079, 0.3101, 0.5087, 0.4604, 0.6480, 
 0.0893, 0.7578, 0.8481, 0.2156, 0.2696, 0.0442, 0.1220, 0.6014, 
 0.4046, 0.8839, 0.5009, 0.8173, 0.5277, 0.0990, 0.0918, 0.0228, 
 0.8590, 0.4364, 0.2373, 0.3989, 0.6103, 0.6776, 0.1127, 0.4201, 
 0.1175, 0.2932, 0.7182, 0.7290, 0.3086, 0.1774, 0.9052, 0.0521, 
 0.9181, 0.1937, 0.9579, 0.8550, 0.4845, 0.1713, 0.5141, 0.3299, 
 0.2570, 0.6416, 0.0187, 0.6541, 0.4997, 0.6352, 0.5448, 0.4020, 
 0.9326, 0.1000, 0.6143, 0.7616, 0.5312, 0.4836, 0.1026, 0.0959, 
 0.4505, 0.6905, 0.3672, 0.6621, 0.0063, 0.1184, 0.4625, 0.1035, 
 0.1686, 0.7521, 0.4968, 0.5912, 0.8935, 0.2918, 0.4169, 0.3222, 
 0.7360, 0.4472, 0.3940, 0.2136, 0.8741, 0.9153, 0.1906, 0.5217, 
 0.4420, 0.0737, 0.2710, 0.6104, 0.5819, 0.6017, 0.1838, 0.8967, 
 0.9024, 0.5567, 0.0868, 0.0671, 0.0721, 0.2892, 0.4601, 0.5607, 
 0.6657, 0.2669, 0.7205, 0.2867, 0.3277, 0.7668, 0.2638, 0.9661, 
 0.1739, 0.2096, 0.5236, 0.4272, 0.2204, 0.2966, 0.5984, 0.3059, 
 0.0430, 0.8306, 0.9162, 0.1058, 0.0954, 0.0722, 0.5292, 0.0835, 
 0.6769, 0.9378, 0.8544, 0.5085, 0.8344, 0.5948, 0.8653, 0.0779, 
 0.9802, 0.7879, 0.2400, 0.5082, 0.3063, 0.5473, 0.1164, 0.9740, 
 0.2239, 0.8660, 0.4383, 0.4147, 0.6045, 0.7131, 0.9683, 0.6793, 
 0.0016, 0.1293, 0.2765, 0.2639, 0.6423, 0.9848, 0.0677, 0.5649, 
 0.8255, 0.0042, 0.4541, 0.8783, 0.3919, 0.8202, 0.2587, 0.7520, 
 0.1886, 0.3004, 0.0462, 0.7979, 0.3158, 0.6397, 0.3529, 0.8591, 
 0.0152, 0.5907, 0.4932, 0.8954, 0.6015, 0.0181, 0.5114, 0.1067, 
 0.5667, 0.1782, 0.5789, 0.5225, 0.3517, 0.6661, 0.5322, 0.9047, 
 0.6874, 0.5696, 0.5431, 0.2188, 0.0406, 0.1732, 0.1677, 0.9587, 
 0.9042, 0.5177, 0.4410, 0.0944, 0.1186, 0.9457, 0.6834, 0.6387, 
 0.0736, 0.8309, 0.3855, 0.7442, 0.6791, 0.2814, 0.5121, 0.7879, 
 0.9188, 0.8110, 0.2677, 0.6821, 0.1788, 0.6663, 0.9223, 0.1031, 
 0.7554, 0.1469, 0.5570, 0.6277, 0.9514, 0.4740, 0.1838, 0.0930, 
 0.1817, 0.7849, 0.5443, 0.3878, 0.5380, 0.8811, 0.0354, 0.7584, 
 0.4730, 0.0498, 0.8755, 0.5989, 0.3843, 0.9411, 0.4075, 0.3554, 
 0.9685, 0.1298, 0.2706, 0.0303, 0.0923, 0.3284, 0.6295, 0.8364, 
 0.2272, 0.9446, 0.0554, 0.9644, 0.0576, 0.7719, 0.7295, 0.2646, 
 0.4186, 0.1998, 0.7432, 0.9269, 0.3642, 0.7662, 0.6662, 0.1944, 
 0.1640, 0.5519, 0.9751, 0.1133, 0.2077, 0.2534, 0.1187, 0.5120, 
 0.7075, 0.3182, 0.1954, 0.2734, 0.5388, 0.0017, 0.8496, 0.5164, 
 0.4852, 0.2012, 0.9248, 0.4175, 0.8240, 0.3260, 0.9319, 0.8085, 
 0.2599, 0.2982, 0.9690, 0.6432, 0.1807, 0.9633, 0.7636, 0.2775, 
 0.5623, 0.4697, 0.5302, 0.9255, 0.6512, 0.2484, 0.4095, 0.9082, 
 0.9555, 0.3873, 0.2352, 0.9118, 0.7003, 0.7411, 0.8043, 0.4670, 
 0.1022, 0.4890, 0.0277, 0.7999, 0.0875, 0.0844, 0.8216, 0.8933, 
 0.4677, 0.8663, 0.8427, 0.0467, 0.6049, 0.8252, 0.3966, 0.2997, 
 0.8895, 0.0866, 0.3467, 0.7939, 0.8406, 0.1862, 0.8006, 0.6423, 
 0.3897, 0.0644, 0.8612, 0.4608, 0.6045, 0.5408, 0.1665, 0.4078, 
 0.4385, 0.2768, 0.3044, 0.3732, 0.5513, 0.6722, 0.0719, 0.4786, 
 0.5780, 0.4782, 0.9945, 0.1610, 0.3673, 0.7276, 0.9026, 0.4755, 
 0.2647, 0.2281, 0.1462, 0.6187, 0.0958, 0.6629, 0.7232, 0.0236, 
 0.0031, 0.3000, 0.7699, 0.1195, 0.6393, 0.0527, 0.2222, 0.1655, 
 0.7791, 0.1133, 0.7713, 0.2936, 0.3225, 0.4165, 0.9379, 0.9315, 
 0.0575, 0.4141, 0.6120, 0.8673, 0.6008, 0.7180, 0.6675, 0.8053, 
 0.2967, 0.3415, 0.8708, 0.3892, 0.2538, 0.3193, 0.6067, 0.0751, 
 0.8396, 0.3271, 0.1228, 0.4228, 0.6866, 0.6215, 0.3515, 0.1843, 
 0.0576, 0.4182, 0.4157, 0.4971, 0.6309, 0.8058, 0.4618, 0.8474, 
 0.6389, 0.0106, 0.5598, 0.3309, 0.8114, 0.0908, 0.9076, 0.5896, 
 0.5131, 0.9562, 0.1158, 0.6961, 0.7800, 0.4473, 0.8020, 0.5228, 
 0.8909, 0.8693, 0.5286, 0.8295, 0.8740, 0.1878, 0.7165, 0.3866, 
 0.1310, 0.7550, 0.2143, 0.0800, 0.9434, 0.6247, 0.0441, 0.6296, 
 0.9317, 0.0809, 0.9818, 0.8191, 0.7728, 0.9794, 0.6236, 0.0144, 
 0.2532, 0.6249, 0.3857, 0.9459, 0.7895, 0.7425, 0.9788, 0.7865, 
 0.0067, 0.9575, 0.6814, 0.6146, 0.4233, 0.4704, 0.9981, 0.0789, 
 0.0089, 0.4896, 0.4814, 0.7071, 0.6309, 0.7672, 0.3645, 0.7544, 
 0.2217, 0.5917, 0.1480, 0.4721, 0.9727, 0.4928, 0.8778, 0.5783, 
 0.9809, 0.5546, 0.4090, 0.5683, 0.5473, 0.3674, 0.4655, 0.8797, 
 0.3345, 0.9084, 0.7385, 0.5225, 0.5336, 0.9589, 0.2187, 0.1393, 
 0.4571, 0.1999, 0.8941, 0.4972, 0.9713, 0.1058, 0.9829, 0.8620, 
 0.3618, 0.9503, 0.1876, 0.5877, 0.3078, 0.9158, 0.3272, 0.9254, 
 0.2042, 0.7342, 0.9056, 0.0973, 0.3088, 0.0447, 0.8651, 0.3242, 
 0.2660, 0.4869, 0.4494, 0.2914, 0.3389, 0.0622, 0.7240, 0.9595, 
 0.6498, 0.4760, 0.0652, 0.6276, 0.3105, 0.5467, 0.6027, 0.2171, 
 0.1156, 0.9168, 0.6701, 0.3420, 0.8573, 0.0484, 0.1067, 0.9349, 
 0.0637, 0.2597, 0.1725, 0.4449, 0.6534, 0.3603, 0.2301, 0.3719, 
 0.6100, 0.9287, 0.9496, 0.1607, 0.0382, 0.3598, 0.4184, 0.8958, 
 0.9452, 0.8336, 0.0573, 0.3794, 0.7989, 0.1625, 0.3010, 0.6052, 
 0.8766, 0.1344, 0.4450, 0.9775, 0.2486, 0.3225, 0.6702, 0.0929, 
 0.0211, 0.3915, 0.7763, 0.1955, 0.6162, 0.6752, 0.7172, 0.4842, 
 0.8239, 0.4846, 0.0681, 0.2412, 0.4180, 0.3550, 0.4031, 0.4777, 
 0.0513, 0.1022, 0.5924, 0.7315, 0.6194, 0.5468, 0.3459, 0.5171, 
 0.6451, 0.4542, 0.0839, 0.2676, 0.1364, 0.3328, 0.7431, 0.1189, 
 0.2263, 0.0217, 0.1147, 0.3416, 0.2595, 0.6336, 0.3791, 0.6836, 
 0.9893, 0.5300, 0.5335, 0.3350, 0.3957, 0.4743, 0.8273, 0.3891, 
 0.4633, 0.8093, 0.6686, 0.3951, 0.1993, 0.2763, 0.2018, 0.2980, 
 0.0348, 0.3035, 0.2091, 0.6760, 0.8118, 0.1277, 0.7763, 0.8856, 
 0.5983, 0.3265, 0.0116, 0.7842, 0.3227, 0.3496, 0.1837, 0.6255, 
 0.4573, 0.1999, 0.9196, 0.7800, 0.3660, 0.7175, 0.2186, 0.1136, 
 0.3776, 0.2073, 0.5970, 0.2148, 0.2940, 0.2365, 0.9086, 0.4323, 
 0.9523, 0.4547, 0.8701, 0.2223, 0.6027, 0.1456, 0.0715, 0.7862, 
 0.8969, 0.2468, 0.8504, 0.5191, 0.0046, 0.8948, 0.5343, 0.5310, 
 0.2632, 0.9892, 0.3481, 0.8706, 0.1639, 0.7501, 0.2854, 0.4820, 
 0.1252, 0.7363, 0.8666, 0.1446, 0.2743, 0.9916, 0.2177, 0.6328, 
 0.9839, 0.2445, 0.6251, 0.2676, 0.3472, 0.9282, 0.9502, 0.7808, 
 0.3386, 0.2376, 0.9352, 0.9027, 0.5149, 0.7046, 0.8354, 0.3020, 
 0.1605, 0.4126, 0.6469, 0.3662, 0.6485, 0.1521, 0.6084, 0.0158, 
 0.6871, 0.4573, 0.3146, 0.6456, 0.5723, 0.2811, 0.2008, 0.1417, 
 0.5530, 0.3282, 0.4978, 0.4112, 0.9264, 0.6691, 0.8212, 0.7854, 
 0.8532, 0.2828, 0.8122, 0.9924, 0.8407, 0.1477, 0.2978, 0.5783, 
 0.7982, 0.5179, 0.0069, 0.8777, 0.3810, 0.5811, 0.2716, 0.2422, 
 0.3157, 0.6051, 0.5134, 0.1766, 0.2420, 0.7351, 0.6494, 0.2080, 
 0.9935, 0.3045, 0.8878, 0.7562, 0.4357, 0.6308, 0.6765, 0.4405, 
 0.3394, 0.1515, 0.6879, 0.9503, 0.2588, 0.9600, 0.7340, 0.2615, 
 0.0003, 0.1613, 0.9478, 0.5435, 0.3687, 0.8694, 0.0192, 0.3585, 
 0.7304, 0.0261, 0.0012, 0.5478, 0.9967, 0.3536, 0.6742, 0.4038, 
 0.4891, 0.1186, 0.7220, 0.7170, 0.3725, 0.3645, 0.4661, 0.7263, 
 0.9476, 0.6232, 0.7563, 0.3064, 0.1896, 0.7577, 0.2857, 0.6538, 
 0.6877, 0.8372, 0.3775, 0.8186, 0.5448, 0.2821, 0.0539, 0.7979, 
 0.4466, 0.7094, 0.6955, 0.0312, 0.6918, 0.8280, 0.8005, 0.3598, 
 0.5976, 0.6272, 0.5848, 0.6874, 0.7982, 0.5763, 0.5195, 0.8390, 
 0.5000, 0.8023, 0.3386, 0.6425, 0.8371, 0.1078, 0.5953, 0.6293, 
 0.3947, 0.7281, 0.7168, 0.0861, 0.8370, 0.3893, 0.8870, 0.9266, 
 0.2741, 0.4673, 0.4586, 0.7928, 0.3216, 0.2355, 0.0391, 0.8477, 
 0.4804, 0.9869, 0.5283, 0.1822, 0.5600, 0.9885, 0.6584, 0.2821, 
 0.5312, 0.2921, 0.0052, 0.2498, 0.0235, 0.6823, 0.0413, 0.0466, 
 0.1085, 0.5900, 0.6050, 0.3950, 0.9968, 0.8928, 0.6105, 0.2027, 
 0.8709, 0.1192, 0.8529, 0.1567, 0.5247, 0.5208, 0.8042, 0.9685, 
 0.9567, 0.1834, 0.2093, 0.1247, 0.5295, 0.8238, 0.2331, 0.7307, 
 0.7975, 0.8971, 0.6995, 0.5483, 0.5574, 0.2934, 0.5426, 0.9595 };

#define COLORRANGE_LEVELS 1024

#define FRAC(X) ((X)-floor(X))

static double noise(double Vx,double Vy,const double *noise_p)
{
   int Ax[3],Ay[3];
   int n,i,j;
   double Sx[3],Sy[3];
   double sum,dsum,f,fx,fy;

   fx=floor(Vx);
   fy=floor(Vy);
   
   for (n=0; n<3; n++) 
   {
      Ax[n] = DOUBLE_TO_INT(floor(NOISE_PX*FRAC( (fx+n)*NOISE_PHI )));
      Ay[n] = DOUBLE_TO_INT(floor(NOISE_PY*FRAC( (fy+n)*NOISE_PHI )));
   }
   
   f=FRAC(Vx);
   Sx[0]=0.5-f+0.5*f*f;
   Sx[1]=0.5+f-f*f;
   Sx[2]=0.5*f*f;

   f=FRAC(Vy);
   Sy[0]=0.5-f+0.5*f*f;
   Sy[1]=0.5+f-f*f;
   Sy[2]=0.5*f*f;

/*    fprintf(stderr,"Vx=%3g fx=%3g Vy=%3g fy=%5g Sx=%5.3f,%5.3f,%5.3f = %5.3f*%5.3f = %5.3f\n", */
/* 	   Vx,fx,Vy,fy,Sx[0],Sx[1],Sx[2], */
/* 	   noise_p[ ((int)(fx+0)) & (NOISE_PTS-1) ],Sx[0], */
/* 	   noise_p[ ((int)(fx+0)) & (NOISE_PTS-1) ]*Sx[0]);  */

/*    return  */

/*       noise_p[ ((int)(fx+0)) & (NOISE_PTS-1) ] * Sx[0]  */
      /*      noise_p[ ((int)(fx+0.5)) & (NOISE_PTS-1) ]*Sx[1] +*/
      /*      noise_p[ ((int)(fx+1)) & (NOISE_PTS-1) ]*Sx[2] */;

   sum=0;
   for (i=0; i<3; i++)
   {
      for (j=0,dsum=0; j<3; j++)
	 dsum+=Sy[j]*noise_p[ (Ax[i]+Ay[j]) & (NOISE_PTS-1) ];
      sum+=Sx[i]*dsum;
   }
   return sum;
}

static INLINE double turbulence(double x,double y,int octaves)
{
   double t=0;
   double mul=1;
   while (octaves-->0)
   {
      t+=noise(x*mul,y*mul,noise_p1)*mul;
      mul*=0.5;
   }
   return t;
}

static void init_colorrange(rgb_group *cr,struct svalue *s,char *where)
{
   double *v,*vp;
   int i,n,k;
   rgbd_group lrgb,*rgbp,*rgb;
   double fr,fg,fb,q;
   int b;

   if (s->type!=T_ARRAY)
      Pike_error("Illegal colorrange to %s\n",where);
   else if (s->u.array->size<2)
      Pike_error("Colorrange array too small (meaningless) (to %s)\n",where);

   vp=v=(void*)xalloc(sizeof(double)*(s->u.array->size/2+1));
   rgbp=rgb=(void*)xalloc(sizeof(rgbd_group)*(s->u.array->size/2+1));

   for (i=0; i<s->u.array->size-1; i+=2)
   {
      rgb_group rgbt;

      if (s->u.array->item[i].type==T_INT) 
	 *vp = (double)s->u.array->item[i].u.integer;
      else if (s->u.array->item[i].type==T_FLOAT) 
	 *vp = s->u.array->item[i].u.float_number;
      else 
	 bad_arg_error(where,
		       0,0, 1, "array of int|float,color", 0,
		       "%s: expected int or float at element %d"
		       " of colorrange\n",where,i);

      if (*vp>1) *vp=1;
      else if (*vp<0) *vp=0;
      vp++;

      if (!image_color_svalue(s->u.array->item+i+1,&rgbt))
	 bad_arg_error(where,
		       0,0, 1, "array of int|float,color", 0,
		       "%s: no color at element %d"
		       " of colorrange\n",where,i+1);

      rgbp->r=rgbt.r;
      rgbp->g=rgbt.g;
      rgbp->b=rgbt.b;

      rgbp++;
   }
   *vp=v[0]+1+1.0/(COLORRANGE_LEVELS-1);
   lrgb=*rgbp=rgb[0]; /* back to original color */

   for (k=1,i=DOUBLE_TO_INT(v[0]*(COLORRANGE_LEVELS-1));
	k<=s->u.array->size/2; k++)
   {
      n = DOUBLE_TO_INT(v[k]*COLORRANGE_LEVELS);

      if (n>i)
      {
	 q = 1.0/((double)(n-i));
   
	 fr=(rgb[k].r-lrgb.r)*q;
	 fg=(rgb[k].g-lrgb.g)*q;
	 fb=(rgb[k].b-lrgb.b)*q;

	 for (b=0;i<n && i<COLORRANGE_LEVELS;i++,b++)
	 {
	    cr[i&(COLORRANGE_LEVELS-1)].r = DOUBLE_TO_COLORTYPE(lrgb.r+fr*b);
	    cr[i&(COLORRANGE_LEVELS-1)].g = DOUBLE_TO_COLORTYPE(lrgb.g+fg*b);
	    cr[i&(COLORRANGE_LEVELS-1)].b = DOUBLE_TO_COLORTYPE(lrgb.b+fb*b);
	 }
      }
      lrgb=rgb[k];
   }

#if 0
   for (k=0; k<COLORRANGE_LEVELS; k++)
      fprintf(stderr,"#%02x%02x%02x \n",
	      cr[k].r,cr[k].g,cr[k].b);
#endif

   free(v);
   free(rgb);
}

#define GET_FLOAT_ARG(sp,args,n,def,where) \
   ( (args>n) \
      ? ( (sp[n-args].type==T_INT) ? (double)(sp[n-args].u.integer) \
	  : ( (sp[n-args].type==T_FLOAT) ? sp[n-args].u.float_number \
	      : ( Pike_error("illegal argument(s) to %s\n", where), 0.0 ) ) ) \
      : def )
#define GET_INT_ARG(sp,args,n,def,where) \
   ( (args>n) \
      ? ( (sp[n-args].type==T_INT) ? sp[n-args].u.integer \
	  : ( (sp[n-args].type==T_FLOAT) ? DOUBLE_TO_INT(sp[n-args].u.float_number) \
	      : ( Pike_error("illegal argument(s) to %s\n", where), 0 ) ) ) \
      : def )

/*
**! method void noise(array(float|int|array(int)) colorrange)
**! method void noise(array(float|int|array(int)) colorrange,float scale,float xdiff,float ydiff,float cscale)
**! 	Gives a new image with the old image's size,
**!	filled width a 'noise' pattern.
**!
**!	The random seed may be different with each instance of pike.
**!
**!	Example: 
**!	<tt>->noise( ({0,({255,0,0}), 0.3,({0,255,0}), 0.6,({0,0,255}), 0.8,({255,255,0})}), 0.2,0.0,0.0,1.0 );</tt>
**!	<br><illustration>return Image.Image(200,100)->noise( ({0,({255,0,0}), 0.3,({0,255,0}), 0.6,({0,0,255}), 0.8,({255,255,0})}), 0.2,0.0,0.0,1.0 );</illustration>
**!
**! arg array(float|int|array(int)) colorrange
**! 	colorrange table
**! arg float scale
**!	default value is 0.1
**! arg float xdiff
**! arg float ydiff
**!	default value is 0,0
**! arg float cscale
**!	default value is 1
**! see also: turbulence
*/

void image_noise(INT32 args)
{
   int x,y;
   rgb_group cr[COLORRANGE_LEVELS];
   double scale,xdiff,ydiff,cscale,xp,yp;
   rgb_group *d;
   struct object *o;
   struct image *img;

   if (args<1) Pike_error("too few arguments to image->noise()\n");

   scale=GET_FLOAT_ARG(sp,args,1,0.1,"image->noise");
   xdiff=GET_FLOAT_ARG(sp,args,2,0,"image->noise");
   ydiff=GET_FLOAT_ARG(sp,args,3,0,"image->noise");
   cscale=GET_FLOAT_ARG(sp,args,4,1,"image->noise");

   init_colorrange(cr,sp-args,"image->noise()");

   o=clone_object(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize)))
   {
      free_object(o);
      SIMPLE_OUT_OF_MEMORY_ERROR("noise",
				 sizeof(rgb_group)*THIS->xsize*THIS->ysize);
   }

   cscale*=COLORRANGE_LEVELS;

   d=img->img;
   for (y=THIS->ysize,xp=xdiff; y--; xp+=1.0)
      for (x=THIS->xsize,yp=ydiff; x--; yp+=1.0)
      {
	 *(d++)=
	    cr[DOUBLE_TO_INT((noise((double)x*scale,(double)y*scale,noise_p1)
/* 		      +noise((double)(x*0.5+y*0.8660254037844386)*scale, */
/* 			     (double)(-y*0.5+x*0.8660254037844386)*scale, */
/* 			     noise_p2) */
			      )
			     *cscale)&(COLORRANGE_LEVELS-1)];
      }

   pop_n_elems(args);
   push_object(o);
}

/*
**! method void turbulence(array(float|int|array(int)) colorrange)
**! method void turbulence(array(float|int|array(int)) colorrange,int octaves,float scale,float xdiff,float ydiff,float cscale)
**! 	gives a new image with the old image's size,
**!	filled width a 'turbulence' pattern
**!
**!	The random seed may be different with each instance of pike.
**!
**!	Example: <br>
**!	<tt>->turbulence( ({0,({229,204,204}), 0.9,({229,20,20}), 0.9,Color.black}) );</tt>
**!	<br><illustration>return Image.Image(200,100)->
**!     turbulence( ({0,({229,204,204}), 0.9,({229,20,20}), 0.9,Image.Color.black}));</illustration>
**! arg array(float|int|array(int)) colorrange
**! 	colorrange table
**! arg int octaves
**!	default value is 3
**! arg float scale
**!	default value is 0.1
**! arg float xdiff
**! arg float ydiff
**!	default value is 0,0
**! arg float cscale
**!	default value is 1
**! see also: noise, Image.Color
*/

void image_turbulence(INT32 args)
{
/* parametrar: 	array(float|int|array(int)) colorrange,
		int octaves=3,
                float scale=0.1,
		float xdiff=0,
		float ydiff=0,
   		float cscale=2
*/
   int x,y,octaves;
   rgb_group cr[COLORRANGE_LEVELS];
   double scale,xdiff,ydiff,cscale,xp,yp;
   rgb_group *d;
   struct object *o;
   struct image *img;

   if (args<1) Pike_error("too few arguments to image->turbulence()\n");

   octaves = GET_INT_ARG(sp,args,1,3,"image->turbulence");
   scale = GET_FLOAT_ARG(sp,args,2,0.1,"image->turbulence");
   xdiff = GET_FLOAT_ARG(sp,args,3,0,"image->turbulence");
   ydiff = GET_FLOAT_ARG(sp,args,4,0,"image->turbulence");
   cscale = GET_FLOAT_ARG(sp,args,5,2,"image->turbulence");

   init_colorrange(cr,sp-args,"image->turbulence()");

   o=clone_object(image_program,0);
   img=(struct image*)o->storage;
   *img=*THIS;
   if (!(img->img=malloc(sizeof(rgb_group)*THIS->xsize*THIS->ysize)))
   {
      free_object(o);
      SIMPLE_OUT_OF_MEMORY_ERROR("noise",
				 sizeof(rgb_group)*THIS->xsize*THIS->ysize);
   }

   cscale*=COLORRANGE_LEVELS;

   d=img->img;
   for (y=THIS->ysize,xp=xdiff; y--; xp+=1.0)
      for (x=THIS->xsize,yp=ydiff; x--; yp+=1.0)
      {
#if 0
	 if (y==0 && x<10)
	 {
	    fprintf(stderr,"%g*%g=%d => %d\n",
		    turbulence(xp*scale,yp*scale,octaves),
		    cscale,
		    (INT32)(turbulence(xp*scale,yp*scale,octaves)*cscale),
		    (INT32)(turbulence(xp*scale,yp*scale,octaves)*cscale)&(COLORRANGE_LEVELS)-1 );
	 }
#endif
	 *(d++)=
	    cr[DOUBLE_TO_INT(turbulence(xp*scale,yp*scale,octaves)*cscale)
	       & (COLORRANGE_LEVELS-1)];
      }

   pop_n_elems(args);
   push_object(o);
}

/*
**! method object random()
**! method object random(int seed)
**! method object randomgrey()
**! method object random(greyint seed)
**! 	Gives a randomized image;<br>
**!	<table><tr valign=center>
**!	<td><illustration> return lena(); </illustration></td>
**!	<td><illustration> return lena()->random()</illustration></td>
**!	<td><illustration> return lena()->random(17)</illustration></td>
**!	<td><illustration> return lena()->random(17)->grey()</illustration></td>
**!	<td><illustration> return lena()->random(17)->color(255,0,0)</illustration></td>
**!	<td><illustration> return lena()->random(17)->color(255,0,0)->grey(1,0,0)</illustration></td>
**!	</tr><tr>
**!	<td>original</td>
**!	<td>->random()</td>
**!	<td>->random(17)</td>
**!	<td>greyed<br>(same again)</td>
**!	<td>color(red)<br>(same again)</td>
**!	<td>...red channel<br></td>
**!	</tr></table>
**!
**!	Use with -><ref>grey</ref>() or -><ref>color</ref>() 
**!	for one-color-results.
**!
**! returns a new image
**!
**! see also test, noise
*/

void image_random(INT32 args)
{
   struct object *o;
   struct image *img;
   rgb_group *d;
   INT32 n;

   push_int(THIS->xsize);
   push_int(THIS->ysize);
   o=clone_object(image_program,2);
   img=(struct image*)get_storage(o,image_program);
   d=img->img;
   if (args) f_random_seed(args);

   THREADS_ALLOW();

   n=img->xsize*img->ysize;
   while (n--)
   {
      d->r=(COLORTYPE)my_rand();
      d->g=(COLORTYPE)my_rand();
      d->b=(COLORTYPE)my_rand();
      d++;
   }

   THREADS_DISALLOW();

   push_object(o);
}

void image_randomgrey(INT32 args)
{
   struct object *o;
   struct image *img;
   rgb_group *d;
   INT32 n;

   push_int(THIS->xsize);
   push_int(THIS->ysize);
   o=clone_object(image_program,2);
   img=(struct image*)get_storage(o,image_program);
   d=img->img;
   if (args) f_random_seed(args);

   THREADS_ALLOW();

   n=img->xsize*img->ysize;
   while (n--)
   {
      d->r=d->g=d->b=(COLORTYPE)my_rand();
      d++;
   }

   THREADS_DISALLOW();

   push_object(o);
}
