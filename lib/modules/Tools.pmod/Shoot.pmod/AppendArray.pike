#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Append array";

int k = 5; /* variable to tune the time of the test */
int m = 100000; /* the target size of the array */
int n = m*k; // for reporting

void perform()
{
   for (int i=0; i<k; i++)
   {
      array v=({});
      for (int j=0; j<m; j++)
	 v+=({42});
   }
}

string present_n(int ntot,int nruns,float tseconds,float useconds,int memusage)
{
   return sprintf("%.0f/s",ntot/useconds);
}
