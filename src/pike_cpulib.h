/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: pike_cpulib.h,v 1.12 2004/06/02 00:08:12 nilsson Exp $
*/

#ifndef PIKE_CPULIB_H
#define PIKE_CPULIB_H

/* FIXME: Should we have an #ifdef PIKE_RUN_UNLOCKED here? -Hubbe
 * Good side: No problems compiling unless --run-unlocked is used
 * Bad side: Can't use these things for other purposes..
 */
/* Since the memlock stuff doesn't work yet, I disable the code for now.
 *	/grubba
 */
#ifdef PIKE_RUN_UNLOCKED

#define pike_atomic_fool_gcc(x) (*(volatile struct { int a[100]; } *)x)


/* These will work on any cpu, but should only be used as a
 * last resort. We must also define PIKE_NEED_MEMLOCK if we
 * use these. /Hubbe
 */
#define BEGIN_MEMORY_LOCK(ADDR) do { \
    PIKE_MUTEX_T *pike_mem_mutex=pike_memory_locks+ \
        ((unsigned long)(ADDR))%PIKE_MEM_HASH; \
    mt_lock(pike_mem_mutex)

#define END_MEMORY_LOCK()  \
    mt_unlock(pike_mem_mutex);  \
  }while(0)

#define pike_memlock(ADDR) do{				\
    PIKE_MUTEX_T *pike_mem_mutex=pike_memory_locks+	\
        ((unsigned long)(ADDR))%PIKE_MEM_HASH;		\
    mt_lock(pike_mem_mutex)				\
}while(0)

#define pike_unmemlock(ADDR) do{			\
    PIKE_MUTEX_T *pike_mem_mutex=pike_memory_locks+	\
        ((unsigned long)(ADDR))%PIKE_MEM_HASH;		\
    mt_unlock(pike_mem_mutex)				\
}while(0)

#define PIKE_NEED_MEMLOCK

/* Autoconf this? */
#if defined(__i386__) && defined(__GNUC__)

/*
 * ia32 general purpose registers
 * "a" = %eax, "b" = %edx, "c" = %ecx, "d" = %edx
 * "S" = %esi, "D" = %edi, "B" = %ebp
 */


#define PIKE_HAS_INC32
static INLINE void pike_atomic_inc32(volatile INT32 *v)
{
  __asm__ __volatile__("lock ; incl %0"
		       :"=m" (pike_atomic_fool_gcc(v))
		       :"m" (pike_atomic_fool_gcc(v))
                       :"cc");
}


#define PIKE_HAS_DEC_AND_TEST32
static INLINE int pike_atomic_dec_and_test32(INT32 *v)
{
  unsigned char c;

  __asm__ __volatile__("lock ; decl %0; setne %1"
		       :"=m" (pike_atomic_fool_gcc(v)), "=qm" (c)
		       :"m" (pike_atomic_fool_gcc(v))
                       :"cc");
  return c;
}

#define PIKE_HAS_COMPARE_AND_SWAP32
static INLINE int
pike_atomic_compare_and_swap32 (INT32 *p, INT32 oldval, INT32 newval)
{
  char ret;
  INT32 readval;

  __asm__ __volatile__ ("lock; cmpxchgl %3, %1; sete %0"
			: "=q" (ret), "=m" (*p), "=a" (readval)
			: "r" (newval), "m" (*p), "a" (oldval)
			: "cc", "memory");
  return ret;
}


/* NOT USED */
#define PIKE_HAS_COMPARE_AND_SWAP64
static INLINE int
pike_atomic_compare_and_swap64 (INT64 *p, INT64 oldval, INT64 newval)
{
  char ret;
#define HIGH(X) ((INT32 *)&(X))[1]
#define LOW(X) ((INT32 *)&(X))[0]

#if 1
  INT32 t1, t2, t3;
/*fprintf(stderr,"cas64(%p(%llx),%llx,%llx) << %d\n",p,*p,oldval,newval,ret);*/
#if 0
  __asm__ __volatile__ (
    "xchgl %%ebx, %%edi ;"
    "lock; cmpxchg8b (%%esi); sete %%al;"
    "movl  %%edi, %%ebx; "
    "movb  %%al, %0 "
    :"=q" (ret),
     "=a" (t1),"=d" (t2),"=D" (t3)
    :"S" (p),
     "1" (LOW(oldval)), "2" (HIGH(oldval)),
     "3" (LOW(newval)), "c" (HIGH(newval))
    :"cc", "memory");
#else
  __asm__ __volatile__ (
    "xchgl %%ebx, %%edi ;"
    "lock; cmpxchg8b (%%esi); sete %%al;"
    "movl  %%edi, %%ebx; "
    "movb  %%al, %0 "
    :"=q" (ret),
     "=a" (t1),"=d" (t2),"=D" (t3)
    :"S" (p),
     "a" (LOW(oldval)), "d" (HIGH(oldval)),
     "D" (LOW(newval)), "c" (HIGH(newval))
    :"cc", "memory");
#endif
/*fprintf(stderr,"cas64(%p(%llx),%llx,%llx) => %d\n",p,*p,oldval,newval,ret);*/
#else
  INT32 readval;
  /* this doesn't work because GCC use %ebp for PIC code */
  __asm__ __volatile__ ("lock; cmpxchg8b (%1); sete %0"
			:"=q" (ret), "=m" (*p)
			:"r" (newval), "m" (*p),
			 "a" (LOW(oldval)),"d" (HIGH(oldval)),
			 "b" (LOW(newval)),"c" (HIGH(newval))
			: "cc", "memory");
#endif
  return ret;
}

