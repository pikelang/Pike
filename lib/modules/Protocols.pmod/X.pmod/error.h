/* error.h
 *
 */

#define error(x) throw( ({ (x), backtrace() }) )
