
#pike 7.7

inherit Array;

array transpose_old(array(array|string) x)
{
   if (!sizeof(x)) return x;
   array ret=allocate(sizeof([array|string]x[0]));
   for(int e=0;e<sizeof([array|string]x[0]);e++) ret[e]=column(x,e);
   return ret;
}

int oid_sort_func(string a0,string b0)
{
  int r = ::oid_sort_func(a0,b0);
  if( r==-1 ) return 0;
  return r;
}
