
#pike 7.6

inherit Array;

array transpose_old(array(array|string) x)
{
   if (!sizeof(x)) return x;
   array ret=allocate(sizeof([array|string]x[0]));
   for(int e=0;e<sizeof([array|string]x[0]);e++) ret[e]=column(x,e);
   return ret;
}
