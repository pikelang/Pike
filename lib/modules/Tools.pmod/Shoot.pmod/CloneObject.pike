inherit Tools.Shoot.Test;

constant name="Clone null-object";

class Target
{
}

int n = 300000;

void perform()
{
  for( int i = 0; i<n; i++ )
    Target();
}

