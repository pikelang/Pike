#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Loops Nested (local,var)";

int n=0;

int perform()
{
  int iter = 40;
  int x=0;

   for (int a; a<iter; a++)
      for (int b; b<iter; b++)
	 for (int c; c<iter; c++)
	    for (int d; d<iter; d++)
	       for (int e; e<iter; e++)
/* This is here to avoid the strength-reduce in the optimizer.. */
                 for (int f=0; f<1; f++)
                    x++;
   return x;
}
