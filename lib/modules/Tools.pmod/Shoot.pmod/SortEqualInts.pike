inherit Tools.Shoot.Test;

constant name="Sort equal integers";

array(int) test_array = allocate (100000, 17);

void perform()
{
  for (int i = 0; i < 10; i++)
    sort (test_array);
}
