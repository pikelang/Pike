#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Sort ordered integers";

array(int) test_array = indices (allocate (100000));

void perform()
{
  for (int i = 0; i < 10; i++)
    sort (test_array);
}
