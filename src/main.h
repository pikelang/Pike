/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: main.h,v 1.26 2005/01/01 17:35:54 grubba Exp $
*/

#ifndef MAIN_H
#define MAIN_H

/* Prototypes begin here */
int main(int argc, char **argv);
DECLSPEC(noreturn) void pike_do_exit(int num) ATTRIBUTE((noreturn));
/* Prototypes end here */

#endif /* !MAIN_H */
