inherit Tools.Shoot.Test;

constant name="Loops Recursed";

int n=0;
int iter=16;
int d=5;

void recur(int d)
{
   if (d--)
      for (int i=0; i<iter; i++) recur(d);
   else
      n++;
}

void perform()
{
   recur(d);
}

string present_n(int ntot,int nruns,float tseconds,float useconds,int memusage)
{
   return sprintf("%.0f iters/s",ntot/useconds);
}
