/*
 * $Id: parser.pike,v 1.2 1997/03/30 17:28:25 grubba Exp $
 *
 * A BNF-grammar in Pike.
 * Compiles to a LALR(1) state-machine.
 *
 * Henrik Grubbström 1996-11-24
 */

//.
//. File:	parser.pike
//. RCSID:	$Id: parser.pike,v 1.2 1997/03/30 17:28:25 grubba Exp $
//. Author:	Henrik Grubbström (grubba@infovav.se)
//.
//. Synopsis:	LALR(1) parser and compiler.
//.
//. +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//.
//. This object implements an LALR(1) parser and compiler.
//.
//. Normal use of this object would be:
//.
//. {add_rule, set_priority, set_associativity}*
//. set_scanner
//. set_symbol_to_string
//. compile
//. {parse}*
//.

/*
 * Includes
 */

#include <array.h>

/*
 * Defines
 */

/* Errors during parsing */
/* Unexpected EOF */
#define ERROR_EOF			1
/* Syntax error in input */
#define ERROR_SYNTAX			2
/* Shift-Reduce or Reduce-Reduce */
#define ERROR_CONFLICTS			4
/* Action is missing from action object */
#define ERROR_MISSING_ACTION		8
/* Action is not a function */
#define ERROR_BAD_ACTION_TYPE		16
/* Action invoked by name, but no object given */
#define ERROR_NO_OBJECT			32
/* Scanner not set */
#define ERROR_NO_SCANNER		64
/* Missing definition of nonterminal */
#define ERROR_MISSING_DEFINITION	128

/*
 * Classes
 */

//. o state_queue
//.
//. This is a combined set and queue.
class state_queue {
  //. + head
  //. Index of the head of the queue.
  int head;
  //. + tail
  //. Index of the tail of the queue.
  int tail;
  //. + arr
  //. The queue/set itself.
  array(object(LR.kernel)) arr=allocate(64);

  //. - memberp
  //.   Returns the index of the state in arr if present.
  //.   Returns -1 on failure.
  //. > state
  //.   State to search for.
  int|object(LR.kernel) memberp(object(LR.kernel) state)
  {
    int j;

    for (j = 0; j<tail; j++) {
      if (state->equalp(arr[j])) {
	return(j);
      }
    }
    return(-1);
  }

  //. - push_if_new
  //.   Pushes the state on the queue if it isn't there already.
  //. > state
  //.   State to push.
  object(LR.kernel) push_if_new(object(LR.kernel) state)
  {
    int index;

    if ((index = memberp(state)) >= 0) {
      return (arr[index]);
    } else {
      if (tail == sizeof(arr)) {
	arr += allocate(tail);
      }
      arr[tail++] = state;

      return(state);
    }
  }

  //. - next
  //.   Return the next state from the queue.
  int|object(LR.kernel) next()
  {
    if (head == tail) {
      return(0);
    } else {
      return(arr[head++]);
    }
  }
}

import LR;

/* The grammar itself */
static private mapping(int|string : array(object(rule))) grammar = ([]);

/* Priority table for terminal symbols */
static private mapping(string : object(priority)) operator_priority = ([]);

static private multiset(mixed) nullable = (< >);

#if 0
static private mapping(mixed : multiset(object(rule))) derives = ([]);

/* Maps from symbol to which rules may start with that symbol */
static private mapping(mixed : multiset(object(rule))) begins = ([]);
#endif /* 0 */


/* Maps from symbol to which rules use that symbol
 * (used for findling nullable symbols)
 */
static private mapping(int : multiset(object(rule))) used_by = ([]);

static private object(kernel) start_state;

//. + verbose
//.   Verbosity level
//.   0 - none
//.   1 - some
int verbose=1;

//. + error
//.   Error code
int error=0;

/* Number of next rule (used only for conflict resolving) */
static private int next_rule_number = 0;

/*
 * Functions
 */

/* Here are some help functions */

/* Several cast to string functions */

static private string builtin_symbol_to_string(int|string symbol)
{
  if (intp(symbol)) {
    return("nonterminal"+symbol);
  } else {
    return("\"" + symbol + "\"");
  }
}

static private function(int|string : string) symbol_to_string = builtin_symbol_to_string;

//. - rule_to_string
//.   Pretty-prints a rule to a string.
//. > r
//.   Rule to print.
string rule_to_string(object(rule) r)
{
  string res = symbol_to_string(r->nonterminal) + ":\t";

  if (sizeof(r->symbols)) {
    foreach (r->symbols, int|string symbol) {
      res += symbol_to_string(symbol) + " ";
    }
  } else {
    res += "/* empty */";
  }
  return(res);
}

