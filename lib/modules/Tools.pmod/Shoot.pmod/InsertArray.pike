#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Insert in array";

int k = 50; /* variable to tune the time of the test */
int m = 100000; /* the target size of the mapping */
int n = m*k; // for reporting

int perform()
{
   array a = allocate(m);
   for (int i=0; i<k; i++)
   {
      for (int j=0; j<m; j++)
          a[j]=42;
   }
   return n;
}
