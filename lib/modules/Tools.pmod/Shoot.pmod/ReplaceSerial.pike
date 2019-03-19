#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Replace (serial)";


int perform()
{
   int z=1000;
   int n=z*4;
   while (z--)
   {
      string s="a rather long but unfortunately not so random string"*1000;

      s=replace(s,"a","e");
      s=replace(s,"l","q");
      s=replace(s," r","cdefgh");
      s=replace(s,"ing","");
   }
   return n;
}
