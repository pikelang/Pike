/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef CONFIG_H
#define CONFIG_H

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
#define MAX_LOCAL	256

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
