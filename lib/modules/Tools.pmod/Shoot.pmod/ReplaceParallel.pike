/* -*- mode: Pike; c-basic-offset: 3; -*- */

#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Replace (parallel)";

int perform()
{
   int z=1000;
   int n=z*4;
   while (z--)
   {
      string s="a rather long but unfortunately not so random string"*1000;

      s=replace(s,
		(["a":"e",
		  "l":"q",
		  " r":"cdefgh",
		  "ing":""]));
   }
   return n;
}