//. - item_to_string
//.   Pretty-prints an item to a string.
//. > i
//.   Item to pretty-print.
string item_to_string(object(item) i)
{
  array(string) res = ({ symbol_to_string(i->r->nonterminal), ":\t" });

  if (i->offset) {
    foreach(i->r->symbols[0..i->offset-1], int|string symbol) {
      res += ({ symbol_to_string(symbol), " " });
    }
  }
  res += ({ "· " });
  if (i->offset != sizeof(i->r->symbols)) {
    foreach(i->r->symbols[i->offset..], int|string symbol) {
      res += ({ symbol_to_string(symbol), " " });
    }
  }
  if (sizeof(indices(i->direct_lookahead))) {
    res += ({ "\t{ ",
		map(indices(i->direct_lookahead), symbol_to_string) * ", ",
		" }" });
  }
  return(res * "");
}

//. - state_to_string
//.   Pretty-prints a state to a string.
//. > state
//.   State to pretty-print.
string state_to_string(object(kernel) state)
{
  return (map(state->items, item_to_string) * "\n");
}

//. - cast_to_string
//.   Pretty-prints the current grammar to a string.
string cast_to_string()
{
  array(string) res = ({});

  foreach (indices(grammar), int nonterminal) {
    res += ({ symbol_to_string(nonterminal) });
    foreach (grammar[nonterminal], object(rule) r) {
      res += ({ "\t: " });
      if (sizeof(r->symbols)) {
	foreach (r->symbols, int|string symbol) {
	  res += ({ symbol_to_string(symbol), " " });
	}
      } else {
	res += ({ "/* empty */" });
      }
      res += ({ "\n" });
    }
    res += ({ "\n" });
  }
  return (res * "");
}

//. - cast
//.   Implements casting.
//. > type
//.   Type to cast to.
mixed cast(string type)
{
  if (type == "string") {
    return(cast_to_string());
  }
  throw ( ({ "Cast to "+type+" not supported\n", backtrace()[0..-2] }) );
}

/* Here come the functions that actually do some work */

//. - set_priority
//.   Sets the priority of a terminal.
//. > terminal
//.   Terminal to set the priority for.
//. > pri_val
//.   Priority; higher = prefer this terminal.
void set_priority(string terminal, int pri_val)
{
  object(priority) pri;

  if (pri = operator_priority[terminal]) {
    pri->value = pri_val;
  } else {
    operator_priority[terminal] = priority(pri_val, 0);
  }
}

//. - set_associativity
//.   Sets the associativity of a terminal.
//. > terminal
//.   Terminal to set the associativity for.
//. > assoc
//.   Associativity; negative - left, positive - right, zero - no associativity.
void set_associativity(string terminal, int assoc)
{
  object(priority) pri;

  if (pri = operator_priority[terminal]) {
    pri->assoc = assoc;
  } else {
    operator_priority[terminal] = priority(0, assoc);
  }
}

//. - set_symbol_to_string
//.   Sets the symbol to string conversion function.
//.   The conversion function is used by the various *_to_string functions
//.   to make comprehensible output.
//. > s_to_s
//.   Symbol to string conversion function.
//.   If zero or not specified, use the built-in function.
void set_symbol_to_string(void|function(int|string:string) s_to_s)
{
  if (s_to_s) {
    symbol_to_string = s_to_s;
  } else {
    symbol_to_string = builtin_symbol_to_string;
  }
}

