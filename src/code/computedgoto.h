/*
 * $Id: computedgoto.h,v 1.5 2002/05/10 23:01:12 mast Exp $
 */

#define UPDATE_PC()

#define PROG_COUNTER pc

#define READ_INCR_BYTE(PC)	((INT32)(ptrdiff_t)((PC)++)[0])
