/*
 * $Id: item.pike,v 1.1 1997/03/03 23:50:15 grubba Exp $
 *
 * An LR(0) item
 *
 * Henrik Grubbström 1996-11-27
 */

import LR;

/* constant kernel = (program)"kernel"; */
/* constant item = (program)"item"; */

/* The rule */
object(rule) r;
/* How long into the rule the parsing has come */
int offset;
/* The state we will get if we shift */
object /* (kernel) */ next_state;
/* Item representing this one (used for shifts) */
object /* (item) */ master_item = 0;
/* Look-ahead set for this item */
multiset(string) direct_lookahead = (<>);
multiset(string) error_lookahead = (<>);
/* Relation to other items (used when compiling) */
multiset(object /* (item) */ ) relation = (<>);
/* Depth counter (used when compiling) */
int counter = 0;
