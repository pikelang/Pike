#ifndef CONFIG_H
#define CONFIG_H

#include "machine.h"

/*
 * Define DEBUG and be sure to compile with -g if you want to debug uLPC
 * with DEBUG defined debugging becomes much easier.
 */

#undef DEBUG

/*
 * Define the evaluator stack size, used for just about everything.
 */
#define EVALUATOR_STACK_SIZE	50000

/*
 * The compiler stack is used when compiling to keep track of data.
 * This value need too be large enough for the programs you compile.
 */
#define COMPILER_STACK_SIZE	4000

/*
 * Max number of local variables in a function.
 * Currently there is no support for more than 256
 */
#define MAX_LOCAL	128

/*
 * Define the size of the shared string hash table.
 */
#define	HTABLE_SIZE 4711

/*
 * Define the size of the cache that is used for method lookup.
 */
#define FIND_FUNCTION_HASHSIZE 4711
   

/*
 * Do we want YYDEBUG?
 */

#ifdef DEBUG
#define YYDEBUG 1
#endif

/* Not parently used */
#define GC_TIME 60

#endif
