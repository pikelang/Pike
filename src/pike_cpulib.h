#ifndef PIKE_CPULIB_H
#define PIKE_CPULIB_H


/* Autoconf this? */
#if defined(__i386__) && defined(__GNUC__)

#define PIKE_HAS_COMPARE_AND_SWAP

static inline int
pike_atomic_compare_and_swap (INT32 *p, INT32 oldval, INT32 newval)
{
  char ret;
  INT32 readval;

  __asm__ __volatile__ ("lock; cmpxchgl %3, %1; sete %0"
			: "=q" (ret), "=m" (*p), "=a" (readval)
			: "r" (newval), "m" (*p), "a" (oldval)
			: "memory");
  return ret;
}

#endif




#ifdef PIKE_HAS_COMPARE_AND_SWAP
static inline INT32 pike_atomic_add_ref(INT32 *ref)
{
  INT32 oldrefs;
  do 
    oldrefs=*ref;
  while(! pike_atomic_compare_and_swap(ref, oldrefs, oldrefs+1));
  return oldrefs;
}

static inline INT32 pike_atomic_sub_ref(INT32 *ref)
{
  INT32 oldrefs;
  do 
    oldrefs=*ref;
  while(! pike_atomic_compare_and_swap(ref, oldrefs, oldrefs-1));
  return oldrefs-1;
}

#endif

#endif
