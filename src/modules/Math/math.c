/*
 * $Id: math.c,v 1.1 1999/03/31 13:12:37 mirar Exp $
 */

#include "global.h"
#include "config.h"

#include "program.h"

#include "math.h"

/*** module init & exit & stuff *****************************************/

/* add other parsers here */

static struct math_class
{
   char *name;
   void (*func)(void);
} sub[] = {
   {"Matrix",init_math_matrix},
};

void pike_module_exit(void)
{
}

void pike_module_init(void)
{
   int i;
   
   for (i=0; i<(int)(sizeof(sub)/sizeof(sub[0])); i++)
   {
      struct program *p;

      start_new_program();
      sub[i].func();
      p=end_program();
      add_program_constant(sub[i].name,p,0);
      free_program(p);
   }
}
