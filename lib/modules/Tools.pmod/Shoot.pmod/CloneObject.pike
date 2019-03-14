#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Clone null-object";

class Target
{
}


int perform()
{
  int n = 300000;
  for( int i = 0; i<n; i++ )
    Target();
  return n;
}
