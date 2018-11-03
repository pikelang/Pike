#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Foreach (arr;local;global)";
constant dont_dump_program = 1;

int arr_size = 10000000;

array prepare() { return enumerate(arr_size); }

int n;

int perform(array const_array)
{
    int res;
    foreach( const_array;int ind;n )
        res=1;
    return ++n;
}
