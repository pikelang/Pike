/*
 * $Id: computedgoto.h,v 1.3 2001/07/20 22:45:16 grubba Exp $
 */

#define PIKE_OPCODE_T	void *

#define UPDATE_PC()

#define READ_INCR_BYTE(PC)	((INT32)(ptrdiff_t)((PC)++)[0])
