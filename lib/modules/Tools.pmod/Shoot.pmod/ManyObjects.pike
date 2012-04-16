#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Clone/free w many allocated";

class Target
{
}

int count = 1000000;
int iters = 10000;

void perform()
{
  array a = allocate (count);
  for (int i = 0; i < count; i++)
    a[i] = Target();
  for (int i = 0; i < iters; i++)
    a[random (count)] = Target();
  a = 0;
}

string present_n(int ntot,int nruns,float tseconds,float useconds,int memusage)
{
   return sprintf("%.0f/s",ntot/useconds);
}
