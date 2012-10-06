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
#ifdef HAVE_X86_64_ASM
#define cpuid_supported 1
#else  /* HAVE_IA32_ASM */
  static int cpuid_supported = 0;
  if (!cpuid_supported) {
    int fbits=0;
#ifdef CL_X86_ASM_STYLE
    __asm {
      pushf
      pop  eax
      mov  ecx, eax
      xor  eax, 00200000h
      push eax
      popf
      pushf
      pop  eax
      xor  ecx, eax
      mov  fbits, ecx
    };
#else  /* GCC_X86_ASM_STYLE */
    /* Note: gcc swaps the argument order... */
    __asm__("pushf\n\t"
	    "pop  %%eax\n\t"
	    "movl %%eax, %%ecx\n\t"
	    "xorl $0x00200000, %%eax\n\t"
	    "push %%eax\n\t"
	    "popf\n\t"
	    "pushf\n\t"
	    "pop  %%eax\n\t"
	    "xorl %%eax, %%ecx\n\t"
	    "movl %%ecx, %0"
	    : "=m" (fbits)
	    :
	    : "cc", "eax", "ecx");
#endif
    if (fbits & 0x00200000) {
      cpuid_supported = 1;
    } else {
      cpuid_supported = -1;
    }
  }
#endif	/* HAVE_IA32_ASM */

  if (cpuid_supported > 0) {
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
                   "=c"(cpuid_ptr[2]), 
                   "=d"(cpuid_ptr[3])
                 : "0"(oper)
                 : "cc");
#else
    __asm__ __volatile__("push %%rbx      \n\t" /* save %rbx */
                 "cpuid            \n\t"
                 "movl %%ebx, %1   \n\t" /* save what cpuid just put in %ebx */
                 "pop %%rbx       \n\t" /* restore the old %rbx */
                 : "=a"(cpuid_ptr[0]),
                   "=r"(cpuid_ptr[1]),
                   "=c"(cpuid_ptr[2]),
                   "=d"(cpuid_ptr[3])
                 : "0"(oper)
                 : "cc");
#endif
#endif
  } else {
    cpuid_ptr[0] = cpuid_ptr[1] = cpuid_ptr[2] = cpuid_ptr[3] = 0;
  }
}

#endif	/* HAVE_IA32_ASM */

#ifdef PIKE_NEED_MEMLOCK

#define PIKE_MEM_HASH 17903
PIKE_MUTEX_T pike_memory_locks[PIKE_MEM_HASH];

void init_pike_cpulib(void)
{
  int e;
  for(e=0;e<PIKE_MEM_HASH;e++)
    mt_init_recursive(pike_memory_locks+e);
}

#endif
