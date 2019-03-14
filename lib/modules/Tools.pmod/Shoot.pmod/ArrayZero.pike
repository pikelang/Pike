#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name = "Array Zero";

int perform()
{
  for (int i = 0; i < 10000; i++)
    arrayzero();
  return 10000;
}

void arrayzero()
{
  array arg = allocate(4711);
}
