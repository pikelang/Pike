#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Insert in mapping";

int perform()
{
   for (int i=0; i<50; i++)
   {
      mapping v=([]);
      for (int j=0; j<100000; j++)
          v[j]=42;
   }
   return 50 * 100000;
}
