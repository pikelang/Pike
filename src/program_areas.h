/* $Id: program_areas.h,v 1.6 1999/09/18 09:21:29 hubbe Exp $ */
/* Who needs templates anyway? / Hubbe */

/* Program *must* be first! */
FOO(SIZE_T,unsigned char,program)
FOO(SIZE_T,char,linenumbers)
FOO(unsigned INT16,unsigned INT16,identifier_index)
FOO(unsigned INT16,unsigned INT16,variable_index)
FOO(unsigned INT16,struct reference,identifier_references)
FOO(unsigned INT16,struct pike_string *,strings)
FOO(unsigned INT16,struct inherit,inherits)
FOO(unsigned INT16,struct identifier,identifiers)
FOO(unsigned INT16,struct program_constant, constants)
#undef FOO