#endif /* __i386__ && __GNUC__ */

#if defined(__ia64__) && defined(__GNUC__)

#define PIKE_HAS_COMPARE_AND_SWAP32

static INLINE int
pike_atomic_compare_and_swap32 (INT32 *p, INT32 oldval, INT32 newval)
{
  INT32 ret;

  __asm__ __volatile__ ("mov ar.ccv = %1;\n"
			"\t;;\n"
			"\tcmpxchg4.rel.nta %0 = [%2], %3, ar.ccv"
			: "=r" (ret)
			: "r" (oldval), "r" (p), "r" (newval)
			: "memory", "ar.ccv");
  return ret == oldval;
}

#endif /* __ia64__ && __GNUC__ */

#if defined(_M_IA64)

#define PIKE_HAS_COMPARE_AND_SWAP32

static INLINE int
pike_atomic_compare_and_swap32 (INT32 *p, INT32 oldval, INT32 newval)
{
  return _InterlockedCompareExchange(p, newval, oldval) == oldval;
}

#endif /* _M_IA64 */

#if defined(__sparc_v9__) && defined(__GNUC__)

#define PIKE_HAS_COMPARE_AND_SWAP32

static INLINE int
pike_atomic_compare_and_swap32 (INT32 *p, INT32 oldval, INT32 newval)
{
  __asm__ __volatile__ ("membar #LoadStore\n"
			"cas [%2], %3, %0"
			: "=r" (newval)
			: "0" (newval), "r" (p), "r" (oldval)
			: "memory");
  return newval == oldval;
}

#endif /* __sparc_v9__ && __GNUC__ */

#if defined(__m68k__) && defined(__GNUC__)


#define PIKE_HAS_COMPARE_AND_SWAP32

static INLINE int
pike_atomic_compare_and_swap32 (INT32 *p, INT32 oldval, INT32 newval)
{
  INT32 cmpval = oldval;

  /* if (oldval == *p) {
   *   *p = newval;
   * } else {
   *   oldval = *p;
   * }
   */
  __asm__ __volatile__ ("casl %0, %3, %1;"
			: "=d" (oldval), "=m", (*p)
			: "0" (oldval), "d" (newval), "0" (p)
			: "memory");
  return oldval == cmpval;
}

#endif /* __m68k__ && __GNUC__ */



/*
 * These functions are used if we don't have
 * compare-and-swap instructions available
 */
#ifndef PIKE_HAS_COMPARE_AND_SWAP32

#define PIKE_HAS_COMPARE_AND_SWAP32
#undef PIKE_NEED_MEMLOCK
#define PIKE_NEED_MEMLOCK

#include "threads.h"

static INLINE int
pike_atomic_compare_and_swap32 (INT32 *p, INT32 oldval, INT32 newval)
{
  int ret;
  BEGIN_MEMORY_LOCK(p);
  if(ret == (*p == oldval)) *p=newval;
  END_MEMORY_LOCK();
  return ret;
}


#ifndef PIKE_HAS_INC32
#define PIKE_HAS_INC32
static INLINE void pike_atomic_inc32(INT32 *ref)
{
  BEGIN_MEMORY_LOCK(p);
  ref++;
  END_MEMORY_LOCK();
}
#endif /* PIKE_HAS_INC32 */

#ifndef PIKE_HAS_DEC_AND_TEST32
#define PIKE_HAS_DEC_AND_TEST32
static INLINE int pike_atomic_dec_and_test32(INT32 *ref)
{
  INT32 ret;
  BEGIN_MEMORY_LOCK(p);
  ret=--ref;
  END_MEMORY_LOCK();
  return ret;
}
#endif /*  PIKE_HAS_DEC_AND_TEST32 */



#ifndef PIKE_HAS_SWAP32
#define PIKE_HAS_SWAP32
static INLINE INT32 pike_atomic_swap32(INT32 *addr, INT32 newval)
{
  INT32 ret;
  BEGIN_MEMORY_LOCK(p);
  ret=*addr;
  *addr=newval;
  END_MEMORY_LOCK();
  return ret;
}
#endif

