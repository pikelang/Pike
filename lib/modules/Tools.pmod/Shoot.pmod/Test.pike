/*
 * base class for tests
 *
 */

//! The name of the test.
constant name="?";

//! perform() is the function called in the tests,
//! when it returns the test is complete.
void perform();

// optional:
optional int n;
optional string present_n(int ntot,int nruns,
			  float tseconds,float useconds,
			  int memusage);
