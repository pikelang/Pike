#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Sort unordered integers";

array(int) test_array = allocate (100000, random) (100000);

int perform()
{
  for (int i = 0; i < 10; i++)
    sort (test_array + ({}));
  return 10*sizeof(test_array);
}
