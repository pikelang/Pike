#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Clone object";

class Target
{
  int flag;
  void create( int i ) { flag = i; }
  int swap(){ flag = !flag; }
}


int perform()
{
  int n = 300000;
  for( int i = 0; i<n; i++ )
    Target(i); //->swap();
  return n;
}
