#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name = "Array Zero";

void perform()
{
  for (int i = 0; i < 10000; i++)
    arrayzero();
}

void arrayzero()
{
  array arg = allocate(4711);
}
