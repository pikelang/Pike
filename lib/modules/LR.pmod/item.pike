/*
 * $Id: item.pike,v 1.3 1998/11/12 01:31:21 grubba Exp $
 *
 * An LR(0) item
 *
 * Henrik Grubbström 1996-11-27
 */

//.
//. File:	item.pike
//. RCSID:	$Id: item.pike,v 1.3 1998/11/12 01:31:21 grubba Exp $
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
object /* (item) */ master_item = 0;

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
int counter = 0;

//. + item_hash
//.   Hash used to compare items.
string item_hash;

//. - make_item_hash
//.   Calculate the item hash.
void make_item_hash()
{
  r->make_rule_hash();
  if (!item_hash) {
    item_hash = r->rule_hash + "·" + offset;
  }
}
