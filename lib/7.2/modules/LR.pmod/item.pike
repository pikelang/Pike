/*
 * $Id$
 *
 * An LR(0) item
 *
 * Henrik Grubbström 1996-11-27
 */

#pike 7.2

//!
//! An LR(0) item, a partially parsed rule.
//!

import LR;

/* constant kernel = (program)"kernel"; */
/* constant item = (program)"item"; */

//! The rule
object(rule) r;

//! How long into the rule the parsing has come.
int offset;

//! The state we will get if we shift according to this rule
object /* (kernel) */ next_state;

//! Item representing this one (used for shifts).
object /* (item) */ master_item;

//! Look-ahead set for this item.
multiset(string) direct_lookahead = (<>);

//! Look-ahead set used for detecting conflicts
multiset(string) error_lookahead = (<>);

//! Relation to other items (used when compiling).
multiset(object /* (item) */ ) relation = (<>);

//! Depth counter (used when compiling).
int counter;

//! Item identification number (used when compiling).
int number;

//! Used to identify the item.
//! Equal to r->number + offset.
int item_id;
