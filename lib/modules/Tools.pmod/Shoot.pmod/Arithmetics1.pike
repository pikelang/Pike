#pike __REAL_VERSION__
// inherit Tools.Shoot.Test;

constant name="Simple arithmetics (locals)";

#define ITER 3100000

int perform()
{
 int a,b;
  for (int i=0; i<ITER; i++)
    a = a+b;
  return ITER;
}
