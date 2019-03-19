#pike __REAL_VERSION__
// inherit Tools.Shoot.Test;

constant name="Adding element to array (global)";

#define ITER 10000

array a = ({});
int perform()
{
  for (int i=0; i<ITER; i++)
      a = a+({random(10)});
  return ITER;
}
