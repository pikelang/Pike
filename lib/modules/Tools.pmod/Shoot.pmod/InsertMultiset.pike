#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Insert in multiset";

int k = 5; /* variable to tune the time of the test */
int m = 100000; /* the target size of the multiset */
int n = m*k; // for reporting

int perform()
{
   for (int i=0; i<k; i++)
   {
      multiset v=(<>);
      for (int j=0; j<m; j++)
	 v[j]=42;
   }
   return n;
}
