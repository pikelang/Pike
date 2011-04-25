/*
 * $Id$
 *
 * A BNF-grammar in Pike.
 * Compiles to a LALR(1) state-machine.
 *
 * Henrik Grubbström 1996-11-24
 */

#pike 7.2

//! This object implements an LALR(1) parser and compiler.
//!
//! Normal use of this object would be:
//!
//! @pre{
//! {add_rule, set_priority, set_associativity}*
//! set_symbol_to_string
//! compile
//! {parse}*
//! @}

/*
 * Includes
 */

import Array;

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

import LR;

/*
 * Classes
 */

//! Implements an LR(1) state
class kernel {

  //! Used to check if a rule already has been added when doing closures.
  multiset(object(rule)) rules = (<>);

  //! Contains the items in this state.
  array(object(item)) items = ({});

  //! Used to lookup items given rule and offset
  mapping(int:object(item)) item_id_to_item = ([]);

  //! Contains the items whose next symbol is this non-terminal.
  mapping(int : multiset(object(item))) symbol_items = ([]);

  //! The action table for this state
  //!
  //! @pre{
  //! object(kernel)    SHIFT to this state on this symbol.
  //! object(rule)      REDUCE according to this rule on this symbol.
  //! @}
  mapping(int|string : object(kernel)|object(rule)) action = ([]);

  //! The symbols that closure has been called on.
  multiset closure_set = (<>);

  /*
   * Functions
   */

  //! Add an item to the state.
  void add_item(object(item) i)
  {
    int|string symbol;
  
    items += ({ i });
    item_id_to_item[i->item_id] = i;

    if (i->offset < sizeof(i->r->symbols)) {
      symbol = i->r->symbols[i->offset];

      if (symbol_items[symbol]) {
	symbol_items[symbol][i] = 1;
      } else {
	symbol_items[symbol] = (< i >);
      }
    }
  }

  //! Make the closure of this state.
  //!
  //! @param nonterminal
  //!   Nonterminal to make the closure on.
  void closure(int nonterminal)
  {
    closure_set[nonterminal] = 1;
    if (grammar[nonterminal]) {
      foreach (grammar[nonterminal], object(rule) r) {
	if (!rules[r]) {

	  object(item) new_item = item();
	
	  new_item->r = r;
	  new_item->item_id = r->number;

	  // Not needed, since 0 is the default.
	  // new_item->offset = 0;
	  // rules[r] is set by the post-increment above.
	  rules[r] = 1;

	  add_item(new_item);

	  if (sizeof(r->symbols) && intp(r->symbols[0]) &&
	      !closure_set[r->symbols[0]]) {
	    closure(r->symbols[0]);
	  }
	}
      }
    } else {
      werror(sprintf("Error: Definition missing for non-terminal %s\n",
		     symbol_to_string(nonterminal)));
      error |= ERROR_MISSING_DEFINITION;
    }
  }

  //! Make the goto-set of this state.
  multiset(int|string) goto_set()
  {
    multiset(int|string) set = (<>);

    foreach (items, object(item) i) {
      if (i->offset != sizeof(i->r->symbols)) {
	set[i->r->symbols[i->offset]] = 1;
      }
    }

    if (verbose) {
      werror(sprintf("goto_set()=> (< %s >)\n",
		     map(indices(set), symbol_to_string) * ", "));
    }

    return (set);
  }

  //! Generates the state reached when doing goto on the specified symbol.
  //! i.e. it compiles the LR(0) state.
  //!
  //! @param symbol
  //!   Symbol to make goto on.
  object(kernel) do_goto(int|string symbol)
  {
    multiset(object(item)) items;

    if (verbose) {
      werror(sprintf("Performing GOTO on <%s>\n", symbol_to_string(symbol)));
    }

