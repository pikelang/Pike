#pike __REAL_VERSION__
// inherit Tools.Shoot.Test;

constant name="Simple arithmentics (private global)";

final private int a,b;

#define ITER 3100000

int perform()
{
  for (int i=0; i<ITER; i++)
    a = a+b;
  return ITER;
}
