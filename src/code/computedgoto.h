/*
 * $Id: computedgoto.h,v 1.2 2001/07/20 15:49:00 grubba Exp $
 */

#define UPDATE_PC()

#define READ_INCR_BYTE(PC)	((INT32)(ptrdiff_t)((PC)++)[0])