    items = symbol_items[symbol];
    if (items) {
      array(int) item_ids = Array.map(sort(indices(items)->item_id),
				      `+, 1);
      string kernel_hash = sprintf("%@4c", item_ids);

      object(kernel) new_state = known_states[kernel_hash];

      if (!new_state) {
	known_states[kernel_hash] = new_state = kernel();
    
	foreach (indices(items), object(item) i) {
	  int|string lookahead;

	  object(item) new_item = item();
	  object(rule) r;
	  int offset = i->offset;

	  new_item->offset = ++offset;
	  new_item->r = r = i->r;
	  new_item->item_id = r->number + offset;

	  new_state->add_item(new_item);

	  if ((offset != sizeof(r->symbols)) &&
	      intp(lookahead = r->symbols[offset]) &&
	      !new_state->closure_set[lookahead]) {
	    new_state->closure(lookahead);
	  }
	}

	s_q->push(new_state);
      } else {
	// werror("Known state\n");
      }
      /* DEBUG */

      if (verbose) {
	werror(sprintf("GOTO on %s generated state:\n%s\n",
		       symbol_to_string(symbol),
		       state_to_string(new_state)));
      }

      /* !DEBUG */

      if (items) {
	foreach (indices(items), object(item) i) {
	  i->next_state = new_state;
	}
      }
    } else {
      werror(sprintf("WARNING: do_goto() on unknown symbol <%s>\n",
		     symbol_to_string(symbol)));
    }
  }

};

//! This is a queue, which keeps the elements even after they are retrieved.
class state_queue {

  //! Index of the head of the queue.
  int head;

  //! Index of the tail of the queue.
  int tail;

  //! The queue itself.
  array(object(kernel)) arr = allocate(64);

  //! Pushes the state on the queue.
  //!
  //! @param state
  //!   State to push.
  object(kernel) push(object(kernel) state)
  {
    if (tail == sizeof(arr)) {
      arr += allocate(tail);
    }
    arr[tail++] = state;

    return(state);
  }

  //! Return the next state from the queue.
  int|object(kernel) next()
  {
    if (head == tail) {
      return(0);
    } else {
      return(arr[head++]);
    }
  }
}

//! The grammar itself.
mapping(int|string : array(object(rule))) grammar = ([]);

/* Priority table for terminal symbols */
static private mapping(string : object(priority)) operator_priority = ([]);

static private multiset(mixed) nullable = (< >);

#if 0
static private mapping(mixed : multiset(object(rule))) derives = ([]);

/* Maps from symbol to which rules may start with that symbol */
static private mapping(mixed : multiset(object(rule))) begins = ([]);
#endif /* 0 */


/* Maps from symbol to which rules use that symbol
 * (used for finding nullable symbols)
 */
static private mapping(int : multiset(object(rule))) used_by = ([]);

//! The initial LR0 state.
object(kernel) start_state;

//! Verbosity level
//!
//! @int
//!   @value 0
//!    None
//!   @value 1
//!    Some
//! @endint
int verbose=1;

//! Error code
int error=0;

/* Number of next rule (used only for conflict resolving) */
static private int next_rule_number = 1;

//! LR0 states that are already known to the compiler.
mapping(string:object(kernel)) known_states = ([]);

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

//! Pretty-prints a rule to a string.
//!
//! @param r
//!   Rule to print.
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

//! Pretty-prints an item to a string.
//!
//! @param i
//!   Item to pretty-print.
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

//! Pretty-prints a state to a string.
//!
//! @param state
//!   State to pretty-print.
string state_to_string(object(kernel) state)
{
  return (map(state->items, item_to_string) * "\n");
}

//! Pretty-prints the current grammar to a string.
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

//! Implements casting.
//!
//! @param type
//!   Type to cast to.
mixed cast(string type)
{
  if (type == "string") {
    return(cast_to_string());
  }
  throw(({ sprintf("Cast to %s not supported\n", type), backtrace() }));
}

/* Here come the functions that actually do some work */

//! Sets the priority of a terminal.
//!
//! @param terminal
//!   Terminal to set the priority for.
//! @param pri_val
//!   Priority; higher = prefer this terminal.
void set_priority(string terminal, int pri_val)
{
  object(priority) pri;

  if (pri = operator_priority[terminal]) {
    pri->value = pri_val;
  } else {
    operator_priority[terminal] = priority(pri_val, 0);
  }
}

//! Sets the associativity of a terminal.
//!
//! @param terminal
//!   Terminal to set the associativity for.
//! @param assoc
//!   Associativity; negative - left, positive - right, zero - no associativity.
void set_associativity(string terminal, int assoc)
{
  object(priority) pri;

  if (pri = operator_priority[terminal]) {
    pri->assoc = assoc;
  } else {
    operator_priority[terminal] = priority(0, assoc);
  }
}

//! Sets the symbol to string conversion function.
//! The conversion function is used by the various *_to_string functions
//! to make comprehensible output.
//!
//! @param s_to_s
//! Symbol to string conversion function.
//! If zero or not specified, use the built-in function.
void set_symbol_to_string(void|function(int|string:string) s_to_s)
{
  if (s_to_s) {
    symbol_to_string = s_to_s;
  } else {
    symbol_to_string = builtin_symbol_to_string;
  }
}

//! Add a rule to the grammar.
//!
//! @param r
//! Rule to add.
void add_rule(object(rule) r)
{
  array(object(rule)) rules;
  int|string symbol;

  /* DEBUG */
  if (verbose) {
    werror("Adding rule: " + rule_to_string(r) + "\n");
  }

  /* !DEBUG */

  r->number = next_rule_number;
  /* Reserve space for the items generatable from this rule. */
  next_rule_number += sizeof(r->symbols) + 1;

  /* First add the rule to the grammar */
  if (grammar[r->nonterminal]) {
    grammar[r->nonterminal] += ({ r });
  } else {
    grammar[r->nonterminal] = ({ r });
  }

  /* Then see if it is nullable */
  if (!r->has_tokens) {
    object(ADT.Stack) new_nullables = ADT.Stack(1024);

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
  } else {
    /* Check if it's an operator */
    foreach(r->symbols, symbol) {
      if (operator_priority[symbol]) {
	r->pri = operator_priority[symbol];
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

static private object(kernel) first_state()
{
  object(kernel) state = kernel();

  array(int) first_state_item_ids = sort(grammar[0]->number);
  string kernel_hash = sprintf("%@4c", first_state_item_ids);
  known_states[kernel_hash] = state;

  foreach (grammar[0], object(rule) r) {
    if (!state->rules[r]) {
      object(item) i = item();

      i->r = r;
      // Not needed since 0 is the default.
      // i->offset = 0;
      i->item_id = r->number;

      state->add_item(i);
      state->rules[r] = 1;	/* Since this is an item with offset 0 */

      if ((sizeof(r->symbols)) &&
	  (intp(r->symbols[0]))) {
	state->closure(r->symbols[0]);
      }
    }
  }
  return(state);
}

//! Contains all states used.
//! In the queue-part are the states that remain to be compiled.
object(state_queue) s_q;

static private object(ADT.Stack) item_stack;

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
  item_stack = ADT.Stack(131072);

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
  item_stack = ADT.Stack(131072);

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
	traverse_items(i, follow_conflict);
      }
    }
  }
}

static private int go_through(object(kernel) state, int item_id,
			      object(item) current_item)
{
  int index;
  object(item) i, master;

  i = state->item_id_to_item[item_id];

  /* What to do if not found? */
  if (!i) {
    werror(sprintf("go_through: item %d not found in state\n"
		   "%s\n",
		   item_id,
		   state_to_string(state)));
    werror(sprintf("Backtrace:\n%s\n", describe_backtrace(backtrace())));
    return(0);
  }

  if (i->master_item) {
    master = i->master_item;
  } else {
    master = i;
  }

  if (i->offset < sizeof(i->r->symbols)) {
    if (go_through(i->next_state, item_id + 1, current_item)) {
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
	  if (pri = i->r->pri) {
	    if (!reduce_pri || (pri->value > reduce_pri->value)) {
	      reduce_pri = pri;
	    }
	  } else {
	    only_operators = 0;
	  }

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
	  /* Reduce */
	  if (i->direct_lookahead[symbol]) {
	    object(priority) new_pri;
	    if ((new_pri = i->r->pri)->value < pri->value) {
	      if (verbose) {
		werror(sprintf("Ignoring reduction of item\n%s\n"
			       "on lookahead %s (Priority %d < %d)\n",
			       item_to_string(i),
			       symbol_to_string(symbol),
			       new_pri->value, pri->value));
	      }
	      i->direct_lookahead[symbol] = 0;
	      if (!sizeof(indices(i->direct_lookahead))) {
		i->direct_lookahead = (<>);
	      }
	    } else if ((pri->assoc >= 0) &&
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
	  }
	} else if (i->r->symbols[i->offset] == symbol) {
	  /* Shift */
	  if (shift_pri->value < pri->value) {
	    if (verbose) {
	      werror(sprintf("Ignoring shift on item\n%s\n"
			     "on lookahead %s (Priority %d < %d)\n",
			     item_to_string(i),
			     symbol_to_string(symbol),
			     i->r->pri->value, pri->value));
	    }
	    i->direct_lookahead = (<>);
	    i->next_state = 0;
	  } else if ((pri->assoc <= 0) &&
		     (reduce_pri->value == pri->value)) {
	    if (verbose) {
	      werror(sprintf("Ignoring shift on item\n%s\n"
			     "on lookahead %s (Left associative)\n",
			     item_to_string(i),
			     symbol_to_string(symbol)));
	    }
	    i->direct_lookahead = (<>);
	    i->next_state = 0;
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
	    if (verbose) {
	      werror(sprintf("Repaired Shift-Reduce-Reduce conflict on %s\n",
			     symbol_to_string(symbol)));
	    }
	  } else {
	    werror(sprintf("Warning: Repaired Shift-Reduce-Reduce conflict on %s\n",
			   symbol_to_string(symbol)));
	  }
	} else {
	  if (only_operators) {
	    if (verbose) {
	      werror(sprintf("Repaired Reduce-Reduce conflict on %s\n",
			     symbol_to_string(symbol)));
	    }
	  } else {
	    werror(sprintf("Warning: Repaired Reduce-Reduce conflict on %s\n",
			   symbol_to_string(symbol)));
	  }
	}
      } else if (reduce_count) {
	if (shift_count) {
	  if (only_operators) {
	    if (verbose) {
	      werror(sprintf("Repaired Shift-Reduce conflict on %s\n",
			     symbol_to_string(symbol)));
	    }
	  } else {
	    werror(sprintf("Warning: Repaired Shift-Reduce conflict on %s\n",
			   symbol_to_string(symbol)));
	  }
	} else {
	  /* No conflict */
	  if (verbose) {
	    werror(sprintf("No conflict on symbol %s (Plain REDUCE)\n",
			   symbol_to_string(symbol)));
	  }
	}
      } else {
	/* No conflict */
	if (verbose) {
	  werror(sprintf("No conflict on symbol %s (SHIFT)\n",
			 symbol_to_string(symbol)));
	}
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

//! Compiles the grammar into a parser, so that parse() can be called.
int compile()
{
  int error = 0;	/* No error yet */
  int state_no = 0;	/* DEBUG INFO */
  object(kernel) state;
  multiset(int|string) symbols, conflicts;

  s_q = state_queue();
  s_q->push(first_state());

  /* First make LR(0) states */

#ifdef LR_PROFILE
  werror(sprintf("LR0: %f\n", gauge {
#endif /* LR_PROFILE */

    while (state = s_q->next()) {

      if (verbose) {
	werror(sprintf("Compiling state %d:\n%s", state_no++,
		       state_to_string(state) + "\n"));
      }

      /* Probably better implemented as a stack */
      foreach (indices(state->goto_set()), int|string symbol) {
	state->do_goto(symbol);
      }
    }
#ifdef LR_PROFILE
  }));
#endif /* LR_PROFILE */

  /* Compute nullables */
  /* Done during add_rule */
  if (verbose) {
    werror(sprintf("Nullable nonterminals: (< %s >)\n",
		   map(indices(nullable), symbol_to_string) * ", "));
  }

#ifdef LR_PROFILE
  werror(sprintf("Master items: %f\n", gauge {
#endif /* LR_PROFILE */

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

#ifdef LR_PROFILE
  }));
#endif /* LR_PROFILE */

  /* Probably OK so far */

#ifdef LR_PROFILE
  werror(sprintf("LA sets: %f\n", gauge {
#endif /* LR_PROFILE */

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

#ifdef LR_PROFILE
  }));
#endif /* LR_PROFILE */

#ifdef LR_PROFILE
  werror(sprintf("Handle shift: %f\n", gauge {
#endif /* LR_PROFILE */

    /* Handle SHIFT-conflicts */
    handle_shift_conflicts();

#ifdef LR_PROFILE
  }));
#endif /* LR_PROFILE */

#ifdef LR_PROFILE
  werror(sprintf("Check shift: %f\n", gauge {
#endif /* LR_PROFILE */

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

#ifdef LR_PROFILE
  }));
#endif /* LR_PROFILE */

#ifdef LR_PROFILE
  werror(sprintf("Lookback sets: %f\n", gauge {
#endif /* LR_PROFILE */

    /* Compute lookback-sets */
    for (int index = 0; index < s_q->tail; index++) {
      array(object(item)) items =  s_q->arr[index]->items;
      // Set up a lookup table to speedup lookups later.
      mapping(int:array(object(item))) lookup = ([]);
      foreach (items, object(item) i) {
	if (!i->offset) {
	  if (!lookup[i->r->nonterminal]) {
	    lookup[i->r->nonterminal] = ({ i });
	  } else {
	    lookup[i->r->nonterminal] += ({ i });
	  }
	}
      }
      foreach (items, object(item) transition) {
	int|string symbol;

	if ((!transition->master_item) &&
	    (transition->offset != sizeof(transition->r->symbols)) &&
	    (intp(symbol = transition->r->symbols[transition->offset]))) {
	  /* Master item and
	   * Not a reduction item and
	   * next symbol is a NonTerminal
	   */
	  if (!lookup[symbol]) {
	    // Foo? Shouldn't these always exist since we've made
	    // a closure earlier?
	    werror(sprintf("WARNING: No item for symbol <%s>\n"
			   "in state:\n"
			   "%s\n",
			   symbol_to_string(symbol),
			   state_to_string(s_q->arr[index])));
	    continue;
	  }

	  /* Find items which can reduce to the nonterminal from above */
	  foreach (lookup[symbol], object(item) i) {
	    if (sizeof(i->r->symbols)) {
	      if (go_through(i->next_state, i->item_id + 1, transition)) {
		/* Nullable */
		object(item) master = i;
		if (i->master_item) {
		  master = i->master_item;
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
	      i->relation[transition] = 1;
	    }
	  }
	}
      }
    }

#ifdef LR_PROFILE
  }));
#endif /* LR_PROFILE */

#ifdef LR_PROFILE
  werror(sprintf("Handle follow: %f\n", gauge {
#endif /* LR_PROFILE */

    /* Handle follow-conflicts */
    handle_follow_conflicts();

#ifdef LR_PROFILE
  }));
#endif /* LR_PROFILE */

#ifdef LR_PROFILE
  werror(sprintf("Compute LA: %f\n", gauge {
#endif /* LR_PROFILE */

    /* Compute the lookahead (LA) */
    for (int index = 0; index < s_q->tail; index++) {
      foreach (s_q->arr[index]->items, object(item) i) {
	if (i->offset == sizeof(i->r->symbols)) {
	  /* Reduction item (always a master item) */
	  
	  /* Calculate Look-ahead for all items in look-back set */
	
	  i->direct_lookahead=`|(i->direct_lookahead,
				 @indices(i->relation)->direct_lookahead);
	}
      }
    }

#ifdef LR_PROFILE
  }));
