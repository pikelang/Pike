#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Foreach (arr,local)";
constant dont_dump_program = 1;

array const = enumerate(10000000);

int perform()
{
    int res;
    foreach( const, int i )
        res=1;
    return sizeof(const);
}
