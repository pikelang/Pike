/*
 * $Id: rule.pike,v 1.3 1998/11/12 01:31:26 grubba Exp $
 *
 * A BNF-rule.
 *
 * Henrik Grubbström 1996-11-24
 */

//.
//. File:	rule.pike
//. RCSID:	$Id: rule.pike,v 1.3 1998/11/12 01:31:26 grubba Exp $
//. Author:	Henrik Grubbström (grubba@infovav.se)
//.
//. Synopsis:	Implements a BNF rule.
//.
//. +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//.
//. This object is used to represent a BNF-rule in the LR parser.
//.

/*
 * Object variables
 */

//. + nonterminal
//.   Non-terminal this rule reduces to.
int nonterminal;

//. + symbols
//.   The actual rule
array(string|int) symbols;

//. + action
//.   Action to do when reducing this rule.
//.   function - call this function.
//.   string - call this function by name in the object given to the parser.
//.   The function is called with arguments corresponding to the values of
//.   the elements of the rule. The return value of the function will be
//.   the value of this non-terminal. The default rule is to return the first
//.   argument.
function|string action;

/* Variables used when compiling */

//. + has_tokens
//.   This rule contains tokens
int has_tokens = 0;

//. + num_nonnullables
//.   This rule has this many non-nullable symbols at the moment.
int num_nonnullables = 0;

/*
multiset(int) prefix_nonterminals = (<>);
multiset(string) prefix_tokens = (<>);
*/

//. + number
//.   Sequence number of this rule (used for conflict resolving)
int number = 0;

//. + rule_hash
//.   Hash value used to compare rules.
string rule_hash;

//. - make_rule_hash
//.   Compute the rule_hash.
void make_rule_hash()
{
  if (!rule_hash) {
    rule_hash = sprintf("%O:%O:%O", nonterminal, symbols, action);
  }
}

/*
 * Functions
 */

//. - create
//.   Create a BNF rule.
//. EXAMPLE:
//.   The rule
//. 
//.	   rule : nonterminal ":" symbols ";" { add_rule };
//. 
//.   might be created as
//. 
//.	   rule(4, ({ 9, ";", 5, ";" }), "add_rule");
//. 
//.   where 4 corresponds to the nonterminal "rule", 9 to "nonterminal"
//.   and 5 to "symbols", and the function "add_rule" is too be called
//.   when this rule is reduced.
//. > nt
//.   Non-terminal to reduce to.
//. > r
//.   Symbol sequence that reduces to nt.
//. > a
//.   Action to do when reducing according to this rule.
//.   function - Call this function.
//.   string - Call this function by name in the object given to the parser.
//.   The function is called with arguments corresponding to the values of
//.   the elements of the rule. The return value of the function will become
//.   the value of this non-terminal. The default rule is to return the first
//.   argument.
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

