/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: main.h,v 1.28 2005/11/23 10:40:40 grubba Exp $
*/

#ifndef MAIN_H
#define MAIN_H

/* Prototypes begin here */
int main(int argc, char **argv);
DECLSPEC(noreturn) void pike_do_exit(int num) ATTRIBUTE((noreturn));
/* Prototypes end here */

#endif /* !MAIN_H */
