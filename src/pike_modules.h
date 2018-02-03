/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef PIKE_MODULES_H
#define PIKE_MODULES_H

struct static_module;
const struct static_module *find_semidynamic_module(const char *name,
						    int namelen);
void *get_semidynamic_init_fun(const struct static_module *sm);
void *get_semidynamic_exit_fun(const struct static_module *sm);
void init_modules(void);
void exit_modules(void);

#endif
