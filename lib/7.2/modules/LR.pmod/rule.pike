/*
 * $Id$
 *
 * A BNF-rule.
 *
 * Henrik Grubbström 1996-11-24
 */

#pike 7.2

//!
//! This object is used to represent a BNF-rule in the LR parser.
//!

/*
 * Object variables
 */

//! Non-terminal this rule reduces to.
int nonterminal;

//! The actual rule
array(string|int) symbols;

//! Action to do when reducing this rule.
//! function - call this function.
//! string - call this function by name in the object given to the parser.
//! The function is called with arguments corresponding to the values of
//! the elements of the rule. The return value of the function will be
//! the value of this non-terminal. The default rule is to return the first
//! argument.
function|string action;

/* Variables used when compiling */

//! This rule contains tokens
int has_tokens = 0;

//! This rule has this many non-nullable symbols at the moment.
int num_nonnullables = 0;

/*
multiset(int) prefix_nonterminals = (<>);
multiset(string) prefix_tokens = (<>);
*/

//! Sequence number of this rule (used for conflict resolving)
//! Also used to identify the rule.
int number = 0;

//! Priority and associativity of this rule.
object /* (priority) */ pri;

/*
 * Functions
 */

//! Create a BNF rule.
//!
//! @example
//!   The rule
//! 
//!	   rule : nonterminal ":" symbols ";" { add_rule };
//! 
//!   might be created as
//! 
//!	   rule(4, ({ 9, ";", 5, ";" }), "add_rule");
//! 
//!   where 4 corresponds to the nonterminal "rule", 9 to "nonterminal"
//!   and 5 to "symbols", and the function "add_rule" is too be called
//!   when this rule is reduced.
//!
//! @param nt
//!   Non-terminal to reduce to.
//! @param r
//!   Symbol sequence that reduces to nt.
//! @param a
//!   Action to do when reducing according to this rule.
//!   function - Call this function.
//!   string - Call this function by name in the object given to the parser.
//!   The function is called with arguments corresponding to the values of
//!   the elements of the rule. The return value of the function will become
//!   the value of this non-terminal. The default rule is to return the first
//!   argument.
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

