#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name = "Array Copy";

void perform()
{
  for (int i = 0; i < 10000; i++)
    arraycopy();
}

void arraycopy()
{
  array arg = ({ "a", "b", "c" });
}
