#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Loops Nested (global)";

int n=0;
int a,b,c,d,e,f;

int perform()
{
#define ITER 31
   n = 0;
   for (a=0; a<ITER; a++)
      for (b=0; b<ITER; b++)
	 for (c=0; c<ITER; c++)
	    for (d=0; d<ITER; d++)
	       for (e=0; e<ITER; e++)
              for (int f; f<1; f++)
                   n++;
   return n;
}
