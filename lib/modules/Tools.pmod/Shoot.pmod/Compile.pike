/*
 * Compile & Exec
 *
 * This tests creates a small program that juggles with integeres,
 * and compiles it <runs> times.
 *
 */

#pike __REAL_VERSION__

inherit Tools.Shoot.Test;

constant name="Compile";

int n;

final constant runs = 30;
final constant ops = 2000;


string prog="";
void create()
{
    int v=10;

   for (int i=0; i<v; i++)
      prog+=sprintf("int %c=random(1000);\n",i+'a');

   for (int i=0; i<ops; i++)
   {
      int c=random(4),d;
      switch (c)
      {
	 case 0..2:
	    prog+=
	       sprintf("%c=%c%s%c;\n",
		       random(v)+'a',random(v)+'a',
		       ("*+-"/1)[c],
		       random(v)+'a');
	    break;
	 case 3:
	    d=random(v);
	    prog+=
	       sprintf("if (%c) %c=%c/%c;\n",
		       d+'a',
		       random(v)+'a',random(v)+'a',
		       d+'a');
	    break;
      }
      if (!(i%50))
      {
	 for (int i=0; i<v; i++)
	    prog+=sprintf("if (%c==0 || %c>1000000000 || %c<-1000000000) %c=random(1000);\n",i+'a',i+'a',i+'a',i+'a');
      }
   }
   prog="int test() {\n"+prog+"   return a;\n};\n";
}

int perform()
{
   for (int i=0; i<runs; i++)
      compile_string(prog);

   return (ops+ops/5+12)*runs;
}

string present_n(int ntot,int nruns,float tseconds,float useconds,int memusage)
{
   return sprintf("%.0fk lines/s",ntot/useconds/1000);
}
