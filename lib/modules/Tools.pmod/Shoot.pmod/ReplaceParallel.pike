#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Replace (parallel)";

int n=0;

void perform()
{
   int z=1000;
   n=z*4;
   while (z--)
   {
      string s="a rather long but unfortunately not so random string"*1000;

      s=replace(s,
		(["a":"e",
		  "l":"q",
		  " r":"cdefgh",
		  "ing":""]));
   }
}

string present_n(int ntot,int nruns,float tseconds,float useconds,int memusage)
{
   return sprintf("%.0f repl/s",ntot/useconds);
}
