/*
 * $Id: pikecode.h,v 1.3 2001/07/23 12:19:10 grubba Exp $
 *
 * Generic headerfile for the code-generator.
 *
 * Henrik Grubbström 20010720
 */

#ifndef CODE_PIKECODE_H
#define CODE_PIKECODE_H

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

/* Byte-code method identification. */
#define PIKE_BYTECODE_DEFAULT	0
#define PIKE_BYTECODE_GOTO	1
#define PIKE_BYTECODE_SPARC	2
#define PIKE_BYTECODE_IA32	3

/* Note: PIKE_BYTECODE_METHOD gets defined to one of the above values below. */

#ifdef PIKE_USE_MACHINE_CODE
#if defined(__i386__) || defined(__i386)
#define PIKE_BYTECODE_METHOD	PIKE_BYTECODE_IA32
#include "code/ia32.h"
#elif defined(sparc) || defined(__sparc__) || defined(__sparc)
#define PIKE_BYTECODE_METHOD	PIKE_BYTECODE_SPARC
#include "code/sparc.h"
#else /* Unsupported cpu */
#error Unknown CPU. Run configure --without-machine-code.
#endif /* CPU type */
#elif defined(HAVE_COMPUTED_GOTO)
#define PIKE_BYTECODE_METHOD	PIKE_BYTECODE_GOTO
#include "code/computedgoto.h"
#else /* Default */
#define PIKE_BYTECODE_METHOD	PIKE_BYTECODE_DEFAULT
#include "code/bytecode.h"
#endif /* Interpreter type. */

#ifndef PIKE_BYTECODE_METHOD
#error PIKE_BYTECODE_METHOD not defined!
#endif /* !PIKE_BYTECODE_METHOD */

#endif /* CODE_PIKECODE_H */
