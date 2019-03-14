#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant size = 100;
constant name="Matrix multiplication ("+size+"x"+size+")";


array(array(float)) mkmatrix(int rows, int cols) 
{
   return map(enumerate(rows*cols,1,0),
	      lambda(int f, float den)
	      {
		if (f & 1)
		  return -((float)f)/den;
		return ((float)f)/den;
	      }, (float)rows*cols)/cols;
}

Math.Matrix gm1 = Math.Matrix(mkmatrix(size, size));
Math.Matrix gm2 = Math.Matrix(mkmatrix(size, size));

void test(int n)
{
    Math.Matrix m1 = gm1;
    Math.Matrix m2 = gm2;

    while (n--) m1=m1*m2;
    //   array q = (array(array(int)))(array)m1;
}

int perform()
{
   test(100);
   return 200 * 100 * 100 * 100;
}

string present_n( int ntot, int nruns, float real, float user )
{
    return sprintf("%.2f GF/s", (ntot/real/1000000000));
}
