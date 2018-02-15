/*
 * base class for tests
 *
 */

#pike __REAL_VERSION__

//! The name of the test.
constant name="?";

//! perform() is the function called in the tests,
//! when it returns the test is complete.
//!
//! The function returns the number of whatever the test did.
int perform();


optional string present_n(int ntot, int nruns, float tseconds, float useconds,  int memusage);
