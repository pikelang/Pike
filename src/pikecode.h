/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/*
 * Generic headerfile for the code-generator.
 *
 * Henrik Grubbström 20010720
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
void ins_f_byte_with_arg(unsigned int a,unsigned INT32 b);
void ins_f_byte_with_2_args(unsigned int a,
			    unsigned INT32 c,
			    unsigned INT32 b);

#if PIKE_BYTECODE_METHOD == PIKE_BYTECODE_IA32
#include "code/ia32.h"
#elif PIKE_BYTECODE_METHOD == PIKE_BYTECODE_SPARC
#include "code/sparc.h"
#elif PIKE_BYTECODE_METHOD == PIKE_BYTECODE_PPC32
#include "code/ppc32.h"
#elif PIKE_BYTECODE_METHOD == PIKE_BYTECODE_GOTO
#include "code/computedgoto.h"
#else
#include "code/bytecode.h"
#endif

#ifndef CHECK_RELOC
#define CHECK_RELOC(REL, PROG_SIZE)
#endif /* !CHECK_RELOC */

#endif /* CODE_PIKECODE_H */
