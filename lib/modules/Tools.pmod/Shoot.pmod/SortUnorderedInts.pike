inherit Tools.Shoot.Test;

constant name="Sort unordered integers";

array(int) test_array = allocate (100000, random) (100000);

void perform()
{
  for (int i = 0; i < 10; i++)
    sort (test_array + ({}));
}
