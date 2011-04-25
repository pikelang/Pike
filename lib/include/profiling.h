//
// $Id$

#ifdef PROF_REALTIME

private mapping __prof_data = ([]);

#define PROF_BEGIN(X) __prof_data[X] = gethrtime()
#define PROF_END(X)   werror("%s : %3.3f\n", X, \
  (gethrtime()-__prof_data[X])/1000000.0)
#define PROF_RESULT()

#elif defined(PROF_OFF)

#define PROF_BEGIN(X)
#define PROF_END(X)
#define PROF_RESULT()

#else

private mapping __prof_data = ([]);

#define PROF_BEGIN(X) __prof_data[X]-=gethrtime()
#define PROF_END(X)   __prof_data[X]+=gethrtime()
#define PROF_RESULT() foreach(__prof_data; string idx; int val) \
  werror("%15s : %3.3f\n", idx, val/1000000.0);

#endif
