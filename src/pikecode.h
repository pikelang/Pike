/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: pikecode.h,v 1.15 2007/03/31 22:59:53 marcus Exp $
*/

/*
 * Generic headerfile for the code-generator.
 *
 * Henrik Grubbstr�m 20010720
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
#warning using amd64
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
#elif PIKE_BYTECODE_METHOD == PIKE_BYTECODE_GOTO
#include "code/computedgoto.h"
#define PIKE_BYTECODE_METHOD_NAME	"computed_goto"
#else
#include "code/bytecode.h"
#define PIKE_BYTECODE_METHOD_NAME	"default"
#endif

#ifndef CHECK_RELOC
#define CHECK_RELOC(REL, PROG_SIZE)
#endif /* !CHECK_RELOC */

#endif /* CODE_PIKECODE_H */
