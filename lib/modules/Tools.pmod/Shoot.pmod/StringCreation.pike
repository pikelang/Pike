#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="String Creation";

int k = 1000; /* variable to tune the time of the test */
string file = random_string(255*1024*10);

// Tests the string_magnitude, internal_findstring and memhash
// functions most (and malloc/free/memcpy).
//
int perform()
{
    int q;
    int z = 1000;
    array ss;
    for( int i=0; i<k; i++ )
    {
        // This temporary is needed, otherwise the optimizer optimizes
        // away the division below.
        ss = file/z;
        q+=sizeof(ss);
        ss = ({});
    }
    return q;
}
