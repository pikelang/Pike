#pike __REAL_VERSION__
// inherit Tools.Shoot.Test;

constant name="Adding element to array (local)";

#define ITER 10000

int perform()
{
    array a = ({});
  for (int i=0; i<ITER; i++)
      a = a+({random(10)});
  return ITER;
}
