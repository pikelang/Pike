#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Read binary INT16";

int k = 100;
int m = 10000;

int perform()
{
   array v;
   for (int i=0; i<k; i++)
      v=array_sscanf(random_string(2*m),"%2c"*m);
   return m*k;
}