//. - add_rule
//.   Add a rule to the grammar.
//. > r
//.   Rule to add.
void add_rule(object(rule) r)
{
  array(object(rule)) rules;
  int|string symbol;

  /* DEBUG */
  if (verbose) {
    werror("Adding rule: " + rule_to_string(r) + "\n");
  }

  /* !DEBUG */

  r->number = next_rule_number++;

  /* First add the rule to the grammar */
  if (grammar[r->nonterminal]) {
    grammar[r->nonterminal] += ({ r });
  } else {
    grammar[r->nonterminal] = ({ r });
  }

  /* Then see if it is nullable */
  if (!r->has_tokens) {
    object(Stack.stack) new_nullables = Stack.stack();

    foreach (r->symbols, symbol) {
      if (nullable[symbol]) {
	r->num_nonnullables--;
      } else {
	if (used_by[symbol]) {
	  if (used_by[symbol][r]) {
	    /* Only count a symbol once */
	    r->num_nonnullables--;
	  } else {
	    used_by[symbol][r] = 1;
	  }
	} else {
	  used_by[symbol] = (< r >);
	}
      }
    }

    if (!(r->num_nonnullables)) {
      /* This rule was nullable */
      new_nullables->push(r->nonterminal);

      while (new_nullables->ptr) {
	symbol = new_nullables->pop();
	if (verbose) {
	  werror(sprintf("Nulling symbol %s\n",
			 symbol_to_string(symbol)));
	}
	nullable[symbol] = 1;
	if (used_by[symbol]) {
	  foreach (indices(used_by[symbol]), object(rule) r2) {
	    if (!(--r2->num_nonnullables)) {
	      new_nullables->push(r2->nonterminal);
	    }
	  }
	  used_by[symbol] = 0;	/* No more need for this info */
	}
      }
    }
  }

  /* The info calculated from this point is not at the moment used
   * by the compiler
   */
#if 0
  /* Now check for symbols that may begin this rule */
  foreach (r->symbols, symbol) {
    if (!stringp(symbol)) {
      multiset set = begins[symbol];

      r->prefix_nonterminals |= (< symbol >);

      if (set) {
	set[r] = 1;
      } else {
	begins[symbol] = (< r >);
      }

      if (grammar[symbol]) {
	foreach (grammar[symbol], object(rule) r2) {
	  r->prefix_nonterminals |= r2->prefix_nonterminals;
	  r->prefix_tokens |= r2->prefix_tokens;
	
	  foreach (indices(r2->prefix_nonterminals), mixed s2) {
	    set = begins[s2];

	    if (set) {
	      set[r] = 1;
	    } else {
	      begins[s2] = (< r >);
	    }
	  }
	}
      }
      if (!nullable[symbol]) {
	break;
      }
    } else {
      r->prefix_tokens[symbol] = 1;
      break;
    }
  }

  /* Scan through the rules beginning with this rule's non-terminal */
  if (begins[r->nonterminal]) {
    foreach (indices(begins[r->nonterminal]), object(rule) r2) {
      r2->prefix_nonterminals |= r->prefix_nonterminals;
      r2->prefix_tokens |= r->prefix_tokens;

      /* NOTE: Might want to move values(r->prefixes) out of the loop */
      foreach (values(r->prefix_nonterminals), symbol) {
	multiset set = begins[symbol];

	if (set) {
	  set[r2] = 1;
	} else {
	  begins[symbol] = (< r2 >);
	}
      }
    }
  }
#endif /* 0 */
}

/* Here come the functions used by the compiler */

static private multiset(int|string) make_goto_set(object(kernel) state)
{
  multiset(int|string) set = (<>);

  foreach (state->items, object(item) i) {
    if (i->offset != sizeof(i->r->symbols)) {
      set[i->r->symbols[i->offset]] = 1;
    }
  }

  if (verbose) {
    werror(sprintf("make_goto_set()=> (< %s >)\n",
		   map(indices(set), symbol_to_string) * ", "));
  }

  return (set);
}

static private void make_closure(object(kernel) state, int nonterminal)
{
  if (grammar[nonterminal]) {
    foreach (grammar[nonterminal], object(rule) r) {
      if (!(state->rules[r])) {
	object(item) new_item = item();
	
	new_item->r = r;
	new_item->offset = 0;

	state->rules[r] = 1;

	state->add_item(new_item);

	if (sizeof(r->symbols) && intp(r->symbols[0])) {
	  make_closure(state, r->symbols[0]);
	}
      }
    }
  } else {
    werror(sprintf("Error: Definition missing for non-terminal %s\n",
		   symbol_to_string(nonterminal)));
    error |= ERROR_MISSING_DEFINITION;
  }
}

static private object(kernel) first_state()
{
  object(kernel) state = kernel();

  foreach (grammar[0], object(rule) r) {
    if (!state->rules[r]) {
      object(item) i = item();

      i->r = r;
      i->offset = 0;

      state->add_item(i);
      state->rules[r] = 1;	/* Since this is an item with offset 0 */

      if ((sizeof(r->symbols)) &&
	  (intp(r->symbols[0]))) {
	make_closure(state, r->symbols[0]);
      }
    }
  }
  return(state);
}

/*
 * Contains all states used.
 *
 * In the queue-part are the states that remain to be compiled.
 */
static private object(state_queue) s_q;

static private object(kernel) do_goto(object(kernel) state, int|string symbol)
{
  object(kernel) new_state = kernel();
  multiset(object(item)) items;
  object(rule) r;
  int offset;

  if (verbose) {
    werror(sprintf("Performing GOTO on <%s>\n", symbol_to_string(symbol)));
  }

  items = state->symbol_items[symbol];
  if (items) {
    foreach (indices(items), object(item) i) {
      int|string lookahead;

      object(item) new_item = item();

      offset = i->offset;

      new_item->offset = ++offset;
      new_item->r = r = i->r;

      new_state->add_item(new_item);

      if ((offset != sizeof(r->symbols)) &&
	  intp(lookahead = r->symbols[offset])) {
	make_closure(new_state, lookahead);
      }
    }
  }

  /* DEBUG */

  if (verbose) {
    werror(sprintf("GOTO on %s generated state:\n%s\n",
		   symbol_to_string(symbol),
		   state_to_string(new_state)));
  }

  /* !DEBUG */

  new_state = s_q->push_if_new(new_state);

  if (items) {
    foreach (indices(items), object(item) i) {
      i->next_state = new_state;
    }
  }
}

static private object(Stack.stack) item_stack;

