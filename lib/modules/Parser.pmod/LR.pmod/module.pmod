/*
 * $Id: module.pmod,v 1.12 2005/04/10 18:06:27 nilsson Exp $
 *
 * A BNF-grammar in Pike.
 * Compiles to a LALR(1) state-machine.
 *
 * Henrik Grubbström 1996-11-24
 */

#pike __REAL_VERSION__

//! LALR(1) parser generator.

#pragma strict_types

/*
 * Defines
 */

/* Errors during parsing */
/* Unexpected EOF */
constant ERROR_EOF=			1;
/* Syntax error in input */
constant ERROR_SYNTAX=			2;
/* Shift-Reduce or Reduce-Reduce */
constant ERROR_CONFLICTS=		4;
/* Action is missing from action object */
constant ERROR_MISSING_ACTION=		8;
/* Action is not a function */
constant ERROR_BAD_ACTION_TYPE=		16;
/* Action invoked by name, but no object given */
constant ERROR_NO_OBJECT=		32;
/* Scanner not set */
constant ERROR_NO_SCANNER=		64;
/* Missing definition of nonterminal */
constant ERROR_MISSING_DEFINITION=	128;

/*
 * Classes
 */

//!
//! Specifies the priority and associativity of a rule.
//!
class Priority
{
  //!   Priority value
  int value;

  //! Associativity
  //!
  //! @int
  //!   @value -1
  //!    Left
  //!   @value 0
  //!    None
  //!   @value 1
  //!    Right
  //! @endint
  int assoc;

  //! Create a new priority object.
  //!
  //! @param p
  //!  Priority.
  //! @param  a
  //!  Associativity.
  void create(int p, int a)
  {
    value = p;
    assoc = a;
  }
}

//!
//! This object is used to represent a BNF-rule in the LR parser.
//!
class Rule
{
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
  Priority pri;

  //! Create a BNF rule.
  //!
  //! @example
  //!   The rule
  //! 
  //!	   rule : nonterminal ":" symbols ";" { add_rule };
  //! 
  //!   might be created as
  //! 
  //!	   rule(4, ({ 9, ":", 5, ";" }), "add_rule");
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
  static void create(int nt, array(string|int) r, function|string|void a)
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
}


//! Severity level
enum SeverityLevel {
  NOTICE = 0,
  WARNING,
  ERROR,
};

//! Class handling reporting of errors and warnings.
class ErrorHandler
{
  //! Verbosity level
  //!
  //! @int
  //!   @value -1
  //!    Just errors.
  //!   @value 0
  //!    Errors and warnings.
  //!   @value 1
  //!    Also notices.
  //! @endint
  optional int(-1..1) verbose = 1;

  static constant severity_kind = ([ NOTICE:"Notice",
				     WARNING:"Warning",
				     ERROR:"Error" ]);

  void report(SeverityLevel level, string subsystem, string msg,
	      mixed ... args)
  {
    if (level > -verbose) {
      werror("%s: %s: "+msg+"\n",
	     severity_kind[level], subsystem, @args);
    }
  }

  //! Create a new error handler.
  //!
  //! @param verbosity
  //!   Level of verbosity.
  //!
  //! @seealso
  //!   @[verbose]
  static void create(int(-1..1)|void verbosity)
  {
    if (!zero_type(verbosity)) {
      verbose = verbosity;
    }
  }
}

//! This object implements an LALR(1) parser and compiler.
//!
//! Normal use of this object would be:
//!
//! @pre{
//! set_error_handler
//! {add_rule, set_priority, set_associativity}*
//! set_symbol_to_string
//! compile
//! {parse}*
//! @}
class Parser
{
  //! The grammar itself.
  mapping(int : array(Rule)) grammar = ([]);

  /* Priority table for terminal symbols */
  static mapping(string : Priority) operator_priority = ([]);

  static multiset(mixed) nullable = (< >);

#if 0
  static mapping(mixed : multiset(Rule)) derives = ([]);

  /* Maps from symbol to which rules may start with that symbol */
  static mapping(mixed : multiset(Rule)) begins = ([]);
#endif /* 0 */

  /* Maps from symbol to the rules that use the symbol
   * (used for finding nullable symbols)
   */
  static mapping(int : multiset(Rule)) used_by = ([]);

  //! The initial LR0 state.
  Kernel start_state;

  //! Error code
  int lr_error=0;

  /* Number of next rule (used only for conflict resolving) */
  static int next_rule_number = 1;

