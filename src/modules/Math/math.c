/*
 * $Id: math.c,v 1.2 1999/03/31 18:36:52 mirar Exp $
 */

#include "global.h"
#include "config.h"

#include "program.h"

#include "math.h"

/*** module init & exit & stuff *****************************************/

/* add other parsers here */

struct program *math_matrix_program;

static struct math_class
{
   char *name;
   void (*func)(void);
   struct program **pd;
} sub[] = {
   {"Matrix",init_math_matrix,&math_matrix_program},
};

void pike_module_exit(void)
{
   int i;
   for (i=0; i<(int)(sizeof(sub)/sizeof(sub[0])); i++)
      if (sub[i].pd && sub[i].pd[0])
	 free_program(sub[i].pd[0]);
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
      if (sub[i].pd) sub[i].pd[0]=p;
      else free_program(p);
   }
}
