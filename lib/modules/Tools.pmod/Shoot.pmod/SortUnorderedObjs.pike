#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Sort unordered objects";

class Foo {
  int n = random (10000);
  protected int `< (Foo o) {return n < o->n;}
}

array(Foo) test_array = allocate (10000, Foo) ();

int perform()
{
  for (int i = 0; i < 10; i++)
    sort (test_array + ({}));
  return sizeof(test_array)*10;
}
