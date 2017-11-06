/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "pike_cpulib.h"
#include "svalue.h"

#ifdef HAVE_X86_ASM

#if !defined (CL_X86_ASM_STYLE) && !defined (GCC_X86_ASM_STYLE)
#error Dont know how to inline assembler with this compiler
#endif

PMOD_EXPORT void x86_get_cpuid(int oper, INT32 *cpuid_ptr)
/* eax -> cpuid_ptr[0]
 * ebx -> cpuid_ptr[1]
 * edx -> cpuid_ptr[2]
 * ecx -> cpuid_ptr[3] */
{
#ifdef CL_X86_ASM_STYLE
  __asm {
    mov eax, oper;
    mov edi, cpuid_ptr;
    cpuid;
    mov [edi], eax;
    mov [edi+4], ebx;
    mov [edi+8], edx;
    mov [edi+12], ecx;
  };
#else  /* GCC_X86_ASM_STYLE */

#if SIZEOF_CHAR_P == 4
  __asm__ __volatile__("pushl %%ebx      \n\t" /* save %ebx */
                 "cpuid            \n\t"
                 "movl %%ebx, %1   \n\t" /* save what cpuid just put in %ebx */
                 "popl %%ebx       \n\t" /* restore the old %ebx */
                 : "=a"(cpuid_ptr[0]),
                   "=r"(cpuid_ptr[1]),
                   "=d"(cpuid_ptr[2]),
                   "=c"(cpuid_ptr[3])
                 : "0"(oper)
                 : "cc");
#else
  __asm__ __volatile__("push %%rbx      \n\t" /* save %rbx */
                 "cpuid            \n\t"
                 "movl %%ebx, %1   \n\t" /* save what cpuid just put in %ebx */
                 "pop %%rbx       \n\t" /* restore the old %rbx */
                 : "=a"(cpuid_ptr[0]),
                   "=r"(cpuid_ptr[1]),
                   "=d"(cpuid_ptr[2]),
                   "=c"(cpuid_ptr[3])
                 : "0"(oper)
                 : "cc");
#endif /* SIZEOF_CHAR_P == 4 */
#endif /* CL_X86_ASM_STYLE */
}

#endif	/* HAVE_IA32_ASM */

/* Same thing as (int)floor(log((double)x) / log(2.0)), except a bit
   quicker. Number of log2 per second:

   log(x)/log(2.0)             50,000,000
   log2(x)                     75,000,000
   Table lookup             3,000,000,000
   Intrinsic       30,000,000,000,000,000
*/

PMOD_EXPORT int my_log2(UINT64 x)
{
    if( x == 0 ) return 0;
    if(x & ~((UINT64)0xffffffffUL)) {
      return 32 + log2_u32((unsigned INT32)(x>>32));
    }
    return log2_u32((unsigned INT32)x);
}