  //! LR0 states that are already known to the compiler.
  mapping(string:Kernel) known_states = ([]);

  //! Compile error and warning handler.
  function(SeverityLevel, string, string, mixed ...:void) error_handler =
    ErrorHandler()->report;

  void report(SeverityLevel level, string subsystem, string msg,
	      mixed ... args)
  {
    if (!error_handler) {
      error_handler = ErrorHandler()->report;
    }
    error_handler(level, subsystem, msg, @args);
  }

  /*
   * Sub-classes
   */

  //!
  //! An LR(0) item, a partially parsed rule.
  //!
  static class Item
  {
    //! The rule
    Rule r;

    //! How long into the rule the parsing has come.
    int offset;

    //! The state we will get if we shift according to this rule
    Kernel next_state;

    //! Item representing this one (used for shifts).
    Item master_item;

    //! Look-ahead set for this item.
    multiset(string) direct_lookahead = (<>);

    //! Look-ahead set used for detecting conflicts
    multiset(string) error_lookahead = (<>);

    //! Relation to other items (used when compiling).
    multiset(Item) relation = (<>);

    //! Depth counter (used when compiling).
    int counter;

    //! Item identification number (used when compiling).
    int number;

    //! Used to identify the item.
    //! Equal to r->number + offset.
    int item_id;

    static string _sprintf()
    {
      array(string) res = ({ symbol_to_string(r->nonterminal), ":\t" });

      if (offset) {
	foreach(r->symbols[0..offset-1], int|string symbol) {
	  res += ({ symbol_to_string(symbol), " " });
	}
      }
      res += ({ "· " });
      if (offset != sizeof(r->symbols)) {
	foreach(r->symbols[offset..], int|string symbol) {
	  res += ({ symbol_to_string(symbol), " " });
	}
      }
      if (sizeof(indices(direct_lookahead))) {
	res += ({ "\t{ ",
		  map(indices(direct_lookahead), symbol_to_string) * ", ",
		  " }" });
      }
      return res * "";
    }
  }

  //! Implements an LR(1) state
  static class Kernel {

    //! Used to check if a rule already has been added when doing closures.
    multiset(Rule) rules = (<>);

    //! Contains the items in this state.
    array(Item) items = ({});

    //! Used to lookup items given rule and offset
    mapping(int:Item) item_id_to_item = ([]);

    //! Contains the items whose next symbol is this non-terminal.
    mapping(int : multiset(Item)) symbol_items = ([]);

    //! The action table for this state
    //!
    //! @pre{
    //! object(kernel)    SHIFT to this state on this symbol.
    //! object(rule)      REDUCE according to this rule on this symbol.
    //! @}
    mapping(int|string : Kernel|Rule) action = ([]);

    //! The symbols that closure has been called on.
    multiset closure_set = (<>);

    /*
     * Functions
     */