static private void traverse_items(object(item) i,
				   function(int:void) conflict_func)
{
  int depth;

  item_stack->push(i);

  i->counter = depth = item_stack->ptr;

  foreach (indices(i->relation), object(item) i2) {
    if (!i2->counter) {
      traverse_items(i2, conflict_func);
    }
    if (i->counter > i2->counter) {
      i->counter = i2->counter;
    }

    i->direct_lookahead |= i2->direct_lookahead;
  }

  if (i->number == depth) {
    int cyclic = 0;
    int empty_cycle = 1;
    object(item) i2;

    while ((i2 = item_stack->pop()) != i) {

      i2->number = 0x7fffffff;
      
      i2->direct_lookahead = i->direct_lookahead;
      
      cyclic = 1;
      empty_cycle &= !(sizeof(i2->error_lookahead));
    }
    i->count = 0x7fffffff;
    
    if (cyclic) {
      if (verbose) {
	werror(sprintf("Cyclic item\n%s\n",
		       item_to_string(i)));
      }
      conflict_func(empty_cycle && !(sizeof(i->error_lookahead)));
    }
  }
}

static private void shift_conflict(int empty)
{
  /* Ignored */
}

static private void handle_shift_conflicts()
{
  item_stack = Stack.stack();

  /* Initialize the counter */
  for (int index = 0; index < s_q->tail; index++) {
    foreach (s_q->arr[index]->items, object(item) i) {
      if ((i->offset != sizeof(i->r->symbols)) &&
	  (intp(i->r->symbols[i->offset])) &&
	  (!i->master_item)) {
	/* Nonterminal master item */
	i->counter = 0;
      } else {
	i->counter = 0x7fffffff;
      }
    }
  }

  for (int index = 0; index < s_q->tail; index++) {
    foreach (s_q->arr[index]->items, object(item) i) {
      if (!i->number) {
	traverse_items(i, shift_conflict);
      }
    }
  }
}

static private void follow_conflict(int empty)
{
}

static private void handle_follow_conflicts()
{
  item_stack = Stack.stack();

  /* Initialize the counter */
  for (int index = 0; index < s_q->tail; index++) {
    foreach (s_q->arr[index]->items, object(item) i) {
      if ((i->offset != sizeof(i->r->symbols)) &&
	  (intp(i->r->symbols[i->offset])) &&
	  (!i->master_item)) {
	/* Nonterminal master item */
	i->counter = 0;
      } else {
	i->counter = 0x7fffffff;
      }
    }
  }

  for (int index = 0; index < s_q->tail; index++) {
    foreach (s_q->arr[index]->items, object(item) i) {
      if (!i->number) {
	traverse_items(i, shift_conflict);
      }
    }
  }
}

static private int go_through(object(kernel) state, object(rule) r, int offset,
			      object(item) current_item)
{
  int index;
  object(item) i, master;

  for (index = 0; index < sizeof(state->items); index++) {
    if ((state->items[index]->r == r) &&
	(state->items[index]->offset == offset)) {
      /* Found the index for the current rule and offset */
      break;
    }
  }

  /* What to do if not found? */
  if (index == sizeof(state->items)) {
    werror(sprintf("go_through: item with offset %d in rule\n%s\n"
		   "not found in state\n%s\n",
		   offset, rule_to_string(r), state_to_string(state)));
    werror(sprintf("Backtrace:\n%s\n", describe_backtrace(backtrace())));
    return(0);
  }

  i = state->items[index];
  if (i->master_item) {
    master = i->master_item;
  } else {
    master = i;
  }

  if (i->offset < sizeof(i->r->symbols)) {
    if (go_through(i->next_state, i->r, i->offset + 1, current_item)) {
      /* Nullable */
      if ((master->offset < sizeof(master->r->symbols)) &&
	  (intp(master->r->symbols[master->offset]))) {
	/* Don't include ourselves */
	if (master != current_item) {
	  master->relation[current_item] = 1;
	}
      }
      return(nullable[i->r->symbols[i->offset]]);
    } else {
      return (0);	/* Not nullable */
    }
  } else {
    /* At end of rule */
    master->relation[current_item] = 1;
    return (1);		/* Always nullable */
  }
}

