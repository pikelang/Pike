#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Loops Nested (local)";

int n=0;

void perform()
{
   int iter = 16;
   int x=0;

   for (int a; a<iter; a++)
      for (int b; b<iter; b++)
	 for (int c; c<iter; c++)
	    for (int d; d<iter; d++)
	       for (int e; e<iter; e++)
		  for (int f; f<iter; f++)
		     x++;

   n=x;
}

string present_n(int ntot,int nruns,float tseconds,float useconds,int memusage)
{
   return sprintf("%.0f iters/s",ntot/useconds);
}