    //! Add an item to the state.
    void add_item(Item i)
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
	foreach (grammar[nonterminal], Rule r) {
	  if (!rules[r]) {

	    Item new_item = Item();
	
	    new_item->r = r;
	    new_item->item_id = r->number;

	    // Not needed, since 0 is the default.
	    // new_item->offset = 0;
	    // rules[r] is set by the post-increment above.
	    rules[r] = 1;

	    add_item(new_item);

	    if (sizeof(r->symbols) && intp(r->symbols[0]) &&
		!closure_set[r->symbols[0]]) {
	      closure([int]r->symbols[0]);
	    }
	  }
	}
      } else {
	report(ERROR, "closure",
	       "Definition missing for non-terminal %s",
	       symbol_to_string(nonterminal));
	lr_error |= ERROR_MISSING_DEFINITION;
      }
    }

    //! Make the goto-set of this state.
    multiset(int|string) goto_set()
    {
      multiset(int|string) set = (<>);

      foreach (items, Item i) {
	if (i->offset != sizeof(i->r->symbols)) {
	  set[i->r->symbols[i->offset]] = 1;
	}
      }

      report(NOTICE, "goto_set", "=> (< %s >)",
	     map(indices(set), symbol_to_string) * ", ");
      return set;
    }

    //! Generates the state reached when doing goto on the specified symbol.
    //! i.e. it compiles the LR(0) state.
    //!
    //! @param symbol
    //!   Symbol to make goto on.
    Kernel do_goto(int|string symbol)
    {
      multiset(Item) items;

      report(NOTICE, "do_goto",
	     "Performing GOTO on <%s>",
	     symbol_to_string(symbol));

      items = symbol_items[symbol];
      if (items) {
	array(int) item_ids = [array(int)]map(sort(indices(items)->item_id),
					      `+, 1);
	string kernel_hash = sprintf("%@4c", item_ids);

	Kernel new_state = known_states[kernel_hash];

	if (!new_state) {
	  known_states[kernel_hash] = new_state = Kernel();
    
	  foreach (indices(items), Item i) {
	    int|string lookahead;

	    Item new_item = Item();
	    Rule r;
	    int offset = i->offset;

	    new_item->offset = ++offset;
	    new_item->r = r = i->r;
	    new_item->item_id = r->number + offset;

	    new_state->add_item(new_item);

	    if ((offset != sizeof(r->symbols)) &&
		intp(lookahead = r->symbols[offset]) &&
		!new_state->closure_set[lookahead]) {
	      new_state->closure([int]lookahead);
	    }
	  }

	  s_q->push(new_state);
	} else {
	  // report(NOTICE, "do_goto", "Known state");
	}
	/* DEBUG */

	report(NOTICE, "do_goto",
	       "GOTO on %s generated state:\n%s",
	       symbol_to_string(symbol),
	       state_to_string(new_state));

	/* !DEBUG */

	if (items) {
	  foreach (indices(items), Item i) {
	    i->next_state = new_state;
	  }
	}
      } else {
	report(WARNING, "do_goto",
	       "do_goto() on unknown symbol <%s>",
	       symbol_to_string(symbol));
      }
    }

    static string _sprintf()
    {
      return sprintf("%{%s\n%}", items);
    }
  }

  //! This is a queue, which keeps the elements even after they are retrieved.
  static class StateQueue {

    //! Index of the head of the queue.
    int head;

    //! Index of the tail of the queue.
    int tail;

    //! The queue itself.
    array(Kernel) arr = allocate(64);

    //! Pushes the state on the queue.
    //!
    //! @param state
    //!   State to push.
    Kernel push(Kernel state)
    {
      if (tail == sizeof(arr)) {
	arr += allocate(tail);
      }
      arr[tail++] = state;

      return state;
    }

    //! Return the next state from the queue.
    Kernel next()
    {
      if (head == tail) {
	return 0;
      } else {
	return arr[head++];
      }
    }
  }

  /*
   * Functions
   */

  /* Here are some help functions */

  /* Several cast to string functions */

  static string builtin_symbol_to_string(int|string symbol)
  {
    if (intp(symbol)) {
      return "nonterminal"+symbol;
    } else {
      return "\"" + symbol + "\"";
    }
  }

  static function(int|string : string) symbol_to_string = builtin_symbol_to_string;

  //! Pretty-prints a rule to a string.
  //!
  //! @param r
  //!   Rule to print.
  string rule_to_string(Rule r)
  {
    string res = symbol_to_string(r->nonterminal) + ":\t";

    if (sizeof(r->symbols)) {
      foreach (r->symbols, int|string symbol) {
	res += symbol_to_string(symbol) + " ";
      }
    } else {
      res += "/* empty */";
    }
    return res;
  }

  //! Pretty-prints an item to a string.
  //!
  //! @param i
  //!   Item to pretty-print.
  string item_to_string(Item i)
  {
    return sprintf("%s", i);
  }

  //! Pretty-prints a state to a string.
  //!
  //! @param state
  //!   State to pretty-print.
  string state_to_string(Kernel state)
  {
    return sprintf("%s", state);
  }

  //! Pretty-prints the current grammar to a string.
  static string _sprintf()
  {
    array(string) res = ({});

    foreach (indices(grammar), int nonterminal) {
      res += ({ symbol_to_string(nonterminal) });
      foreach (grammar[nonterminal], Rule r) {
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
    return res * "";
  }

  string cast_to_string()
  {
    return _sprintf();
  }

  //! Implements casting.
  //!
  //! @param type
  //!   Type to cast to.
  mixed cast(string type)
  {
    if (type == "string")
      return _sprintf();
    error("Cast to %s not supported\n", type);
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
    Priority pri;

    if (pri = operator_priority[terminal]) {
      pri->value = pri_val;
    } else {
      operator_priority[terminal] = Priority(pri_val, 0);
    }
  }

  //! Sets the associativity of a terminal.
  //!
  //! @param terminal
  //!   Terminal to set the associativity for.
  //! @param assoc
  //!   Associativity; negative - left, positive - right,
  //!   zero - no associativity.
  void set_associativity(string terminal, int assoc)
  {
    Priority pri;

    if (pri = operator_priority[terminal]) {
      pri->assoc = assoc;
    } else {
      operator_priority[terminal] = Priority(0, assoc);
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
    symbol_to_string = s_to_s || builtin_symbol_to_string;
  }

  //! Sets the error report function.
  //!
  //! @param handler
  //!   Function to call to report errors and warnings.
  //!   If zero or not specifier, use the built-in function.
  void set_error_handler(void|function(SeverityLevel, string, string, mixed ...: void) handler)
  {
    error_handler = handler || ErrorHandler()->report;
  }

  //! Add a rule to the grammar.
  //!
  //! @param r
  //! Rule to add.
  void add_rule(Rule r)
  {
    array(Rule) rules;
    int|string symbol;

    /* DEBUG */
    report(NOTICE, "add_rule", "Adding rule: %s", rule_to_string(r));

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
      ADT.Stack new_nullables = ADT.Stack(1024);

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
	  symbol = [int]new_nullables->pop();
	  report(NOTICE, "add_rule", "Nulling symbol %s",
		 symbol_to_string(symbol));
	  nullable[symbol] = 1;
	  if (used_by[symbol]) {
	    foreach (indices(used_by[symbol]), Rule r2) {
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
	  foreach (grammar[symbol], Rule r2) {
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
      foreach (indices(begins[r->nonterminal]), Rule r2) {
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

  static Kernel first_state()
  {
    Kernel state = Kernel();

    array(int) first_state_item_ids = sort(grammar[0]->number);
    string kernel_hash = sprintf("%@4c", first_state_item_ids);
    known_states[kernel_hash] = state;

    foreach (grammar[0], Rule r) {
      if (!state->rules[r]) {
	Item i = Item();

	i->r = r;
	// Not needed since 0 is the default.
	// i->offset = 0;
	i->item_id = r->number;

	state->add_item(i);
	state->rules[r] = 1;	/* Since this is an item with offset 0 */

	if ((sizeof(r->symbols)) &&
	    (intp(r->symbols[0]))) {
	  state->closure([int]r->symbols[0]);
	}
      }
    }
    return state;
  }

  //! Contains all states used.
  //! In the queue section are the states that remain to be compiled.
  StateQueue s_q;

  static ADT.Stack item_stack;

  static void traverse_items(Item i,
			     function(int:void) conflict_func)
  {
    int depth;

    item_stack->push(i);

    i->counter = depth = item_stack->ptr;

    foreach (indices(i->relation), Item i2) {
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
      Item i2;

      while ((i2 = [object(Item)]item_stack->pop()) != i) {

	i2->number = 0x7fffffff;

	i2->direct_lookahead = i->direct_lookahead;

	cyclic = 1;
	empty_cycle &= !(sizeof(i2->error_lookahead));
      }
      i->count = 0x7fffffff;

      if (cyclic) {
	report(NOTICE, "traverse_items", "Cyclic item\n%s",
	       item_to_string(i));
	conflict_func(empty_cycle && !(sizeof(i->error_lookahead)));
      }
    }
  }

  static void shift_conflict(int empty)
  {
    /* Ignored */
  }

  static void handle_shift_conflicts()
  {
    item_stack = ADT.Stack(131072);

    /* Initialize the counter */
    for (int index = 0; index < s_q->tail; index++) {
      foreach (s_q->arr[index]->items, Item i) {
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
      foreach (s_q->arr[index]->items, Item i) {
	if (!i->number) {
	  traverse_items(i, shift_conflict);
	}
      }
    }
  }

  static void follow_conflict(int empty)
  {
  }

  static void handle_follow_conflicts()
  {
    item_stack = ADT.Stack(131072);

    /* Initialize the counter */
    for (int index = 0; index < s_q->tail; index++) {
      foreach (s_q->arr[index]->items, Item i) {
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
      foreach (s_q->arr[index]->items, Item i) {
	if (!i->number) {
	  traverse_items(i, follow_conflict);
	}
      }
    }
  }

  static int go_through(Kernel state, int item_id,
			Item current_item)
  {
    int index;
    Item i, master;

    i = state->item_id_to_item[item_id];

    /* What to do if not found? */
    if (!i) {
      report(ERROR, "go_through",
	     "Item %d not found in state\n"
	     "%s\n"
	     "Backtrace:\n%s",
	     item_id,
	     state_to_string(state),
	     describe_backtrace(backtrace()));
      return 0;
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
	return nullable[i->r->symbols[i->offset]];
      } else
	return 0;	/* Not nullable */
    } else {
      /* At end of rule */
      master->relation[current_item] = 1;
      return 1;		/* Always nullable */
    }
  }

  static int repair(Kernel state, multiset(int|string) conflicts)
  {
    multiset(int|string) conflict_set = (<>);

    report(NOTICE, "repair",
	   "Repairing conflict in state:\n%s\n"
	   "Conflicts on (< %s >)",
	   state_to_string(state),
	   map(indices(conflicts), symbol_to_string) * ", ");
  
    foreach (indices(conflicts), int|string symbol) {
      int reduce_count = 0;
      int shift_count = 0;
      int only_operators = 1;
      Priority shift_pri, reduce_pri, pri;
      Rule min_rule = 0;

      /* Analyse the items */
      /* This loses if there are reduce-reduce conflicts,
       * or shift-shift conflicts
       */
      foreach (state->items, Item i) {
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

      int reduce_rest = 0;
      int shift_rest = 0;

      if (only_operators) {
	if (reduce_pri->value > shift_pri->value) {
	  pri = reduce_pri;
	} else {
	  pri = shift_pri;
	}

	foreach (state->items, Item i) {
	  if (i->offset == sizeof(i->r->symbols)) {
	    /* Reduce */
	    if (i->direct_lookahead[symbol]) {
	      Priority new_pri;
	      if ((new_pri = i->r->pri)->value < pri->value) {
		report(NOTICE, "repair",
		       "Ignoring reduction of item\n%s\n"
		       "on lookahead %s (Priority %d < %d)",
		       item_to_string(i),
		       symbol_to_string(symbol),
		       new_pri->value, pri->value);
		i->direct_lookahead[symbol] = 0;
		if (!sizeof(indices(i->direct_lookahead))) {
		  i->direct_lookahead = (<>);
		}
	      } else if ((pri->assoc >= 0) &&
			 (shift_pri->value == pri->value)) {
		report(NOTICE, "repair",
		       "Ignoring reduction of item\n%s\n"
		       "on lookahead %s (Right associative)",
		       item_to_string(i),
		       symbol_to_string(symbol));
		i->direct_lookahead[symbol] = 0;
		if (!sizeof(indices(i->direct_lookahead))) {
		  i->direct_lookahead = (<>);
		}
	      } else {
		report(NOTICE, "repair",
		       "Kept item\n%s\n"
		       "on lookahead %s",
		       item_to_string(i),
		       symbol_to_string(symbol));
		reduce_rest++;
	      }
	    }
	  } else if (i->r->symbols[i->offset] == symbol) {
	    /* Shift */
	    if (shift_pri->value < pri->value) {
	      report(NOTICE, "repair",
		     "Ignoring shift on item\n%s\n"
		     "on lookahead %s (Priority %d < %d)",
		     item_to_string(i),
		     symbol_to_string(symbol),
		     i->r->pri->value, pri->value);
	      i->direct_lookahead = (<>);
	      i->next_state = 0;
	    } else if ((pri->assoc <= 0) &&
		       (reduce_pri->value == pri->value)) {
	      report(NOTICE, "repair",
		     "Ignoring shift on item\n%s\n"
		     "on lookahead %s (Left associative)",
		     item_to_string(i),
		     symbol_to_string(symbol));
	      i->direct_lookahead = (<>);
	      i->next_state = 0;
	    } else {
	      report(NOTICE, "repair",
		     "Kept item\n%s\n"
		     "on lookahead %s",
		     item_to_string(i),
		     symbol_to_string(symbol));
	      shift_rest++;
	    }
	  }
	}
      } else {
	/* Not only operators */
	if (shift_count) {
	  /* Prefer shifts */
	  foreach (state->items, Item i) {
	    if (i->offset == sizeof(i->r->symbols)) {
	      /* Reduction */
	      if (i->direct_lookahead[symbol]) {
		report(NOTICE, "repair",
		       "Ignoring reduction on item\n%s\n"
		       "on lookahead %s (can shift)",
		       item_to_string(i),
		       symbol_to_string(symbol));
		i->direct_lookahead[symbol] = 0;
		if (!sizeof(indices(i->direct_lookahead))) {
		  i->direct_lookahead = (<>);
		}
	      }
	    } else {
	      /* Shift */
	      if (i->r->symbols[i->offset] == symbol) {
		report(NOTICE, "repair",
		       "Kept item\n%s\n"
		       "on lookahead (shift)%s",
		       item_to_string(i),
		       symbol_to_string(symbol));
		shift_rest++;
	      }
	    }
	  }
	} else {
	  /* Select the first reduction */
	  foreach (state->items, Item i) {
	    if (i->r == min_rule) {
	      report(NOTICE, "repair",
		     "Kept item\n%s\n"
		     "on lookahead %s (first rule)",
		     item_to_string(i),
		     symbol_to_string(symbol));
	      reduce_rest++;
	    } else {
	      report(NOTICE, "repair",
		     "Ignoring reduction on item\n%s\n"
		     "on lookahead %s (not first rule)",
		     item_to_string(i),
		     symbol_to_string(symbol));
	      i->direct_lookahead[symbol] = 0;
	      if (!sizeof(indices(i->direct_lookahead))) {
		i->direct_lookahead = (<>);
	      }
	    }
	  }
	}
      }

      int conflict_free = 0;

      if (reduce_rest > 1) {
	if (shift_rest) {
	  report(ERROR, "repair",
		 "Shift-Reduce-Reduce conflict on lookahead %s",
		 symbol_to_string(symbol));
	} else {
	  report(ERROR, "repair",
		 "Reduce-Reduce conflict on lookahead %s",
		 symbol_to_string(symbol));
	}
      } else if (reduce_rest) {
	if (shift_rest) {
	  report(ERROR, "repair",
		 "Shift-Reduce conflict on lookahead %s",
		 symbol_to_string(symbol));
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
	    report(only_operators?NOTICE:WARNING, "repair",
		   "Repaired Shift-Reduce-Reduce conflict on %s",
		   symbol_to_string(symbol));
	  } else {
	    report(only_operators?NOTICE:WARNING, "repair",
		   "Repaired Reduce-Reduce conflict on %s",
		   symbol_to_string(symbol));
	  }
	} else if (reduce_count) {
	  if (shift_count) {
	    report(only_operators?NOTICE:WARNING, "repair",
		   "Repaired Shift-Reduce conflict on %s",
		   symbol_to_string(symbol));
	  } else {
	    /* No conflict */
	    report(NOTICE, "repair",
		   "No conflict on symbol %s (Plain REDUCE)",
		   symbol_to_string(symbol));
	  }
	} else {
	  /* No conflict */
	  report(NOTICE, "repair",
		 "No conflict on symbol %s (SHIFT)",
		 symbol_to_string(symbol));
	}

      } else {
	/* Still conflicts left on this symbol */
	conflict_set[symbol] = 1;
      }
    }
  
    if (sizeof(indices(conflict_set))) {
      report(ERROR, "repair",
	     "Still conflicts remaining in state\n%s\n"
	     "on symbols (< %s >)",
	     state_to_string(state),
	     map(indices(conflict_set), symbol_to_string) * ", ");
      return ERROR_CONFLICTS;
    } else {
      report(WARNING, "repair",
	     "All conflicts removed!");
      return 0;
    }
  }

#ifdef LR_PROFILE
#define LR_GAUGE(X, BLOCK)	\
	report(NOTICE, "compile", X ": %f\n", gauge BLOCK)
#else /* !LR_PROFILE */
#define LR_GAUGE(X, BLOCK)	do BLOCK while(0)
#endif /* LR_PROFILE */

  //! Compiles the grammar into a parser, so that parse() can be called.
  int compile()
  {
    int lr_error = 0;	/* No error yet */
    int state_no = 0;	/* DEBUG INFO */
    Kernel state;
    multiset(int|string) symbols, conflicts;

    s_q = StateQueue();
    s_q->push(first_state());

    /* First make LR(0) states */

    LR_GAUGE("LR0", {
      while (state = s_q->next()) {

	report(NOTICE, "compile", "Compiling state %d:\n%s", state_no++,
	       state_to_string(state) + "\n");

	/* Probably better implemented as a stack */
	foreach (indices(state->goto_set()), int|string symbol) {
	  state->do_goto(symbol);
	}
      }
    });

    /* Compute nullables */
    /* Done during add_rule */
    report(NOTICE, "compile", "Nullable nonterminals: (< %s >)\n",
	   map(indices(nullable), symbol_to_string) * ", ");

    LR_GAUGE("Master items", {
      /* Mark Transition and Reduction master items */
      for (int index = 0; index < s_q->tail; index++) {
	mapping(int|string : Item) master_item =([]);

	foreach (s_q->arr[index]->items, Item i) {
	  if (i->offset < sizeof(i->r->symbols)) {
	    /* This is not a reduction item, which represent themselves */
	    int|string symbol = i->r->symbols[i->offset];

	    if (!(i->master_item = master_item[symbol])) {
	      master_item[symbol] = i;
	    }
	  }
	}
      }
    });

    /* Probably OK so far */

    LR_GAUGE("LA sets", {
      /* Calculate look-ahead sets (DR and relation) */
      for (int index = 0; index < s_q->tail; index++) {
	foreach (s_q->arr[index]->items, Item i) {
	  if ((!i->master_item) && (i->offset != sizeof(i->r->symbols)) &&
	      (intp(i->r->symbols[i->offset]))) {
	    /* This is a non-terminal master item */
	    foreach (i->next_state->items, Item i2) {
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
    });

    LR_GAUGE("Handle shift", {
      /* Handle SHIFT-conflicts */
      handle_shift_conflicts();
    });

    LR_GAUGE("Check shift", {
      /* Check the shift sets */
      /* (Is this needed?)
       * Yes - initializes error_lookahead
       */
      for (int index = 0; index < s_q->tail; index++) {
	foreach (s_q->arr[index]->items, Item i) {
	  if ((!i->master_item) &&
	      (i->offset != sizeof(i->r->symbols)) &&
	      (intp(i->r->symbols[i->offset]))) {
	    i->error_lookahead = copy_value(i->direct_lookahead);
	  }
	}
      }
    });

    LR_GAUGE("Lookback sets", {
      /* Compute lookback-sets */
      for (int index = 0; index < s_q->tail; index++) {
	array(Item) items =  s_q->arr[index]->items;
	// Set up a lookup table to speedup lookups later.
	mapping(int:array(Item)) lookup = ([]);
	foreach (items, Item i) {
	  if (!i->offset) {
	    if (!lookup[i->r->nonterminal]) {
	      lookup[i->r->nonterminal] = ({ i });
	    } else {
	      lookup[i->r->nonterminal] += ({ i });
	    }
	  }
	}
	foreach (items, Item transition) {
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
	      report(WARNING, "compile",
		     "No item for symbol <%s>\n"
		     "in state:\n"
		     "%s",
		     symbol_to_string(symbol),
		     state_to_string(s_q->arr[index]));
	      continue;
	    }

	    /* Find items which can reduce to the nonterminal from above */
	    foreach (lookup[symbol], Item i) {
	      if (sizeof(i->r->symbols)) {
		if (go_through(i->next_state, i->item_id + 1, transition)) {
		  /* Nullable */
		  Item master = i;
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
    });

    LR_GAUGE("Handle follow", {
      /* Handle follow-conflicts */
      handle_follow_conflicts();
    });

    LR_GAUGE("Compute LA", {
      /* Compute the lookahead (LA) */
      for (int index = 0; index < s_q->tail; index++) {
	foreach (s_q->arr[index]->items, Item i) {
	  if (i->offset == sizeof(i->r->symbols)) {
	    /* Reduction item (always a master item) */
	  
	    /* Calculate Look-ahead for all items in look-back set */
	
	    i->direct_lookahead=`|(i->direct_lookahead,
				   @indices(i->relation)->direct_lookahead);
	  }
	}
      }
    });

    /* Probably OK from this point onward */

    LR_GAUGE("Check conflicts", {
      /* Check for conflicts */
      for (int index = 0; index < s_q->tail; index++) {
	Kernel state = s_q->arr[index];
    
	conflicts = (<>);
	symbols = (<>);

	foreach (state->items, Item i) {
	  if (i->offset == sizeof(i->r->symbols)) {
	    /* Reduction */
	    conflicts |= i->direct_lookahead & symbols;
	    symbols |= i->direct_lookahead;
	  } else if (!i->master_item) {
	    string|int symbol;

	    /* Only master items, since we get
	     * Shift-Shift conflicts otherwise
	     */

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
	  lr_error = repair(state, conflicts);
	  // verbose = ov;
	} else {
	  report(NOTICE, "compile", "No conflicts in state:\n%s",
		 state_to_string(s_q->arr[index]));
	}
      }
    });

    LR_GAUGE("Compile actions", {
      /* Compile action tables */
      for (int index = 0; index < s_q->tail; index++) {
	Kernel state = s_q->arr[index];

	state->action = ([]);

	foreach (state->items, Item i) {
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
    });

#ifdef LR_PROFILE
    report(NOTICE, "compile", "DONE\n");
#endif /* LR_PROFILE */

    return lr_error;
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
    ADT.Stack value_stack = ADT.Stack(4096);
    ADT.Stack state_stack = ADT.Stack(4096);
    Kernel state = start_state;

    string input;
    mixed value;

    lr_error = 0;	/* No parse error yet */

    if (!functionp(scanner) &&
	!(objectp(scanner) && functionp(scanner->`()))) {
      report(ERROR, "parse", "parser->parse(): scanner not set!\n");
      lr_error = ERROR_NO_SCANNER;
      return 0;
    }

    while (1) {
      mixed a;

      /* Read some input */
      value = scanner();

      if (arrayp(value)) {
	input = ([array(string)]value)[0];
	value = ([array(mixed)]value)[1];
      } else {
	input = [string]value;
      }

      while(1) {
	while (object_program(a = state->action[input]) == Rule) {
	  Rule r = [object(Rule)]a;

	  report(NOTICE, "parse", "Reducing according to rule\n%s\n",
		 rule_to_string(r));

	  do {
	    if (r->action) {
	      /* REDUCE */
	      string|function func = 0;

	      if (stringp(func = r->action)) {
		if (action_object) {
		  func = [string|function]action_object[r->action];
		  if (!functionp(func)) {
		    if (!func) {
		      report(ERROR, "parse",
			     "Missing action \"%s\" in object",
			     r->action);
		      lr_error |= ERROR_MISSING_ACTION;
		    } else {
		      report(ERROR, "parse",
			     "Bad type (%s) for action \"%s\" in object",
			     typeof(func), r->action);
		      lr_error |= ERROR_BAD_ACTION_TYPE;
		      func = 0;
		    }
		  }
		} else {
		  report(ERROR, "parse", "Missing object for action \"%s\"",
			 r->action);
		  lr_error |= ERROR_NO_OBJECT;
		  func = 0;
		}
	      }
	      if (func) {
		if (sizeof(r->symbols)) {
		  value_stack->push(([function(mixed ...:mixed)]func)
				    (@[array(mixed)]value_stack->
				     pop(sizeof(r->symbols))));
		  state = ([array(Kernel)]state_stack->pop(sizeof(r->symbols)))[0];
		} else {
		  value_stack->push(r->action());
		}
		break;	// Break out of the do-while.
	      }
	    }
	    // Default action.
	    if (sizeof(r->symbols)) {
	      if (sizeof(r->symbols) > 1) {
		value_stack->quick_pop(sizeof(r->symbols) - 1);
	      }
	      state = ([array(Kernel)]state_stack->pop(sizeof(r->symbols)))[0];
	    } else {
	      value_stack->push(0);
	    }
	  } while(0);

	  state_stack->push(state);
	  state = [object(Kernel)]state->action[r->nonterminal]; /* Goto */
	}

	if (a) {
	  /* SHIFT or ACCEPT */
	  if (input == "") {
	    /* Only the final state is allowed to shift on ""(EOF) */
	    /* ACCEPT */
	    return value_stack->pop();
	  }
	  /* SHIFT */
	  report(NOTICE, "parse",
		 "Shifting \"%s\", value \"%O\"", input, value);
	  value_stack->push(value);
	  state_stack->push(state);
	  state = [object(Kernel)]a;
	} else {
	  /* ERROR */
	  if (input = "") {
	    /* At end of file */
	    lr_error |= ERROR_EOF;

	    if (value_stack->ptr != 1) {
	      if (value_stack->ptr) {
		report(ERROR, "parse", "Bad state at EOF -- Throwing \"%O\"",
		       value_stack->pop());
		state = [object(Kernel)]state_stack->pop();
		continue;
	      } else {
		report(ERROR, "parse", "Empty stack at EOF!");
		return 0;
	      }
	    } else {
	      report(ERROR, "parse", "Bad state at EOF");
	      return value_stack->pop();
	    }
	  } else {
	    lr_error |= ERROR_SYNTAX;

	    report(ERROR, "parse", "Bad input: %O(%O)", input, value);
	  }
	}
	break;	/* Break out of the inner while(1) to read more input. */
      }
    }
  }
}