static private int repair(object(kernel) state, multiset(int|string) conflicts)
{
  multiset(int|string) conflict_set = (<>);

  if (verbose) {
    werror(sprintf("Repairing conflict in state:\n%s\n"
		   "Conflicts on (< %s >)\n",
		   state_to_string(state),
		   map(indices(conflicts), symbol_to_string) * ", "));
  }
  
  foreach (indices(conflicts), int|string symbol) {
    /* Initialize some vars here */
    int reduce_count = 0;
    int shift_count = 0;
    int reduce_rest = 0;
    int shift_rest = 0;
    int only_operators = 1;
    object(priority) shift_pri, reduce_pri, pri;
    object(rule) min_rule = 0;
    int conflict_free;

    /* Analyse the items */
    /* This loses if there are reduce-reduce conflicts,
     * or shift-shift conflicts
     */
    foreach (state->items, object(item) i) {
      if (i->offset == sizeof(i->r->symbols)) {
	if (i->direct_lookahead[symbol]) {
	  /* Reduction */
	  reduce_count++;
	  /* ******************* BUGGY! *********************
	  if (i->r->priority) {
	    reduce_pri = i->r->pri;
	  } else {
	  */
	    only_operators = 0;
	    /*
	  }
	  */
	  if ((!min_rule) || (i->r->number < min_rule->number)) {
	    min_rule = i->r;
	  }
	}
      } else if (!intp(i->r->symbols[i->offset])) {
	if (i->r->symbols[i->offset] == symbol) {
	  /* Shift */
	  shift_count++;

	  if (operator_priority[symbol]) {
	    shift_pri = operator_priority[symbol];
	  } else {
	    only_operators = 0;
	  }
	}
      }
    }

    if (only_operators) {
      if (reduce_pri->value > shift_pri->value) {
	pri = reduce_pri;
      } else {
	pri = shift_pri;
      }

      foreach (state->items, object(item) i) {
	if (i->offset == sizeof(i->r->symbols)) {
	  if (i->direct_lookahead[symbol]) {
	    /* *************************** BUGGY PRIORITY HANDLING ******
	    if (i->r->pri->value < pri->value) {
	      if (verbose) {
		werror(sprintf("Ignoring reduction of item\n%s\n"
			       "on lookahead %s (Priority %d < %d)\n",
			       item_to_string(i),
			       symbol_to_string(symbol),
			       i->r->pri->value, pri->value));
	      }
	      i->direct_lookahead[symbol] = 0;
	      if (!sizeof(indices(i->direct_lookahead))) {
		i->direct_lookahead = (<>);
	      }
	    } else */ if ((pri->assoc >= 0) &&
		       (shift_pri->value == pri->value)) {
	      if (verbose) {
		werror(sprintf("Ignoring reduction of item\n%s\n"
			       "on lookahead %s (Right associative)\n",
			       item_to_string(i),
			       symbol_to_string(symbol)));
	      }
	      i->direct_lookahead[symbol] = 0;
	      if (!sizeof(indices(i->direct_lookahead))) {
		i->direct_lookahead = (<>);
	      }
	    } else {
	      if (verbose) {
		werror(sprintf("Kept item\n%s\n"
			       "on lookahead %s\n",
			       item_to_string(i),
			       symbol_to_string(symbol)));
	      }
	      reduce_rest++;
	    }
	  } else if (i->r->symbols[i->offset] == symbol) {
	    /* Shift */
	    if (shift_pri->value < pri->value) {
	      /************* BUGGY PRIORITY HANDLING ***********
	      if (verbose) {
		werror(sprintf("Ignoring shift on item\n%s\n"
			       "on lookahead %s (Priority %d < %d)\n",
			       item_to_string(i),
			       symbol_to_string(symbol),
			       i->r->pri->value, pri->value));
	      }
	      */
	      i->direct_lookahead = (<>);
	    } else if ((pri->assoc <= 0) &&
		       (reduce_pri->value == pri->value)) {
	      if (verbose) {
		werror(sprintf("Ignoring shift on item\n%s\n"
			       "on lookahead %s (Left associative)\n",
			       item_to_string(i),
			       symbol_to_string(symbol)));
	      }
	      i->direct_lookahead = (<>);
	    } else {
	      if (verbose) {
		werror(sprintf("Kept item\n%s\n"
			       "on lookahead %s\n",
			       item_to_string(i),
			       symbol_to_string(symbol)));
	      }
	      shift_rest++;
	    }
	  }
	}
      }
    } else {
      /* Not only operators */
      if (shift_count) {
	/* Prefer shifts */
	foreach (state->items, object(item) i) {
	  if (i->offset == sizeof(i->r->symbols)) {
	    /* Reduction */
	    if (i->direct_lookahead[symbol]) {
	      if (verbose) {
		werror(sprintf("Ignoring reduction on item\n%s\n"
			       "on lookahead %s (can shift)\n",
			       item_to_string(i),
			       symbol_to_string(symbol)));
	      }
	      i->direct_lookahead[symbol] = 0;
	      if (!sizeof(indices(i->direct_lookahead))) {
		i->direct_lookahead = (<>);
	      }
	    }
	  } else {
	    /* Shift */
	    if (i->r->symbols[i->offset] == symbol) {
	      if (verbose) {
		werror(sprintf("Kept item\n%s\n"
			       "on lookahead (shift)%s\n",
			       item_to_string(i),
			       symbol_to_string(symbol)));
	      }
	      shift_rest++;
	    }
	  }
	}
      } else {
	/* Select the first reduction */
	foreach (state->items, object(item) i) {
	  if (i->r == min_rule) {
	    if (verbose) {
	      werror(sprintf("Kept item\n%s\n"
			     "on lookahead %s (first rule)\n",
			     item_to_string(i),
			     symbol_to_string(symbol)));
	    }
	    reduce_rest++;
	  } else {
	    if (verbose) {
	      werror(sprintf("Ignoring reduction on item\n%s\n"
			     "on lookahead %s (not first rule)\n",
			     item_to_string(i),
			     symbol_to_string(symbol)));
	    }
	    i->direct_lookahead[symbol] = 0;
	    if (!sizeof(indices(i->direct_lookahead))) {
	      i->direct_lookahead = (<>);
	    }
	  }
	}
      }
    }

    conflict_free = 0;

    if (reduce_rest > 1) {
      if (shift_rest) {
	werror(sprintf("Error: Shift-Reduce-Reduce conflict on lookahead %s\n",
		       symbol_to_string(symbol)));
      } else {
	werror(sprintf("Error: Reduce-Reduce conflict on lookahead %s\n",
		       symbol_to_string(symbol)));
      }
    } else if (reduce_rest) {
      if (shift_rest) {
	werror(sprintf("Error: Shift-Reduce conflict on lookahead %s\n",
		       symbol_to_string(symbol)));
      } else {
	/* REDUCE
	 *
	 * No other rule left -- conflict resolved!
	 */
	conflict_free = 1;
      }
    } else {
      /* SHIFT
       *
       * All reductions removed -- conflict resolved!
       */
      conflict_free = 1;
    }
    if (conflict_free) {
      if (reduce_count > 1) {
	if (shift_count) {
	  if (only_operators) {
	    werror(sprintf("Repaired Shift-Reduce-Reduce conflict on %s\n",
			   symbol_to_string(symbol)));
	  } else {
	    werror(sprintf("Warning: Repaired Shift-Reduce-Reduce conflict on %s\n",
			   symbol_to_string(symbol)));
	  }
	} else {
	  if (only_operators) {
	    werror(sprintf("Repaired Reduce-Reduce conflict on %s\n",
			   symbol_to_string(symbol)));
	  } else {
	    werror(sprintf("Warning: Repaired Reduce-Reduce conflict on %s\n",
			   symbol_to_string(symbol)));
	  }
	}
      } else if (reduce_count) {
	if (shift_count) {
	  if (only_operators) {
	    werror(sprintf("Repaired Shift-Reduce conflict on %s\n",
			   symbol_to_string(symbol)));
	  } else {
	    werror(sprintf("Warning: Repaired Shift-Reduce conflict on %s\n",
			   symbol_to_string(symbol)));
	  }
	} else {
	  /* No conflict */
	  werror(sprintf("No conflict on symbol %s (Plain REDUCE)\n",
			 symbol_to_string(symbol)));
	}
      } else {
	/* No conflict */
	werror(sprintf("No conflict on symbol %s (SHIFT)\n",
		       symbol_to_string(symbol)));
      }
	      
    } else {
      /* Still conflicts left on this symbol */
      conflict_set[symbol] = 1;
    }
  }
  
  if (sizeof(indices(conflict_set))) {
    werror(sprintf("Still conflicts remaining in state\n%s\n"
		   "on symbols (< %s >)\n",
		   state_to_string(state),
		   map(indices(conflict_set), symbol_to_string) * ", "));
    return (ERROR_CONFLICTS);
  } else {
    if (verbose) {
      werror("All conflicts removed!\n");
    }
    return (0);
  }
}

