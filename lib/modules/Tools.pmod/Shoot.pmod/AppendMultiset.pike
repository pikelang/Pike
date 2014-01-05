#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Append multiset";

int k = 10; /* variable to tune the time of the test */
int m = 1000; /* the target size of the multiset */

int perform()
{
   for (int i=0; i<k; i++)
   {
      multiset v=(<>);
      for (int j=0; j<m; j++)
	 v|=(<j>);
   }
   return m*k;
}
