/*
 * $Id: kernel.pike,v 1.1 1997/03/03 23:50:16 grubba Exp $
 *
 * Implements a LR(1) state;
 *
 * Henrik Grubbström 1996-11-25
 */

import LR;

/* Used to check if a rule already has been added when doing closures */
multiset(object(rule)) rules = (<>);

/* Contains the items in this state */
array(object(item)) items = ({});

/* Contains the items whose next symbol is this non-terminal */
mapping(int : multiset(object(item))) symbol_items = ([]);

/* The action table for this state
 *
 * object(kernel)	SHIFT to this state on this symbol.
 * object(rule)		REDUCE according to this rule on this symbol.
 */
mapping(int|string : object /* (kernel) */|object(rule)) action = ([]);

/*
 * Functions
 */

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

int equalp(object /* (kernel) */ state)
{
  /* Two states are the same if they contain the same items */
  if (sizeof(state->items) != sizeof(items)) {
    return(0);
  }

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

