#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Loops Nested (global)";

int n=0;
int a,b,c,d,e,f;

void perform()
{
#define ITER 16

   for (a=0; a<ITER; a++)
      for (b=0; b<ITER; b++)
	 for (c=0; c<ITER; c++)
	    for (d=0; d<ITER; d++)
	       for (e=0; e<ITER; e++)
		  for (f=0; f<ITER; f++)
		     n++;
}

string present_n(int ntot,int nruns,float tseconds,float useconds,int memusage)
{
   return sprintf("%.0f iters/s",ntot/useconds);
}
