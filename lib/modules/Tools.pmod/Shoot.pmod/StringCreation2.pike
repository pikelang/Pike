#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="String Creation (existing)";

int k = 800; /* variable to tune the time of the test */
int n;
string file = random_string(1000*1024);

void perform()
{
    int q;
    array ss;
    int z = 1000;
    array zz = file/z;
    q += sizeof(zz);

    for( int i=0; i<k; i++ )
    {
        ss = file/z;
        q += sizeof(ss);
    }
    ss = zz = 0;
    n = q;
}

string present_n(int ntot,int nruns,float tseconds,float useconds,int memusage)
{
   return sprintf("%.0f/s",ntot/useconds);
}
