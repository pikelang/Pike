/*
 * $Id: computedgoto.h,v 1.4 2002/04/07 19:30:11 mast Exp $
 */

#define UPDATE_PC()

#define READ_INCR_BYTE(PC)	((INT32)(ptrdiff_t)((PC)++)[0])
