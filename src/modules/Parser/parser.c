/*
 * $Id: parser.c,v 1.1 1999/02/19 04:58:40 mirar Exp $
 */

#include "global.h"
#include "config.h"

#include "program.h"

#include "parser.h"

/*** module init & exit & stuff *****************************************/

/* add other parsers here */

static struct parser_class
{
   char *name;
   void (*func)(void);
} sub[] = {
   {"HTML",init_parser_html},
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
      fprintf(stderr,"%d %x\n",i,sub[i].func);
      sub[i].func();
      p=end_program();
      add_program_constant(sub[i].name,p,0);
      free_program(p);
   }
}
