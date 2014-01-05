#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Append mapping (+)";

int k = 600; /* variable to tune the time of the test */
int m = 2000; /* the target size of the mapping */

int perform()
{
   for (int i=0; i<k; i++)
   {
      mapping v=([]);
      for (int j=0; j<m; j++)
	v+=([j:42]);
   }
   return m*k;
}
