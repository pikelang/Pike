inherit Tools.Shoot.Test;

constant name="Append multiset";

int k = 5; /* variable to tune the time of the test */
int m = 100000; /* the target size of the multiset */
int n = m*k; // for reporting

void perform()
{
   for (int i=0; i<k; i++)
   {
      multiset v=(<>);
      int z=i*m+m;
      for (int j=i*m; j<z; j++)
	 v[j]=42;
   }
}

string present_n(int ntot,int nruns,float tseconds,float useconds,int memusage)
{
   return sprintf("%.0f/s",ntot/useconds);
}
