#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name = "Array Copy";

int perform()
{
  int i;
  for (i = 0; i < 10000; i++)
    arraycopy();
  return i*1001;
}

void arraycopy()
{
  array arg = ({ "a", "b", "c" }), arg2 = arg;
  for( int i=0; i<1000; i++ )
      arg += ({});
}
