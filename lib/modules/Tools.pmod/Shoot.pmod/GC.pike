inherit Tools.Shoot.Test;

constant name="GC";

class Target
{
  int i;
  int j;
  object q;
  float x;
}

int c = 2000;
int n = 150;

void perform()
{
  array waster = allocate( c );
  for( int i = 0 ; i<c; i++ )
  {
    waster[i] = Target();
    waster[i]->q = Target();
    waster[i]->q->q=waster[random(i)];
  }

  for( int i = 0; i<n; i++ )
    gc();
}

