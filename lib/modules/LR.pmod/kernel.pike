/*
 * $Id: kernel.pike,v 1.7 1998/11/13 21:05:35 grubba Exp $
 *
 * Implements a LR(1) state;
 *
 * Henrik Grubbström 1996-11-25
 */

//.
//. File:	kernel.pike
//. RCSID:	$Id: kernel.pike,v 1.7 1998/11/13 21:05:35 grubba Exp $
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
}