#endif /* LR_PROFILE */

  /* Probably OK from this point onward */

#ifdef LR_PROFILE
  werror(sprintf("Check conflicts: %f\n", gauge {
#endif /* LR_PROFILE */

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
	// int ov = verbose;
	// verbose = 1;
	error = repair(state, conflicts);
	// verbose = ov;
      } else if (verbose) {
	werror(sprintf("No conflicts in state:\n%s\n",
		       state_to_string(s_q->arr[index])));
      }
    }

#ifdef LR_PROFILE
  }));
#endif /* LR_PROFILE */

#ifdef LR_PROFILE
  werror(sprintf("Compile actions: %f\n", gauge {
#endif /* LR_PROFILE */

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

#ifdef LR_PROFILE
  }));
#endif /* LR_PROFILE */

#ifdef LR_PROFILE
  werror("DONE\n");
#endif /* LR_PROFILE */

  return (error);
}

//! Parse the input according to the compiled grammar.
//! The last value reduced is returned.
//!
//! @note
//!   The parser must have been compiled (with compile())
//!   prior to calling this function.
//!
//! @bugs
//!   Errors should be throw()n.
//!
//! @param scanner
//!   The scanner function. It returns the next symbol from the input.
//!   It should either return a string (terminal) or an array with
//!   a string (terminal) and a mixed (value).
//!   EOF is indicated with the empty string.
//!
//! @param action_object
//!   Object used to resolve those actions that have been specified as
//!   strings.
mixed parse(object|function(void:string|array(string|mixed)) scanner,
	    void|object action_object)
{
  object(ADT.Stack) value_stack = ADT.Stack(4096);
  object(ADT.Stack) state_stack = ADT.Stack(4096);
  object(kernel) state = start_state;

  string input;
  mixed value;

  error = 0;	/* No parse error yet */

  if (!functionp(scanner) &&
      !(objectp(scanner) && functionp(scanner->`()))) {
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

      if (verbose) {
	werror(sprintf("Reducing according to rule\n%s\n",
		       rule_to_string(a)));
      }

      if (a->action) {
	/* REDUCE */
	string|function (mixed ...:mixed) func = 0;

	if (stringp(func = a->action)) {
	  if (action_object) {
	    func = action_object[a->action];
	    if (!functionp(func)) {
	      if (!func) {
		werror(sprintf("Missing action \"%s\" in object\n",
			       a->action));
		error |= ERROR_MISSING_ACTION;
	      } else {
		werror(sprintf("Bad type (%s) for action \"%s\" in object\n",
			       typeof(func), a->action));
		error |= ERROR_BAD_ACTION_TYPE;
		func = 0;
	      }
	    }
	  } else {
	    werror(sprintf("Missing object for action \"%s\"\n",
			   a->action));
	    error |= ERROR_NO_OBJECT;
	    func = 0;
	  }
	}
	if (func) {
	  if (sizeof(a->symbols)) {
	    value_stack->push(func(@value_stack->pop(sizeof(a->symbols))));
	    state = state_stack->pop(sizeof(a->symbols))[0];
	  } else {
	    value_stack->push(a->action());
	  }
	} else {
	  // Default action.
	  if (sizeof(a->symbols)) {
#if 0
	    value_stack->push(value_stack->pop(sizeof(a->symbols))[0]);
#else /* !0 */
	    if (sizeof(a->symbols) > 1) {
	      value_stack->quick_pop(sizeof(a->symbols) - 1);
	    }
#endif /* 0 */
	    state = state_stack->pop(sizeof(a->symbols))[0];
	  } else {
	    value_stack->push(0);
	  }
	}
      } else {
	// Default action.
	if (sizeof(a->symbols)) {
#if 0
	  value_stack->push(value_stack->pop(sizeof(a->symbols))[0]);
#else /* !0 */
	  if (sizeof(a->symbols) > 1) {
	    value_stack->quick_pop(sizeof(a->symbols) - 1);
	  }
#endif /* 0 */
	  state = state_stack->pop(sizeof(a->symbols))[0];
	} else {
	  value_stack->push(0);
	}
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
