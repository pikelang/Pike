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

#endif /* __i386__ && __GNUC__ */

#if defined(__ia64__) && defined(__GNUC__)

#define PIKE_HAS_COMPARE_AND_SWAP

static inline int
pike_atomic_compare_and_swap (INT32 *p, INT32 oldval, INT32 newval)
{
  INT32 ret;

  register ptrdiff_t cmpval __asm__("ar.ccv") = oldval;

  __asm__ __volatile__ ("mov ar.ccv = %1;\n"
			"\t;;\n"
			"\tcmpxchg4.rel.nta %0 = [%2], %3, ar.ccv"
			: "=r" (ret)
			: "r" (cmpval), "r" (p), "r" (newval)
			: "memory", "ar.ccv");
  return ret;
}

#endif /* __i386__ && __GNUC__ */

#if defined(__sparc_v9__) && defined(__GNUC__)

#define PIKE_HAS_COMPARE_AND_SWAP

static inline int
pike_atomic_compare_and_swap (INT32 *p, INT32 oldval, INT32 newval)
{
  __asm__ __volatile__ ("membar #LoadStore\n"
			"cas [%2], %3, %0"
			: "=r" (newval)
			: "0" (newval), "r" (p), "r" (oldval)
			: "memory");
  return newval;
}

#endif /* __sparc_v9__ && __GNUC__ */

#if defined(__m68k__) && defined(__GNUC__)


#define PIKE_HAS_COMPARE_AND_SWAP

static inline int
pike_atomic_compare_and_swap (INT32 *p, INT32 oldval, INT32 newval)
{
  /* if (oldval == *p) {
   *   *p = newval;
   * } else {
   *   oldval = *p;
   * }
   */
  __asm__ __volatile__ ("casl %0, %3, %1"
			: "=d" (oldval), "=m", (*p)
			: "0" (oldval), "d" (newval), "0" (p)
			: "memory");
  return oldval;
}

#endif /* __m68k__ && __GNUC__ */

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
