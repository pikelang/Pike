/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef PIKE_CPULIB_H
#define PIKE_CPULIB_H

#define bit_SSE4_2 (1<<20)
#define bit_RDRND_2 (1<<30)

#ifdef __GNUC__
#  ifdef __i386__
#    define HAVE_X86_ASM
#    define HAVE_IA32_ASM
#    define GCC_X86_ASM_STYLE
#    define GCC_IA32_ASM_STYLE
#  elif defined (__amd64__) || defined (__x86_64__)
#    define HAVE_X86_ASM
#    define HAVE_X86_64_ASM
#    define GCC_X86_ASM_STYLE
#    define GCC_X86_64_ASM_STYLE
#  endif
#elif defined (_MSC_VER)
#  ifdef _M_IX86
#    define HAVE_X86_ASM
#    define HAVE_IA32_ASM
#    define CL_X86_ASM_STYLE
#    define CL_IA32_ASM_STYLE
#  elif defined (_M_X64)
#    define HAVE_X86_ASM
#    define HAVE_X86_64_ASM
#    define CL_X86_ASM_STYLE
#    define CL_X86_64_ASM_STYLE
#  endif
#endif

#ifdef HAVE_X86_ASM
PMOD_EXPORT void x86_get_cpuid(int oper, INT32 *cpuid_ptr);
#endif

#ifdef HAVE_RDTSC
#ifdef GCC_X86_ASM_STYLE
#define RDTSC(v)  do {					    \
   unsigned INT32 __l, __h;                                 \
   __asm__ __volatile__ ("rdtsc" : "=a" (__l), "=d" (__h)); \
   (v)= __l | (((INT64)__h)<<32);                           \
} while (0)
#endif
#ifdef CL_X86_ASM_STYLE
#define RDTSC(v) do {							\
    unsigned INT32 l, h;						\
    __asm {rdtsc}							\
    __asm {mov l, eax}							\
    __asm {mov h, edx}							\
    v = l | ((INT64) h << 32);						\
  } while (0)
#endif
#endif

PMOD_EXPORT int my_log2(UINT64 x) ATTRIBUTE((const));

#endif /* PIKE_CPULIB_H */
