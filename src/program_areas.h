/* $Id: program_areas.h,v 1.8 2001/07/08 20:47:07 grubba Exp $ */
/* Who needs templates anyway? / Hubbe */

/* Program *must* be first! */
FOO(size_t,PIKE_OPCODE_T,program)
FOO(size_t,char,linenumbers)
FOO(unsigned INT16,unsigned INT16,identifier_index)
FOO(unsigned INT16,unsigned INT16,variable_index)
FOO(unsigned INT16,struct reference,identifier_references)
FOO(unsigned INT16,struct pike_string *,strings)
FOO(unsigned INT16,struct inherit,inherits)
FOO(unsigned INT16,struct identifier,identifiers)
FOO(unsigned INT16,struct program_constant, constants)
#undef FOO

