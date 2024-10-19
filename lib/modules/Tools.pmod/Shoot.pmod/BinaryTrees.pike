#pike __REAL_VERSION__

inherit Tools.Shoot.Test;

constant name = "Binary Trees";

private class Tree
{
    Tree left, right;
    int val;
    protected void create( int v, int vv, int depth )
    {
        val = v;
        if( depth > 0 )
        {
            depth--;
            left = this_program(2*vv-1,vv+1,depth);
            right = this_program(2*vv,vv+1,depth);
        }
    }

    int check()
    {
        if( !right ) return val;
        return val + left->check() - right->check();
    }
}

int k = 18;

int perform()
{
    int n;
    Pike.gc_parameters( (["enabled":0]) );
    Tree longlived = Tree( 0,0,k );
    for( int i=0; i<3; i++ )
        n -= Tree(0,0,k)->check() * 2->pow(k);
    n -= longlived->check() * 2->pow(k);
    return n;
}
