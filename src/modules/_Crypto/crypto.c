/*
 * $Id: crypto.c,v 1.1.1.1 1996/11/05 15:10:09 grubba Exp $
 *
 * A pike module for getting access to some common cryptos.
 *
 * Henrik Grubbström 1996-10-24
 */

/*
 * Includes
 */

/* From the Pike distribution */
#include "global.h"
#include "stralloc.h"
#include "interpret.h"
#include "svalue.h"
#include "constants.h"
#include "macros.h"
#include "threads.h"
#include "object.h"
#include "stralloc.h"
#include "interpret.h"
#include "builtin_functions.h"

/* System includes */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

/* Module specific includes */
#include "precompiled_crypto.h"

/*
 * Globals
 */

/*
 * Functions
 */

/*
 * efuns and the like
 */

/* string string_to_hex(string) */
static void f_string_to_hex(INT32 args)
{
  char *buffer;
  int i;

  if (args != 1) {
    error("Wrong number of arguments to string_to_hex()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to string_to_hex()\n");
  }

  if (!(buffer=alloca(sp[-1].u.string->len*2))) {
    error("string_to_hex(): Out of memory\n");
  }
  
  for (i=0; i<sp[-1].u.string->len; i++) {
    sprintf(buffer + i*2, "%02x", sp[-1].u.string->str[i] & 0xff);
  }
  
  pop_n_elems(args);
  
  push_string(make_shared_binary_string(buffer, i*2));
  sp[-1].u.string->refs++;
}

/* string hex_to_string(string) */
static void f_hex_to_string(INT32 args)
{
  char *buffer;
  int i;

  if (args != 1) {
    error("Wrong number of arguments to hex_to_string()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to hex_to_string()\n");
  }
  if (sp[-1].u.string->len & 1) {
    error("Bad string length to hex_to_string()\n");
  }
  if (!(buffer = alloca(sp[-1].u.string->len/2))) {
    error("hex_to_string(): Out of memory\n");
  }
  for (i=0; i*2<sp[-1].u.string->len; i++) {
    switch (sp[-1].u.string->str[i*2]) {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      buffer[i] = (sp[-1].u.string->str[i*2] - '0')<<4;
      break;
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
      buffer[i] = (sp[-1].u.string->str[i*2] + 10 - 'A')<<4;
      break;
    default:
      error("hex_to_string(): Illegal character (0x%02x) in string\n",
	    sp[-1].u.string->str[i*2] & 0xff);
    }
    switch (sp[-1].u.string->str[i*2+1]) {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      buffer[i] |= sp[-1].u.string->str[i*2+1] - '0';
      break;
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
      buffer[i] |= (sp[-1].u.string->str[i*2+1] + 10 - 'A') & 0x0f;
      break;
    default:
      error("hex_to_string(): Illegal character (0x%02x) in string\n",
	    sp[-1].u.string->str[i*2+1] & 0xff);
    }
  }

  pop_n_elems(args);

  push_string(make_shared_binary_string(buffer, i));
  sp[-1].u.string->refs++;
}

/*
 * Module linkage
 */

void init_module_efuns(void)
{
  /* add_efun()s */

  add_efun("string_to_hex", f_string_to_hex, "function(string:string)", OPT_TRY_OPTIMIZE);
  add_efun("hex_to_string", f_hex_to_string, "function(string:string)", OPT_TRY_OPTIMIZE);

  init_md2_efuns();
  init_md5_efuns();
  init_idea_efuns();
  init_des_efuns();
}

void init_module_programs(void)
{
  /*
   * start_new_program();
   *
   * add_storage();
   *
   * add_function();
   * add_function();
   * ...
   *
   * set_init_callback();
   * set_exit_callback();
   *
   * program = end_c_program();
   * program->refs++;
   *
   */
  init_md2_programs();
  init_md5_programs();
  init_idea_programs();
  init_des_programs();
}

void exit_module(void)
{
  /* free_program()s */
  exit_md2();
  exit_md5();
  exit_idea();
  exit_des();
}
