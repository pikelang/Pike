/*
 * $Id: Item.pike,v 1.2 2002/05/09 20:46:49 nilsson Exp $
 *
 * An LR(0) item
 *
 * Henrik Grubbström 1996-11-27
 */

#pike __REAL_VERSION__

//!
//! An LR(0) item, a partially parsed rule.
//!

import LR;

/* constant kernel = (program)"kernel"; */
/* constant item = (program)"item"; */

//! The rule
Rule r;

//! How long into the rule the parsing has come.
int offset;

//! The state we will get if we shift according to this rule
object /* Kernel */ next_state;

//! Item representing this one (used for shifts).
object /* Item */ master_item;

//! Look-ahead set for this item.
multiset(string) direct_lookahead = (<>);

//! Look-ahead set used for detecting conflicts
multiset(string) error_lookahead = (<>);

//! Relation to other items (used when compiling).
multiset(object /* Item */ ) relation = (<>);

//! Depth counter (used when compiling).
int counter;

//! Item identification number (used when compiling).
int number;

//! Used to identify the item.
//! Equal to r->number + offset.
int item_id;
