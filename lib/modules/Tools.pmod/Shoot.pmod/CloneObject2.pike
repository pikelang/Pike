inherit Tools.Shoot.Test;

constant name="Clone object";

class Target
{
  int flag;
  void create( int i ) { flag = i; }
  int swap(){ flag = !flag; }
}

int n = 300000;

void perform()
{
  for( int i = 0; i<n; i++ )
    Target(i); //->swap();
}

