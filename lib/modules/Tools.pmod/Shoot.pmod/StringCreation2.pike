#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="String Creation (existing)";

int k = 1000; /* variable to tune the time of the test */
int n;
string file = (string)enumerate(256*1024);
string file2 = file+"0";

// This code mostly tests memcmp, actually.
void perform()
{
    int q;
    array ss;
    int z = 1000;
    array zz = file/z;
    q += sizeof(zz);

    for( int i=0; i<k; i++ )
    {
        ss = file2/z;
        q += sizeof(ss);
    }
    ss = zz = 0;
    n = q;
}

string present_n(int ntot,int nruns,float tseconds,float useconds,int memusage)
{
   return sprintf("%.0f/s",ntot/useconds);
}
