/* $Id: program_areas.h,v 1.3 1998/03/28 13:38:43 grubba Exp $ */
/* Who needs templates anyway? / Hubbe */

/* Program *must* be first! */
FOO(SIZE_T,unsigned char,program)
FOO(SIZE_T,char,linenumbers)
FOO(unsigned INT16,struct inherit,inherits)
FOO(unsigned INT16,struct pike_string *,strings)
FOO(unsigned INT16,struct reference,identifier_references)
FOO(unsigned INT16,struct identifier,identifiers)
FOO(unsigned INT16,unsigned INT16,identifier_index)
FOO(unsigned INT16,struct svalue, constants)

#undef FOO

