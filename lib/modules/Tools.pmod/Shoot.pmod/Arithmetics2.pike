#pike __REAL_VERSION__
// inherit Tools.Shoot.Test;

constant name="Simple arithmentics (globals)";

#define ITER 3100000
int b,a;
int perform()
{
  for (int i=0; i<ITER; i++)
    a = a+b;
  return ITER;
}
