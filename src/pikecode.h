/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/*
 * Generic headerfile for the code-generator.
 */

#ifndef CODE_PIKECODE_H
#define CODE_PIKECODE_H

#include "program.h"

void ins_pointer(INT32 ptr);
INT32 read_pointer(INT32 off);
void upd_pointer(INT32 off, INT32 ptr);

void ins_byte(INT32 val);
void ins_data(INT32 val);

void ins_align(INT32 align);

void ins_f_byte(unsigned int b);
void ins_f_byte_with_arg(unsigned int a, INT32 b);
void ins_f_byte_with_2_args(unsigned int a, INT32 c, INT32 b);

#if PIKE_BYTECODE_METHOD == PIKE_BYTECODE_AMD64
#ifdef __NT__
#warning using amd64
#endif
#endif

#if PIKE_BYTECODE_METHOD == PIKE_BYTECODE_IA32
#include "code/ia32.h"
#define PIKE_BYTECODE_METHOD_NAME	"ia32"
#elif PIKE_BYTECODE_METHOD == PIKE_BYTECODE_AMD64
#include "code/amd64.h"
#define PIKE_BYTECODE_METHOD_NAME	"amd64"
#elif PIKE_BYTECODE_METHOD == PIKE_BYTECODE_SPARC
#include "code/sparc.h"
#define PIKE_BYTECODE_METHOD_NAME	"sparc"
#elif PIKE_BYTECODE_METHOD == PIKE_BYTECODE_PPC32
#include "code/ppc32.h"
#define PIKE_BYTECODE_METHOD_NAME	"ppc32"
#elif PIKE_BYTECODE_METHOD == PIKE_BYTECODE_PPC64
#include "code/ppc64.h"
#define PIKE_BYTECODE_METHOD_NAME	"ppc64"
#elif PIKE_BYTECODE_METHOD == PIKE_BYTECODE_ARM32
#include "code/arm32.h"
#define PIKE_BYTECODE_METHOD_NAME	"arm32"
#elif PIKE_BYTECODE_METHOD == PIKE_BYTECODE_ARM64
#include "code/arm64.h"
#define PIKE_BYTECODE_METHOD_NAME	"arm64"
#elif PIKE_BYTECODE_METHOD == PIKE_BYTECODE_RV32
#include "code/riscv.h"
#define PIKE_BYTECODE_METHOD_NAME	"rv32"
#elif PIKE_BYTECODE_METHOD == PIKE_BYTECODE_RV64
#include "code/riscv.h"
#define PIKE_BYTECODE_METHOD_NAME	"rv64"
#else
#include "code/bytecode.h"
#define PIKE_BYTECODE_METHOD_NAME	"default"
#endif

#ifndef CHECK_RELOC
#define CHECK_RELOC(REL, PROG_SIZE)
#endif /* !CHECK_RELOC */

#endif /* CODE_PIKECODE_H */
