/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: main.h,v 1.25 2005/01/01 14:35:45 grubba Exp $
*/

#ifndef MAIN_H
#define MAIN_H

#include "callback.h"

/* Prototypes begin here */
PMOD_EXPORT struct callback *add_post_master_callback(callback_func call,
					  void *arg,
					  callback_func free_func);
PMOD_EXPORT struct callback *add_exit_callback(callback_func call,
				   void *arg,
				   callback_func free_func);
int main(int argc, char **argv);
DECLSPEC(noreturn) void pike_do_exit(int num) ATTRIBUTE((noreturn));
/* Prototypes end here */

#endif /* !MAIN_H */
