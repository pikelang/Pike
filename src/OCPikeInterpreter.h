/* Standard Pike include files. */
#include "bignum.h"
#include "array.h"
#include "builtin_functions.h"
#include "constants.h"
#include "interpret.h"
#include "mapping.h"
#include "multiset.h"
#include "module_support.h"
#include "object.h"
#include "pike_macros.h"
#include "pike_types.h"
#include "program.h"
#include "stralloc.h"
#include "svalue.h"
#include "pike_threads.h"
#include "version.h"
#include "operators.h"

/* Objective-C includes */
#import <Foundation/NSObject.h>
#import <Foundation/NSException.h>

PMOD_EXPORT
@interface OCPikeInterpreter : NSObject
{
  int is_started;
  id master_location;
  JMP_BUF jmploc;
}

+ (OCPikeInterpreter *)sharedInterpreter;
+ (id)allocWithZone:(NSZone *)zone;
- (id)copyWithZone:(NSZone *)zone;
- (id)retain;
- (void)release;
- (id)autorelease;
- (NSUInteger)retainCount;
- (void)setMaster:(id)master;
- (BOOL)startInterpreter;
- (BOOL)isStarted;
- (BOOL)stopInterpreter;
- (struct Pike_interpreter_struct *) getInterpreter;
- (struct program *)compileString: (id)code;
- (struct svalue *)evalString: (id)expression;

@end /* interface OCPikeInterpreter */