//. - compile
//.   Compiles the grammar into a parser, so that parse() can be called.
int compile()
{
  int error = 0;	/* No error yet */
  int state_no = 0;	/* DEBUG INFO */
  object(kernel) state;
  multiset(int|string) symbols, conflicts;

  s_q = state_queue();
  s_q->push_if_new(first_state());

  /* First make LR(0) states */

  while (state = s_q->next()) {

    if (verbose) {
      werror(sprintf("Compiling state %d:\n%s", state_no++,
		     state_to_string(state) + "\n"));
    }

    /* Probably better implemented as a stack */
    foreach (indices(make_goto_set(state)), int|string symbol) {
      do_goto(state, symbol);
    }
  }

  /* Compute nullables */
  /* Done during add_rule */
  if (verbose) {
    werror(sprintf("Nullable nonterminals: (< %s >)\n",
		   map(indices(nullable), symbol_to_string) * ", "));
  }

  /* Mark Transition and Reduction master items */
  for (int index = 0; index < s_q->tail; index++) {
    mapping(int|string : object(item)) master_item =([]);
    
    foreach (s_q->arr[index]->items, object(item) i) {
      if (i->offset < sizeof(i->r->symbols)) {
	/* This is not a reduction item, which represent themselves */
	int|string symbol = i->r->symbols[i->offset];

	if (!(i->master_item = master_item[symbol])) {
	  master_item[symbol] = i;
	}
      }
    }
  }

  /* Probably OK so far */

  /* Calculate look-ahead sets (DR and relation) */
  for (int index = 0; index < s_q->tail; index++) {
    foreach (s_q->arr[index]->items, object(item) i) {
      if ((!i->master_item) && (i->offset != sizeof(i->r->symbols)) &&
	  (intp(i->r->symbols[i->offset]))) {
	/* This is a non-terminal master item */
	foreach (i->next_state->items, object(item) i2) {
	  int|string symbol;

	  if (!i2->master_item) {
	    /* Master item */
	    if (i2->offset != sizeof(i2->r->symbols)) {
	      if (intp(symbol = i2->r->symbols[i2->offset])) {
		if (nullable[symbol]) {
		  /* Add the item to the look-ahead relation set */
		  i->relation[i2] = 1;
		}
	      } else {
		/* Add the string to the direct look-ahead set (DR) */
		i->direct_lookahead[symbol] = 1;
	      }
	    }
	  }
	}
      }
    }
  }

  /* Handle SHIFT-conflicts */
  handle_shift_conflicts();

  /* Check the shift sets */
  /* (Is this needed?)
   * Yes - initializes error_lookahead
   */
  for (int index = 0; index < s_q->tail; index++) {
    foreach (s_q->arr[index]->items, object(item) i) {
      if ((!i->master_item) &&
	  (i->offset != sizeof(i->r->symbols)) &&
	  (intp(i->r->symbols[i->offset]))) {
	i->error_lookahead = copy_value(i->direct_lookahead);
      }
    }
  }

  /* Compute lookback-sets */
  for (int index = 0; index < s_q->tail; index++) {
    foreach (s_q->arr[index]->items, object(item) transition) {
      int|string symbol;

      if ((transition->offset != sizeof(transition->r->symbols)) &&
	  (intp(symbol = transition->r->symbols[transition->offset])) &&
	  (!transition->master_item)) {
	/* NonTerminal and master item*/

	/* Find items which can reduce to the nonterminal from above */
	foreach (s_q->arr[index]->items, object(item) i2) {
	  if ((!i2->offset) &&
	      (i2->r->nonterminal == symbol)) {
	    if (sizeof(i2->r->symbols)) {
	      if (go_through(i2->next_state, i2->r, i2->offset+1,
			     transition)) {
		/* Nullable */
		object(item) master = i2;
		if (i2->master_item) {
		  master = i2->master_item;
		}
		/* Is this a nonterminal transition? */
		if ((master->offset != sizeof(master->r->symbols)) &&
		    (intp(master->r->symbols[master->offset]))) {
		  /* Don't include ourselves */
		  if (master != transition) {
		    master->relation[transition] = 1;
		  }
		}
	      }
	    } else {
	      i2->relation[transition] = 1;
	    }
	  }
	}
      }
    }
  }

  /* Handle follow-conflicts */
  handle_follow_conflicts();

  /* Compute the lookahead (LA) */
  for (int index = 0; index < s_q->tail; index++) {
    foreach (s_q->arr[index]->items, object(item) i) {
      if (i->offset == sizeof(i->r->symbols)) {
	/* Reduction item (always a master item) */

	/* Calculate Look-ahead for all items in look-back set */
	
	foreach (indices(i->relation), object(item) lookback) {
	  /* Add Follow(i2) to the lookahead-set */

	  i->direct_lookahead |= lookback->direct_lookahead;
	}
      }
    }
  }

  /* Probably OK from this point onward */

  /* Check for conflicts */
  for (int index = 0; index < s_q->tail; index++) {
    object(kernel) state = s_q->arr[index];

    conflicts = (<>);
    symbols = (<>);

    foreach (state->items, object(item) i) {
      if (i->offset == sizeof(i->r->symbols)) {
	/* Reduction */
	conflicts |= i->direct_lookahead & symbols;
	symbols |= i->direct_lookahead;
      } else if (!i->master_item) {
	string|int symbol;

	/* Only master items, since we get Shift-Shift conflicts otherwise */

	if (!intp(symbol = i->r->symbols[i->offset])) {
	  /* Shift on terminal */
	  if (symbols[symbol]) {
	    conflicts[symbol] = 1;
	  } else {
	    symbols[symbol] = 1;
	  }
	}
      }
    }
    if (sizeof(conflicts)) {
      /* Repair conflicts */
      error = repair(state, conflicts);
    } else if (verbose) {
      werror(sprintf("No conflicts in state:\n%s\n",
		     state_to_string(s_q->arr[index])));
    }
  }

  /* Compile action tables */
  for (int index = 0; index < s_q->tail; index++) {
    object(kernel) state = s_q->arr[index];

    state->action = ([]);

    foreach (state->items, object(item) i) {
      if (i->next_state) {
	/* SHIFT */
	state->action[i->r->symbols[i->offset]] = i->next_state;
      } else {
	foreach (indices(i->direct_lookahead), int|string symbol) {
	  state->action[symbol] = i->r;
	}
      }
    }
  }
  start_state = s_q->arr[0];

  return (error);
}

