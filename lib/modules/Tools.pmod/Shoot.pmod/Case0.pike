#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Upper/lower case shift 0";
constant m = 255;

string t;

void create()
{
  String.Buffer b = String.Buffer();
  while(sizeof(b)<1024*1024)
    b->putchar( random(m) );
  t = (string)b;
}

void perform()
{
  string t2 = upper_case(t);
  if( t )
    t2 = lower_case(t);
  if( !t )
    t = t2;
}
