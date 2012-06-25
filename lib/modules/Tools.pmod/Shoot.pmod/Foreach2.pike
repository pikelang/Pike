#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Foreach (arr;local;local)";

array const = enumerate(10000000);
int n;

void perform()
{
    int res;

    foreach( const;int ind; int i )
        res ++;

    n=sizeof(const);
}


string present_n(int ntot,int nruns,float tseconds,float useconds,int memusage)
{
   return sprintf("%.0f iters/s",ntot/useconds);
}