//. - parse
//.   Parse the input according to the compiled grammar.
//.   The last value reduced is returned.
//. NOTA BENE:
//.   The parser must have been compiled (with compile()), and a scanner
//.   been set (with set_scanner()) prior to calling this function.
//. BUGS
//.   Errors should be throw()n.
//. > scanner
//.   The scanner function. It returns the next symbol from the input.
//.   It should either return a string (terminal) or an array with
//.   a string (terminal) and a mixed (value).
//.   EOF is indicated with the empty string.
//. > action_object
//.   Object used to resolve those actions that have been specified as
//.   strings.
mixed parse(function(void:string|array(string|mixed)) scanner,
	    void|object action_object)
{
  object(Stack.stack) value_stack = Stack.stack();
  object(Stack.stack) state_stack = Stack.stack();
  object(kernel) state = start_state;

  string input;
  mixed value;

  error = 0;	/* No parse error yet */

  if (!scanner || !functionp(scanner)) {
    werror("parser->parse(): scanner not set!\n");
    error = ERROR_NO_SCANNER;
    return(0);
  }

  value = scanner();

  if (arrayp(value)) {
    input = value[0];
    value = value[1];
  } else {
    input = value;
  }

  while (1) {
    mixed a = state->action[input];

    if (object_program(a) == rule) {
      /* REDUCE */
      function (mixed ...:mixed) func = 0;

      if (verbose) {
	werror(sprintf("Reducing according to rule\n%s\n",
		       rule_to_string(a)));
      }

      if (a->action) {
	if (functionp(a->action)) {
	  func = a->action;
	} else if (stringp(a->action)) {
	  if (action_object) {
	    func = action_object[a->action];
	    if (!func) {
	      werror(sprintf("Missing action \"%s\" in object\n",
			     a->action));
	      error |= ERROR_MISSING_ACTION;
	    } else if (!functionp(func)) {
	      werror(sprintf("Bad type (%s) for action \"%s\" in object\n",
			     typeof(func), a->action));
	      error |= ERROR_BAD_ACTION_TYPE;
	      func = 0;
	    }
	  } else {
	    werror(sprintf("Missing object for action \"%s\"\n",
			   a->action));
	    error |= ERROR_NO_OBJECT;
	  }
	} else {
	  werror(sprintf("Unsupported action type \"%s\" (%s)\n",
			 a->action, typeof(a->action)));
	  error |= ERROR_BAD_ACTION_TYPE;
	}
      }
      if (func) {
	if (sizeof(a->symbols)) {
	  value_stack->push(a->action(@value_stack->pop(sizeof(a->symbols))));
	} else {
	  value_stack->push(a->action());
	}
      } else {
	if (sizeof(a->symbols)) {
	  value_stack->push(value_stack->pop(sizeof(a->symbols))[0]);
	} else {
	  value_stack->push(0);
	}
      }
      if (sizeof(a->symbols)) {
	state = state_stack->pop(sizeof(a->symbols))[0];
      }
      state_stack->push(state);
      state = state->action[a->nonterminal];	/* Goto */
    } else if (a) {
      /* SHIFT or ACCEPT */
      if (input == "") {
	/* Only the final state is allowed to shift on ""(EOF) */
	/* ACCEPT */
	return(value_stack->pop());
      }
      /* SHIFT */
      if (verbose) {
	werror(sprintf("Shifting \"%s\", value \"%O\"\n", input, value));
      }
      value_stack->push(value);
      state_stack->push(state);
      state = a;

      value = scanner();

      if (arrayp(value)) {
	input = value[0];
	value = value[1];
      } else {
	input = value;
      }
    } else {
      /* ERROR */
      if (input = "") {
	/* At end of file */
	error |= ERROR_EOF;

	if (value_stack->ptr != 1) {
	  if (value_stack->ptr) {
	    werror(sprintf("Error: Bad state at EOF -- Throwing \"%O\"\n",
			   value_stack->pop()));
	    state=state_stack->pop();
	  } else {
	    werror(sprintf("Error: Empty stack at EOF!\n"));
	    return (0);
	  }
	} else {
	  werror("Error: Bad state at EOF\n");
	  return(value_stack->pop());
	}
      } else {
	error |= ERROR_SYNTAX;

	werror("Error: Bad input: \""+input+"\"(\""+value+"\")\n");

	value = scanner();
	
	if (arrayp(value)) {
	  input = value[0];
	  value = value[1];
	} else {
	  input = value;
	}
      }
    }
  }
}
