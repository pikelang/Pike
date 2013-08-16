#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Append array";

int k = 80; /* variable to tune the time of the test */
int m = 100000; /* the target size of the array */

int perform()
{
   for (int i=0; i<k; i++)
   {
      array v=({});
      for (int j=0; j<m; j++)
	 v+=({42});
   }
   return m*k;
}
