inherit Tools.Shoot.Test;

constant name="Sort unordered objects";

class Foo {
  int n = random (10000);
  int `< (Foo o) {return n < o->n;}
}

array(Foo) test_array = allocate (10000, Foo) ();

void perform()
{
  for (int i = 0; i < 10; i++)
    sort (test_array + ({}));
}