#endif /* PIKE_HAS_COMPARE_AND_SWAP32 */

#if !defined(PIKE_HAS_COMPARE_AND_SWAP64) && defined(INT64)

#define PIKE_HAS_COMPARE_AND_SWAP64
#undef PIKE_NEED_MEMLOCK
#define PIKE_NEED_MEMLOCK

#include "threads.h"

static INLINE int
pike_atomic_compare_and_swap64 (INT64 *p, INT64 oldval, INT64 newval)
{
  int ret;
  BEGIN_MEMORY_LOCK(p);
  if(ret == (*p == oldval)) *p=newval;
  END_MEMORY_LOCK();
  return ret;
}


#ifndef PIKE_HAS_SWAP64
#define PIKE_HAS_SWAP64
static INLINE INT64 pike_atomic_swap64(INT64 *addr, INT64 newval)
{
  INT64 ret;
  BEGIN_MEMORY_LOCK(p);
  ret=*addr;
  *addr=newval;
  END_MEMORY_LOCK();
  return ret;
}
#endif


#ifndef PIKE_HAS_SET64
#define PIKE_HAS_SET64
static INLINE void pike_atomic_set64(INT64 *addr, INT64 newval)
{
  BEGIN_MEMORY_LOCK(p);
  *addr=newval;
  END_MEMORY_LOCK();
}
#endif

#ifndef PIKE_HAS_GET64
#define PIKE_HAS_GET64
static INLINE INT64 pike_atomic_get64(INT64 *addr)
{
  INT64 ret;
  BEGIN_MEMORY_LOCK(p);
  ret=*addr;
  END_MEMORY_LOCK();
  return ret;
}
#endif


#endif /* PIKE_HAS_COMPARE_AND_SWAP64 */



/* These functions are used if we have
 * compare-and-swap, but not the rest of
 * the atomic functions
 */

#if defined(PIKE_HAS_COMPARE_AND_SWAP32) && !defined(PIKE_HAS_INC32)
#define PIKE_HAS_INC32
static INLINE void pike_atomic_inc32(INT32 *ref)
{
  INT32 oldrefs;
  do 
    oldrefs=*ref;
  while(! pike_atomic_compare_and_swap32(ref, oldrefs, oldrefs+1));
}
#endif


#if defined(PIKE_HAS_COMPARE_AND_SWAP32) && !defined(PIKE_HAS_DEC_AND_TEST32)
#define PIKE_HAS_DEC_AND_TEST32
static INLINE int pike_atomic_dec_and_test32(INT32 *ref)
{
  INT32 oldrefs;
  do 
    oldrefs=*ref;
  while(! pike_atomic_compare_and_swap32(ref, oldrefs, oldrefs-1));
  return oldrefs-1;
}
#endif


#if defined(PIKE_HAS_COMPARE_AND_SWAP32) && !defined(PIKE_HAS_SWAP32)
#define PIKE_HAS_SWAP32
static INLINE INT32 pike_atomic_swap32(INT32 *addr, INT32 newval)
{
  INT32 ret;
  do
  {
    ret=*addr;
  }while(!pike_atomic_compare_and_swap32(addr, ret, newval));
  return ret;
}
#endif


#if defined(PIKE_HAS_COMPARE_AND_SWAP64) && !defined(PIKE_HAS_SWAP64)
#define PIKE_HAS_SWAP64
static INLINE INT64 pike_atomic_swap64(INT64 *addr, INT64 newval)
{
  INT64 ret;
  do
  {
    ret=*addr;
  }while(!pike_atomic_compare_and_swap64(addr, ret, newval));
  return ret;
}
#endif



#if defined(PIKE_HAS_SWAP64) && !defined(PIKE_HAS_SET64)
#define PIKE_HAS_SET64
static INLINE void pike_atomic_set64(INT64 *addr, INT64 newval)
{
  pike_atomic_swap64(addr, newval);
}
#endif

#if defined(PIKE_HAS_COMPARE_AND_SWAP64) && !defined(PIKE_HAS_GET64)
#define PIKE_HAS_GET64
static INLINE INT64 pike_atomic_get64(INT64 *addr)
{
  INT64 ret;
  do
  {
    ret=*addr;
  }while(!pike_atomic_compare_and_swap64(addr, ret, ret));
  return ret;
}
#endif

/* End emulating functions */

#endif /* PIKE_RUN_UNLOCKED */

#ifdef PIKE_NEED_MEMLOCK
extern void init_pike_cpulib(void);
#else
#define init_pike_cpulib()
#endif

#endif /* PIKE_CPULIB_H */
