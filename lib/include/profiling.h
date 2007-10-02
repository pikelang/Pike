//
// $Id: profiling.h,v 1.2 2007/10/02 20:08:48 nilsson Exp $

#ifdef PROF_REALTIME

private mapping __prof_data = ([]);
private System.Timer __prof_timer = System.Timer();

#define PROF_BEGIN(X) __prof_data[X] = __prof_timer->peek()
#define PROF_END(X)   werror("%s : %3.3f\n", X, __prof_timer->peek()-__prof_data[X])
#define PROF_RESULT()

#elif defined(PROF_OFF)

#define PROF_BEGIN(X)
#define PROF_END(X)
#define PROF_RESULT()

#else

private mapping __prof_data = ([]);
private System.Timer __prof_timer = System.Timer();

#define PROF_BEGIN(X) __prof_data[X]-=__prof_timer->peek()
#define PROF_END(X)   __prof_data[X]+=__prof_timer->peek()
#define PROF_RESULT() foreach(__prof_data; string idx; int val) \
  werror("%15s : %3.3f\n", idx, val);

#endif
