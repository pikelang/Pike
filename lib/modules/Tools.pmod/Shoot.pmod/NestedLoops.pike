inherit Tools.Shoot.Test;

constant name="Loops Nested";

void perform()
{
   int n = 16;
   int x=0;

   for (int a; a<n; a++)
      for (int b; b<n; b++)
	 for (int c; c<n; c++)
	    for (int d; d<n; d++)
	       for (int e; e<n; e++)
		  for (int f; f<n; f++)
		     x++;
}
