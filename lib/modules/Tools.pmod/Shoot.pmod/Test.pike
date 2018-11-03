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
int perform(mixed|void context);

//! If this function exists, it computes the context to pass to
//! the perform() function.  The time consumed by this function will
//! not count towards the test.
optional mixed prepare();

optional string present_n(int ntot, int nruns, float tseconds, float useconds,  int memusage);
