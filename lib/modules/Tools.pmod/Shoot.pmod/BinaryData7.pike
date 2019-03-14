#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Read binary INT128";

int perform()
{
   int n = 10000;
   array v=array_sscanf(random_string(16*n),"%16c"*n);
   return n;
}
