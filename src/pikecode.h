/*
 * $Id: pikecode.h,v 1.2 2001/07/20 16:27:53 grubba Exp $
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

#ifdef PIKE_USE_MACHINE_CODE
#if defined(__i386__) || defined(__i386)
#include "code/ia32.h"
#elif defined(sparc) || defined(__sparc__) || defined(__sparc)
#include "code/sparc.h"
#else /* Unsupported cpu */
#error Unknown CPU
#endif /* CPU type */
#elif defined(HAVE_COMPUTED_GOTO)
#include "code/computedgoto.h"
#else /* Default */
#include "code/bytecode.h"
#endif /* Interpreter type. */

#endif /* CODE_PIKECODE_H */
