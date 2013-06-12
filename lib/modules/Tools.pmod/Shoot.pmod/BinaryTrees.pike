inherit Tools.Shoot.Test;

constant name = "Binary Trees";

private class Tree
{
    Tree left, right;
    int val;
    void create( int v, int vv, int depth )
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
int n;

string present_n(int ntot,int nruns,float tseconds,float useconds,int memusage)
{
   return sprintf("%.0f/s",ntot/useconds);
}

void perform()
{
    Tree longlived = Tree( 0,0,k );
    n -= Tree(0,0,k)->check() * 2->pow(k);
    n -= longlived->check() * 2->pow(k);
}
