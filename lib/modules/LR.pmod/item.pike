/*
 * $Id: item.pike,v 1.4 1998/11/12 19:45:41 grubba Exp $
 *
 * An LR(0) item
 *
 * Henrik Grubbström 1996-11-27
 */

//.
//. File:	item.pike
//. RCSID:	$Id: item.pike,v 1.4 1998/11/12 19:45:41 grubba Exp $
//. Author:	Henrik Grubbström (grubba@infovav.se)
//.
//. Synopsis:	An LR(0) item
//.
//. +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//.
//. A partially parsed rule.
//.

import LR;

/* constant kernel = (program)"kernel"; */
/* constant item = (program)"item"; */

//. + r
//.   The rule
object(rule) r;

//. + offset
//.   How long into the rule the parsing has come.
int offset;

//. + next_state
//.   The state we will get if we shift according to this rule
object /* (kernel) */ next_state;

//. + master_item
//.   Item representing this one (used for shifts).
object /* (item) */ master_item;

//. + direct_lookahead
//.   Look-ahead set for this item.
multiset(string) direct_lookahead = (<>);
//. + error_lookahead
//.   Look-ahead set used for detecting conflicts
multiset(string) error_lookahead = (<>);
//. + relation
//.   Relation to other items (used when compiling).
multiset(object /* (item) */ ) relation = (<>);
//. + counter
//.   Depth counter (used when compiling).
int counter;

//. + number
//.   Item identification number (used when compiling).
int number;

//. + item_id
//.   Used to identify the item.
//.   Equal to r->number + offset.
int item_id;
