#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Clone null-object";

class Target
{
}

int n = 300000;

void perform()
{
  for( int i = 0; i<n; i++ )
    Target();
}

string present_n(int ntot,int nruns,float tseconds,float useconds,int memusage)
{
   return sprintf("%.0f/s",ntot/useconds);
}
