/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: program_areas.h,v 1.13 2005/05/26 12:00:56 grubba Exp $
*/

/* Who needs templates anyway? / Hubbe */

#ifndef BAR
#define BAR(NUMTYPE, TYPE, NAME) FOO(NUMTYPE, TYPE, NAME)
#endif /* !BAR */

/* Program *must* be first! */
BAR(size_t,PIKE_OPCODE_T,program)
FOO(size_t,size_t,relocations)
FOO(size_t,char,linenumbers)
FOO(unsigned INT16,unsigned INT16,identifier_index)
FOO(unsigned INT16,unsigned INT16,variable_index)
FOO(unsigned INT16,struct reference,identifier_references)
FOO(unsigned INT16,struct pike_string *,strings)
FOO(unsigned INT16,struct inherit,inherits)
FOO(unsigned INT16,struct identifier,identifiers)
FOO(unsigned INT16,struct program_constant, constants)
#undef FOO
#undef BAR
