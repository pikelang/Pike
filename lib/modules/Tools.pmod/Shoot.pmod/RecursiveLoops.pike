inherit Tools.Shoot.Test;

constant name="Loops Recursed";

int n=16;
int d=5;
int x=0;

void recur(int d)
{
   if (d--)
      for (int i=0; i<n; i++) recur(d);
   else
      x++;
}

void perform()
{
   recur(d);
}
