/*
 * $Id: pikecode.c,v 1.3 2001/07/26 21:04:13 marcus Exp $
 *
 * Generic strap for the code-generator.
 *
 * Henrik Grubbström 20010720
 */

#include "global.h"
#include "program.h"
#include "opcodes.h"
#include "docode.h"
#include "interpret.h"
#include "language.h"
#include "lex.h"

#include "pikecode.h"

#ifdef PIKE_USE_MACHINE_CODE
#if defined(__i386__) || defined(__i386)
#include "code/ia32.c"
#elif defined(sparc) || defined(__sparc__) || defined(__sparc)
#include "code/sparc.c"
#elif defined(__ppc__) || defined(_POWER)
#include "code/ppc32.c"
#else /* Unsupported cpu */
#error Unknown CPU
#endif /* CPU type */
#elif defined(HAVE_COMPUTED_GOTO)
#include "code/computedgoto.c"
#else /* Default */
#include "code/bytecode.c"
#endif /* Interpreter type. */
