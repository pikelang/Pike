/*
 * Compile & Exec
 *
 * This tests creates a small program that juggles with integeres,
 * and compiles it <runs> times.
 *
 */

inherit Tools.Shoot.Test;

constant name="Compile";

void perform()
{
   int n=2000,v=10,runs=10;
   string prog="";

   for (int i=0; i<v; i++)
      prog+=sprintf("int %c=random(1000);\n",i+'a');
   
   for (int i=0; i<n; i++)
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
   for (int i=0; i<runs; i++)
      compile_string(prog);
}
