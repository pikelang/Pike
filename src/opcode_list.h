/*
 * These values are used by the stack machine, and can not be directly
 * called from Pike.
 */
OPCODE_NOCODE("arg+=256", F_PREFIX_256,0)
OPCODE_NOCODE("arg+=512", F_PREFIX_512,0)
OPCODE_NOCODE("arg+=768", F_PREFIX_768,0)
OPCODE_NOCODE("arg+=1024", F_PREFIX_1024,0)
OPCODE_NOCODE("arg+=256*X", F_PREFIX_CHARX256,0)
OPCODE_NOCODE("arg+=256*XX", F_PREFIX_WORDX256,0)
OPCODE_NOCODE("arg+=256*XXX", F_PREFIX_24BITX256,0)

OPCODE_NOCODE("arg+=256", F_PREFIX2_256,0)
OPCODE_NOCODE("arg+=512", F_PREFIX2_512,0)
OPCODE_NOCODE("arg+=768", F_PREFIX2_768,0)
OPCODE_NOCODE("arg+=1024", F_PREFIX2_1024,0)
OPCODE_NOCODE("arg+=256*X", F_PREFIX2_CHARX256,0)
OPCODE_NOCODE("arg+=256*XX", F_PREFIX2_WORDX256,0)
OPCODE_NOCODE("arg+=256*XXX", F_PREFIX2_24BITX256,0)

#include "interpret_protos.h"

/* Used to mark an entry point from eval_instruction(). */
OPCODE_NOCODE("entry", F_ENTRY,0)

/* These two are only used for dumping. */
OPCODE_NOCODE("filename", F_FILENAME, 0)
OPCODE_NOCODE("line", F_LINE, 0)

/* Alias for F_EXTERNAL when the identifier is a getter/setter. */
OPCODE_NOCODE("get/set", F_GET_SET, 0)

/*
 * These are token values that needn't have an associated code for the
 * compiled file
 */
OPCODE_NOCODE("opcode_max", F_MAX_OPCODE, 0)

OPCODE_NOCODE("+=", F_ADD_EQ,0)
OPCODE_NOCODE("&=", F_AND_EQ,0)
OPCODE_NOCODE("arg_list", F_ARG_LIST, 0)
OPCODE_NOCODE("comma_expr", F_COMMA_EXPR, 0)
OPCODE_NOCODE("break", F_BREAK,0)
OPCODE_NOCODE("case", F_CASE,0)
OPCODE_NOCODE("case_range", F_CASE_RANGE,0)
OPCODE_NOCODE("continue", F_CONTINUE,0)
OPCODE_NOCODE("default", F_DEFAULT,0)
OPCODE_NOCODE("/=", F_DIV_EQ,0)
OPCODE_NOCODE("do-while", F_DO,0)
OPCODE_NOCODE("call_efun", F_EFUN_CALL, 0)
OPCODE_NOCODE("for", F_FOR,0)
OPCODE_NOCODE("<<=", F_LSH_EQ,0)
OPCODE_NOCODE("lvalue_list", F_LVALUE_LIST,0)
OPCODE_NOCODE("%=", F_MOD_EQ,0)
OPCODE_NOCODE("|=", F_OR_EQ,0)
OPCODE_NOCODE("*=", F_MULT_EQ,0)
OPCODE_NOCODE("**=", F_POW_EQ,0)
OPCODE_NOCODE(">>=", F_RSH_EQ,0)
OPCODE_NOCODE("-=", F_SUB_EQ,0)
OPCODE_NOCODE("val_lval", F_VAL_LVAL, 0)
OPCODE_NOCODE("foreach_val_lval", F_FOREACH_VAL_LVAL, 0)
OPCODE_NOCODE("^=", F_XOR_EQ,0)
OPCODE_NOCODE("multi_assign", F_MULTI_ASSIGN, 0)
OPCODE_NOCODE("nop", F_NOP,0)

/* a[i.. */
OPCODE_NOCODE("[i..]", F_RANGE_FROM_BEG, 0) 

/* a[<i.. */
OPCODE_NOCODE("[<i..]", F_RANGE_FROM_END, 0)

/* a[.. */
OPCODE_NOCODE("[..]", F_RANGE_OPEN, 0)
OPCODE_NOCODE("version", F_VERSION, 0)
OPCODE_NOCODE("align", F_ALIGN, I_HASARG)
OPCODE_NOCODE("pointer", F_POINTER, I_ISPOINTER)
OPCODE_NOCODE("label", F_LABEL,I_HASARG)
OPCODE_NOCODE("stmt_label", F_NORMAL_STMT_LABEL,I_HASARG)
OPCODE_NOCODE("custom_label", F_CUSTOM_STMT_LABEL,I_HASARG)
OPCODE_NOCODE("data", F_DATA, I_DATA)
OPCODE_NOCODE("function start", F_START_FUNCTION,0)
OPCODE_NOCODE("byte", F_BYTE, I_DATA)
OPCODE_NOCODE("notreached!", F_NOTREACHED, 0)
OPCODE_NOCODE("auto_map_marker", F_AUTO_MAP_MARKER, 0)
OPCODE_NOCODE("auto_map", F_AUTO_MAP, 0)
OPCODE_NOCODE("typeof", F_TYPEOF, 0)

/* Alias for F_RETURN, but cannot be optimized into a tail recursion call */
OPCODE_NOCODE("volatile_return", F_VOLATILE_RETURN, 0)

/* Alias for F_ASSIGN, used when LHS has side-effects that should                                         
 * only be evaluated once. */
OPCODE_NOCODE("assign_self", F_ASSIGN_SELF, 0)

OPCODE_NOCODE("frame_type", F_FRAME_TYPE, I_HASARG2)
OPCODE_NOCODE("frame_name", F_FRAME_NAME, I_HASARG2)
OPCODE_NOCODE("frame_end", F_FRAME_END, I_HASARG)

OPCODE_NOCODE("instr_max", F_MAX_INSTR, 0)
