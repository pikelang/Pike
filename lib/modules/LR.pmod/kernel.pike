/*
 * $Id: kernel.pike,v 1.3 1998/11/12 01:31:24 grubba Exp $
 *
 * Implements a LR(1) state;
 *
 * Henrik Grubbström 1996-11-25
 */

//.
//. File:	kernel.pike
//. RCSID:	$Id: kernel.pike,v 1.3 1998/11/12 01:31:24 grubba Exp $
//. Author:	Henrik Grubbström
//.
//. Synopsis:	Implements an LR(1) state.
//.
//. +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//.
//. State in the LR(1) state-machine.
//.

import LR;

//. + rules
//.   Used to check if a rule already has been added when doing closures.
multiset(object(rule)) rules = (<>);

//. + items
//.   Contains the items in this state.
array(object(item)) items = ({});

//. + symbol_items
//.   Contains the items whose next symbol is this non-terminal.
mapping(int : multiset(object(item))) symbol_items = ([]);

//. + action
//.   The action table for this state
//.
//.   object(kernel)	SHIFT to this state on this symbol.
//.   object(rule)	REDUCE according to this rule on this symbol.
mapping(int|string : object /* (kernel) */|object(rule)) action = ([]);

//. + kernel_hash
//.   Hash value used by equalp
string kernel_hash;

/*
 * Functions
 */

//. - add_item
//.   Add an item to the state.
//. > i
//.   Item to add.
void add_item(object(item) i)
{
  int|string symbol;
  
  items += ({ i });
  if (i->offset < sizeof(i->r->symbols)) {
    symbol = i->r->symbols[i->offset];

    if (symbol_items[symbol]) {
      symbol_items[symbol][i] = 1;
    } else {
      symbol_items[symbol] = (< i >);
    }
  }
  kernel_hash = 0;
}

//. - make_kernel_hash
//.   Computes the kernel hash.
void make_kernel_hash()
{
  if (!kernel_hash) {
    items->make_item_hash();
    kernel_hash = sort(items->item_hash) * ":";
  }
}

//. - equalp
//.   Compare with another state.
//. > state
//.   State to compare with.
int equalp(object /* (kernel) */ state)
{
  /* Two states are the same if they contain the same items */
  if (sizeof(state->items) != sizeof(items)) {
    return(0);
  }

  if (!kernel_hash) {
    make_kernel_hash();
  }
  if (!state->kernel_hash) {
    state->make_kernel_hash();
  }
  return(kernel_hash == state->kernel_hash);


  /* Could probably make it test only kernel items */

  foreach (state->items, object(item) i) {
    if (search(items, i) == -1) {
      int found = 0;

      foreach (items, object(item) i2) {
	/* Two items are the same if they have the same rule
	 * and the same offset;
	 */
	if ((i->offset == i2->offset) &&
	    (i->r == i2->r)) {
	  found = 1;
	  break;		/* BUG in Pike 0.3 beta */
	}
      }
      if (!found) {
	return(0);
      }
    }
  }
  return(1);
}

