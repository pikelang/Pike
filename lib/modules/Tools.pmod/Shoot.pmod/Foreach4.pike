#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Foreach (arr;local;global)";
constant dont_dump_program = 1;

array const_array = enumerate(10000000);
int n;

int perform()
{
    int res;
    foreach( const_array;int ind;n )
        res=1;
    return ++n;
}
