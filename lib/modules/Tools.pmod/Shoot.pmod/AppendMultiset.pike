inherit Tools.Shoot.Test;

constant name="Append multiset";

int k = 10; /* variable to tune the time of the test */
int m = 1000; /* the target size of the multiset */
int n = m*k; // for reporting

void perform()
{
   for (int i=0; i<k; i++)
   {
      multiset v=(<>);
      for (int j=0; j<m; j++)
	 v|=(<j>);
   }
}

string present_n(int ntot,int nruns,float tseconds,float useconds,int memusage)
{
   return sprintf("%.0f/s",ntot/useconds);
}
