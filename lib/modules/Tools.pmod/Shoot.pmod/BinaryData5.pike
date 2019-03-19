#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Read binary INT32";

int k = 50;
int m = 10000;

int perform()
{
   array v;
   for (int i=0; i<k; i++)
      v=array_sscanf(random_string(4*m),"%4c"*m);
   return m*k;
}
