/*
 * $Id: rule.pike,v 1.1 1997/03/03 23:50:21 grubba Exp $
 *
 * A BNF-rule.
 *
 * Henrik Grubbström 1996-11-24
 */

/*
 * Object variables
 */

/* Nonterminal this rule reduces to */
int nonterminal;
/* The actual rule */
array(string|int) symbols;
/* Action to do when reducing this rule
 *
 * function - call this function
 * string - call this function by name in the object given to the parser
 */
function|string action;

/* Variables used when compiling */

/* This rule contains tokens */
int has_tokens = 0;
/* This rule has this many nonnullable symbols at the moment */
int num_nonnullables = 0;

/*
multiset(int) prefix_nonterminals = (<>);
multiset(string) prefix_tokens = (<>);
*/

/* Number of this rule (used for conflict resolving) */
int number = 0;

/*
 * Functions
 */

void create(int nt, array(string|int) r, function|string|void a)
{
  mixed symbol;

  nonterminal = nt;
  symbols = r;
  action = a;

  foreach (r, symbol) {
    if (stringp(symbol)) {
      has_tokens = 1;
      break;
    }
  }

  num_nonnullables = sizeof(r);
}

