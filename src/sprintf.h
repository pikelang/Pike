#ifndef SPRINTF_H
#define SPRINTF_H
#include "global.h"

#include "string_builder.h"

/* Prototypes begin here */
PMOD_EXPORT void f_sprintf(INT32 num_arg);
void f___handle_sprintf_format(INT32 args);
void low_f_sprintf(INT32 args, struct string_builder *r);
void init_sprintf(void);
void exit_sprintf(void);
/* Prototypes end here */

#endif /* SPRINTF_H */
