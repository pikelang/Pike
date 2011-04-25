/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/* Who needs templates anyway? / Hubbe */

#ifndef BAR
#define BAR(NUMTYPE, TYPE, ARGTYPE, NAME) FOO(NUMTYPE, TYPE, ARGTYPE, NAME)
#endif /* !BAR */
#ifndef PASS1ONLY
#define PASS1ONLY(NUMTYPE, TYPE, ARGTYPE, NAME) FOO(NUMTYPE, TYPE, ARGTYPE, NAME)
#endif /* !PASS1ONLY */

/* Program *must* be first! */
BAR(size_t,PIKE_OPCODE_T, PIKE_OPCODE_T, program)
FOO(size_t,size_t, size_t, relocations)
FOO(size_t,char, int, linenumbers)
FOO(unsigned INT16,unsigned INT16, unsigned, identifier_index)
FOO(unsigned INT16,unsigned INT16, unsigned, variable_index)
FOO(unsigned INT16,struct pike_string *, struct pike_string *, strings)
FOO(unsigned INT16,struct program_constant, struct program_constant, constants)
PASS1ONLY(unsigned INT16,struct reference, struct reference, identifier_references)
PASS1ONLY(unsigned INT16,struct inherit, struct inherit, inherits)
PASS1ONLY(unsigned INT16,struct identifier, struct identifier, identifiers)
#undef PASS1ONLY
#undef FOO
#undef BAR
