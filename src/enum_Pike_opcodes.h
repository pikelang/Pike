/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef ENUM_PIKE_OPCODES_H
#define ENUM_PIKE_OPCODES_H

/*
 * enum Pike_opcodes
 */

#define OPCODE0(X,Y,F) X,
#define OPCODE1(X,Y,F) X,
#define OPCODE2(X,Y,F) X,
#define OPCODE0_TAIL(X,Y,F) X,
#define OPCODE1_TAIL(X,Y,F) X,
#define OPCODE2_TAIL(X,Y,F) X,
#define OPCODE0_JUMP(X,Y,F) X,
#define OPCODE1_JUMP(X,Y,F) X,
#define OPCODE2_JUMP(X,Y,F) X,
#define OPCODE0_TAILJUMP(X,Y,F) X,
#define OPCODE1_TAILJUMP(X,Y,F) X,
#define OPCODE2_TAILJUMP(X,Y,F) X,
#define OPCODE0_PTRJUMP(X,Y,F) X,
#define OPCODE1_PTRJUMP(X,Y,F) X,
#define OPCODE2_PTRJUMP(X,Y,F) X,
#define OPCODE0_TAILPTRJUMP(X,Y,F) X,
#define OPCODE1_TAILPTRJUMP(X,Y,F) X,
#define OPCODE2_TAILPTRJUMP(X,Y,F) X,
#define OPCODE0_RETURN(X,Y,F) X,
#define OPCODE1_RETURN(X,Y,F) X,
#define OPCODE2_RETURN(X,Y,F) X,
#define OPCODE0_TAILRETURN(X,Y,F) X,
#define OPCODE1_TAILRETURN(X,Y,F) X,
#define OPCODE2_TAILRETURN(X,Y,F) X,
#define OPCODE0_BRANCH(X,Y,F) X,
#define OPCODE1_BRANCH(X,Y,F) X,
#define OPCODE2_BRANCH(X,Y,F) X,
#define OPCODE0_TAILBRANCH(X,Y,F) X,
#define OPCODE1_TAILBRANCH(X,Y,F) X,
#define OPCODE2_TAILBRANCH(X,Y,F) X,
#define OPCODE0_ALIAS(X,Y,F,A) X,
#define OPCODE1_ALIAS(X,Y,F,A) X,
#define OPCODE2_ALIAS(X,Y,F,A) X,

#define OPCODE_NOCODE(DESC, OP, FLAGS)   OP,

enum Pike_opcodes
{
  F_OFFSET = 257,
#include "opcode_list.h"

  /* These are only used for the parse tree. */
  F_LOCAL_INDIRECT,	/* F_LOCAL that has not yet been allocated
                         * a slot in the Pike_fp->locals array.
                         */

  F_SPACE = ' ',
  F_COLON = ':',
  F_COND = '?',

#ifdef PIKE_DEBUG
  OPCODES_END = USHRT_MAX
#endif
};

#undef OPCODE0
#undef OPCODE1
#undef OPCODE2
#undef OPCODE0_TAIL
#undef OPCODE1_TAIL
#undef OPCODE2_TAIL
#undef OPCODE0_PTRJUMP
#undef OPCODE1_PTRJUMP
#undef OPCODE2_PTRJUMP
#undef OPCODE0_TAILPTRJUMP
#undef OPCODE1_TAILPTRJUMP
#undef OPCODE2_TAILPTRJUMP
#undef OPCODE0_RETURN
#undef OPCODE1_RETURN
#undef OPCODE2_RETURN
#undef OPCODE0_TAILRETURN
#undef OPCODE1_TAILRETURN
#undef OPCODE2_TAILRETURN
#undef OPCODE0_BRANCH
#undef OPCODE1_BRANCH
#undef OPCODE2_BRANCH
#undef OPCODE0_TAILBRANCH
#undef OPCODE1_TAILBRANCH
#undef OPCODE2_TAILBRANCH
#undef OPCODE0_JUMP
#undef OPCODE1_JUMP
#undef OPCODE2_JUMP
#undef OPCODE0_TAILJUMP
#undef OPCODE1_TAILJUMP
#undef OPCODE2_TAILJUMP
#undef OPCODE0_ALIAS
#undef OPCODE1_ALIAS
#undef OPCODE2_ALIAS
#undef OPCODE_NOCODE

#endif /* ENUM_PIKE_OPCODES_H */
