//! PLIS, Permuted Lisp. A Lisp language somewhat similar to scheme.

#pike __REAL_VERSION__

#ifdef LISP_DEBUG
#define WERROR werror
#else
#define WERROR(x)
#endif

/* Data shared between all Lisp objects */
mapping symbol_table = ([ ]);

Nil     Lempty = Nil();
Boolean Lfalse = Boolean("#f");
Boolean Ltrue = Boolean("#t");

object readline;

/* Lisp types */

class LObject 
{
}

class SelfEvaluating (string name)
{
  inherit LObject;

  SelfEvaluating eval(Environment env, Environment globals)
    {
      return this_object();
    }

  string print(int display) { return name; }
}

class Boolean
{
  inherit SelfEvaluating;
}

class Cons 
{
  inherit LObject;

  object car;
  object cdr;

  void create(object a, object d)
    {
      if (!a)
	error("Cons: car is null!\n");
      if (!d)
	error("Cons: cdr is null!\n");
      car = a; cdr = d;
    }

  object mapcar(string|function fun, mixed ...extra)
    {
      object new_car = stringp(fun)? car[fun](@extra) : fun(car, @extra);
      if (!new_car)
      {
	werror("No car\n");
	return 0;
      }

      object new_cdr = (cdr != Lempty) ? cdr->mapcar(fun, @extra)
	: cdr;
      if (new_cdr) 
	return object_program(this_object())(new_car, new_cdr);
      else
      {
	werror("No cdr\n");
	return 0;
      }
    }

  object map(string|function fun, mixed ...extra)
    {
      /* Do this as a special case to allow tail recursion */
      if (!cdr || (cdr == Lempty))
      {
	if (stringp(fun))
	  return car[fun](@extra);
	else
	  return fun(car, @extra);
      }
      if (stringp(fun) ? car[fun](@extra) : fun(car, @extra))
	return cdr->map(fun, @extra);
      else
      {
	werror("Function ["+(stringp(fun)?fun:sprintf("%O", fun))+
	       "]\nin "+car->print(1)+" returned error\n");
	return 0;
      }
    }

  string print(int display)
    {
      string s = "(";
      object p = this_object();
      while (p != Lempty)
      {
	if (!p->car)
	{ /* Not a cons cell */
	  s += " . " + p->print(display);
	  break;
	}
	s += " " + p->car->print(display);
	p = p->cdr;
      }
      s += " )";
      return s;
    }

  object eval(Environment env, Environment globals)
    {
      WERROR(sprintf("eval list: %s\n", print(1)));
      //if (car->name == "read-line")
      //  trace(1);
      object fun = car->eval(env, globals);
      if (fun)
      {
	if (fun->is_special)
	  return fun->apply(cdr, env, globals);
	
	object args = cdr->mapcar("eval", env, globals);
	if (args)
	  return fun->apply(args, env, globals);
	werror("No args\n");
	return 0;
      }
      werror("No function to eval\n");
      return 0;
    }
}

/* Used for "user-level" cons cells */
class MutableCons
{
  inherit Cons;
  constant is_mutable_cons = 1;
}

class Symbol 
{
  inherit LObject;

  string name;

  object eval(Environment env, Environment globals)
    {
      WERROR(sprintf("eval symbol '%s'\n", name));
#if 0
      if(globals->eval_limit)
      {
	globals->eval_limit--;
	if(globals->eval_limit==0)
	{
	  globals->eval_limit=1;  
	  error("Maximum eval-depth reached.");
	}
      }
#endif
      object binding =  env->query_binding(this_object())
	|| globals->query_binding(this_object());
      if (!binding)
      {
	werror("No binding for this symbol ["+name+"].\n");
	return 0;
      }
      return binding->query();
    }
  
  //  int __hash() { return hash(name); }

  string print(int display)
    {
      return name;
    }

  void create(string n, mapping|void table)
    {
      //     werror(sprintf("Creating symbol '%s'\n", n));
      name = n;
      if (table)
	table[name] = this_object();
    }

  string to_string() { return name; }

}

class ConstantSymbol 
{
  inherit Symbol : symbol;
  inherit SelfEvaluating;
}

class Nil 
{
  inherit Cons;
  inherit SelfEvaluating;

  // constant is_nil = 1;

  void create()
    {
      Cons :: create(this_object(), this_object());
      SelfEvaluating :: create("()");
    }

  object mapcar(mixed ...ignored) { return this_object(); }
  object map(mixed ...ignored) { return this_object(); }
}

class String (string value)
{
  inherit SelfEvaluating;

  constant is_string = 1;

  string print(int display)
    {
      return display ? ("\"" + replace(value, ({ "\\", "\"", "\n",}),
				       ({ "\\\\", "\\\"", "\\n"}) ) + "\"")
	: value;
    }

  string to_string() { return value; }
}

class Number (int|float|object value)
{
  inherit SelfEvaluating;

  constant is_number = 1;

  string print(int display) { return (string) value; }
}
  
class Binding (object value)
{
  object query() { return value; }
  void set(object v) { value = v; }
}
  
class Environment 
{
  inherit LObject;
  // int eval_limit; // ugly hack..

  /* Mapping of symbols and their values.
   * As a binding may exist in several environments, they
   * are accessed indirectly. */
  mapping(Symbol:object) env = ([ ]);

  object query_binding(Symbol symbol)
    {
      return env[symbol];
    }

  void create(mapping|void bindings)
    {
      env = bindings || ([ ]);
    }

  object copy() { return object_program(this_object())(copy_value(env)); };

  void extend(Symbol symbol, object value)
    {
      //     werror(sprintf("Binding '%s'\n", symbol->print(1)));
      env[symbol] = Binding(value);
    }

  string print(int display) 
    {
      string res="";
      // werror("PLIS.Environment->print\n");
      foreach(Array.sort_array(indices(env),
			       lambda(object a, object b)
			       { return a->to_string && b->to_string
				   && (a->to_string() > b->to_string());
			       } ),
	      object s)
      {
	if(env[s]->value != this_object())
	  res += s->print(display)+": "+env[s]->value->print(display)+"\n";
	else
	  res += "global-environment: ...\n";
      }
      return res;
    }
}
  
class Lambda (object formals, // May be a dotted list
	      object list     // expressions
	      )
{
  inherit LObject;

  string print(int display) { return "lambda "+list->print(display); }

  int build_env1(Environment env, object symbols, object arglist)
    {
      if (symbols == Lempty)
	return arglist == Lempty;
      if (!symbols->car)
      {
	/* An atom */
	env->extend(symbols, arglist);
	return 1;
      } else {
	return build_env1(env, symbols->car, arglist->car)
	  && build_env1(env, symbols->cdr, arglist->cdr);
      }
    }

  Environment build_env(Environment env, object arglist)
    {
      Environment res = env->copy();
      return build_env1(res, formals, arglist) ? res : 0;
    }

  object new_env(Environment env, object arglist);

  object apply(object arglist, Environment env, Environment globals)
    {
      if (globals->limit_apply && (globals->limit_apply()))
	return 0;
      env = new_env(env, arglist); 
      if (env)
	return list->map("eval", env, globals);
      werror("Nothing to apply with.\n");
      return 0;
    }
}
  
class Lexical 
{
  inherit Lambda : l;
  object env;

  void create(object e, object formals_list, object expressions)
    {
      env = e;
      //    werror(sprintf("Building lexical closure, env = %s\n",
      //		   env->print(1)));
      l :: create(formals_list, expressions);
    }

  object new_env(object ignored, object arglist)
    {
      return build_env(env, arglist);
    }
}

class Macro 
{
  inherit Lexical;
  constant is_special = 1;
  object apply(object arglist, Environment env, Environment globals)
    {
      object expansion = ::apply(arglist, env, globals);
      return expansion && expansion->eval(env, globals);
    }
}

class Dynamic 
{
  inherit Lambda;
  object new_env(Environment env, object arglist)
    {
      return build_env(env, arglist);
    }
}

class Builtin (function apply)
{
  inherit LObject;

  string print(int display)
    {
      return sprintf("Builtin (%O)", apply);
    }
}  

class Special 
{
  inherit Builtin;
  constant is_special = 1;
  string print(int display)
    {
      return sprintf("Special (%O)", apply);
    }
}


/* Misc functions */
Symbol make_symbol(string name)
{
  return symbol_table[name] || Symbol(name, symbol_table);
}

Cons make_list(object ...args)
{
  Cons res = Lempty;
  for (int i = sizeof(args) - 1; i >= 0; i--)
    res = Cons(args[i], res);
  return res;
}


/* Parser */

class Parser (string buffer)
{
  object number_re = Regexp("^(-|)([0-9]+)");
  object symbol_re = Regexp("^([^0-9 \t\n(.)\"#]+)");
  object space_re = Regexp("^([ \t\n]+)");
  object comment_re = Regexp("^(;[^\n]*\n)");
  object string_re = Regexp("^(\"([^\\\\\"]|\\\\.)*\")");

  mixed _read()
    {
      if (!sizeof(buffer))
      {
	return 0;
      }
      array a;
      if (a = space_re->split(buffer) || comment_re->split(buffer))
      {
	//	werror(sprintf("Ignoring space and comments: '%s'\n", a[0]));
	buffer = buffer[sizeof(a[0])..];
	return _read();
      }
      if (a = number_re->split(buffer))
      {
	//	werror("Scanning number\n");
	string s = `+(@ a);
	buffer = buffer[ sizeof(s) ..];
	return Number( (int)s );
      }
      if (a = symbol_re->split(buffer))
      {
	// 	werror("Scanning symbol\n");
	buffer = buffer[sizeof(a[0])..];
	return make_symbol(a[0]);
      }
      if (a = string_re->split(buffer))
      {
	//	werror("Scanning string\n");
	buffer = buffer[sizeof(a[0])..];
	return String(replace(a[0][1 .. sizeof(a[0]) - 2],
			      ({ "\\\\", "\\\"", "\\n" }),
			      ({ "\\", "\"", "\n"}) ) );
      }

      switch(int c = buffer[0])
      {
      case '(':
	// 	werror("Reading (\n");
	buffer = buffer[1 ..];
	return read_list();
      case '.':
      case ')':
	// 	werror(sprintf("Reading %c\n", c));
	buffer = buffer[1..];
	return c;
      case '#':
	if (sizeof(buffer) < 2)
	  return 0;
	c = buffer[1];
	buffer = buffer[2..];
	switch(c)
	{
	case 't':
	  return Ltrue;
	case 'f':
	  return Lfalse;
	default:
	  return 0;
	}
      default:
	werror("Parse error while reading.");
	return 0;
      }
    }

  object read()
    {
      mixed res = _read();
      if (intp(res))
      {
	return 0;
      }
      return res;
    }

  object read_list()
    {
      mixed item = _read();
      if (!item)
      {
	return 0;
      }
      if (intp(item))
	switch(item)
	{
	case ')': return Lempty;
	case '.':
	  object|int fin = _read();
	  if (intp(fin) || (_read() != ')'))
	  {
	    return 0;
	  }
	  return fin;
	default:
	  error( "lisp->parser: internal error\n" );
	}
      object rest = read_list();
      return rest && Cons(item , rest);
    }
}


/* Lisp special forms */

object s_quote(object arglist, Environment env, Environment globals)
{
  return arglist->car;
}

object s_setq(object arglist, Environment env, Environment globals)
{
//  werror(sprintf("set!, arglist: %s\n", arglist->print(1) + "\n"));
  object value = arglist->cdr->car->eval(env, globals);
  object binding = env->query_binding(arglist->car)
    || globals->query_binding(arglist->car);
  if (binding)
    {
      binding->set(value);
      return value;
    }
  else
    return 0;
}

object s_define(object arglist, Environment env, Environment globals)
{
  object symbol, value;
  if (arglist->car->car)
  { /* Function definition */
    // werror(sprintf("define '%s'\n", arglist->car->car->print(0)));
    symbol = arglist->car->car;
    value = Lexical(env, arglist->car->cdr, arglist->cdr);
  } else {
    symbol = arglist->car;
    value = arglist->cdr->car->eval(env, globals);
  }
  if (!value)
    return 0;
  env->extend(symbol, value);
  return symbol;
}    

object s_defmacro(object arglist, Environment env, Environment globals)
{
  // werror(sprintf("defmacro '%s'\n", arglist->car->car->print(0)));
  object symbol = arglist->car->car;
  object value = Macro(env, arglist->car->cdr, arglist->cdr);
  if (!value)
    return 0;
  env->extend(symbol, value);
  return symbol;
}

object s_if(object arglist, Environment env, Environment globals)
{
  if (arglist->car->eval(env, globals) != Lfalse)
    return arglist->cdr->car->eval(env, globals);
  arglist = arglist->cdr->cdr;
  return (arglist != Lempty) ? arglist->car->eval(env, globals) : Lfalse;
}

object s_and(object arglist, Environment env, Environment globals)
{
  if (arglist == Lempty)
    return Ltrue;

  while(arglist->cdr != Lempty)
  {
    object res = arglist->car->eval(env, globals);
    if (!res || (res == Lfalse) )
      return res;
    arglist = arglist->cdr;
  }
  return arglist->car->eval(env, globals);
}

object s_or(object arglist, Environment env, Environment globals)
{
  if (arglist == Lempty)
    return Lfalse;

  while(arglist->cdr != Lempty)
  {
    object res = arglist->car->eval(env, globals);
    if (!res || (res != Lfalse) )
      return res;
    arglist = arglist->cdr;
  }
  return arglist->car->eval(env, globals);
}

object s_begin(object arglist, Environment env, Environment globals)
{
  return arglist->map("eval", env, globals);
}

object s_lambda(object arglist, Environment env, Environment globals)
{
  return Lexical(env, arglist->car, arglist->cdr);
}

/* In general, errors are signaled by returning 0, and are
 * fatal.
 *
 * The catch special form catches errors, returning Lfalse
 * if an error occured. */
object s_catch(object arglist, Environment env, Environment globals)
{
  return s_begin(arglist, env, globals) || Lfalse;
}

object s_while(object arglist, Environment env, Environment globals)
{
  object expr = arglist->car, res;
  object to_eval = arglist->cdr;
  werror( to_eval->print(1) );
  
  while ( (res = expr->eval(env,globals)) != Lfalse)
    to_eval->map("eval", env, globals);//f_eval (to_eval,env,globals);
  return res;
}


/* Functions */

object f_car(object arglist, Environment env, Environment globals)
{
  return arglist->car->car;
}

object f_cdr(object arglist, Environment env, Environment globals)
{
  return arglist->car->cdr;
}

object f_null(object arglist, Environment env, Environment globals)
{
  return (arglist->car == Lempty) ? Ltrue : Lfalse;
}

object f_cons(object arglist, Environment env, Environment globals)
{
  return MutableCons(arglist->car, arglist->cdr->car);
}

object f_list(object arglist, Environment env, Environment globals)
{
  return arglist;
}

object f_setcar(object arglist, Environment env, Environment globals)
{
  if (arglist->car->is_mutable_cons)
    return arglist->car->car = arglist->cdr->car;
  return 0;
}

object f_setcdr(object arglist, Environment env, Environment globals)
{
  if (arglist->car->is_mutable_cons)
    return arglist->car->cdr = arglist->cdr->car;
  return 0;
}

object f_eval(object arglist, Environment env, Environment globals)
{
  if (arglist->cdr != Lempty)
  {
    env = arglist->cdr->car;
    if (!env->query_binding)
      /* Invalid environment */
      return 0;
  }
  else
    env = Environment();
  return arglist->car->eval(env, globals);
}

object f_apply(object arglist, Environment env, Environment globals)
{
  return arglist->car->apply(arglist->cdr->car, env, globals);
}

object f_add(object arglist, Environment env, Environment globals)
{
  int sum = 0;

  while(arglist != Lempty)
  {
    if (!arglist->car->is_number)
      return 0;
    sum += arglist->car->value;
    arglist = arglist->cdr;
  }
  return Number(sum);
}

object f_mult(object arglist, Environment env, Environment globals)
{
  int product = 1;

  while(arglist != Lempty)
  {
    if (!arglist->car->is_number)
      return 0;
    product *= arglist->car->value;
    arglist = arglist->cdr;
  }
  return Number(product);
}

object f_subtract(object arglist, Environment env, Environment globals)
{
  if (arglist == Lempty)
    return Number( 0 );

  if (!arglist->car->is_number)
    return 0;

  if (arglist->cdr == Lempty)
    return Number(- arglist->car->value);

  object diff = arglist->car->value;
  arglist = arglist->cdr;
  do
  {
    if (!arglist->car->is_number)
      return 0;
    diff -= arglist->car->value;
  } while( (arglist = arglist->cdr) != Lempty);

  return Number(diff);
}

object f_equal(object arglist, Environment env, Environment globals)
{
  return ( (arglist->car == arglist->cdr->car)
	   || (arglist->car->value == arglist->cdr->car->value))
    ? Ltrue : Lfalse;
}

object f_lt(object arglist, Environment env, Environment globals)
{
  return (arglist->car->value < arglist->cdr->car->value)
    ? Ltrue : Lfalse;
}

object f_gt(object arglist, Environment env, Environment globals)
{
  return (arglist->car->value > arglist->cdr->car->value)
    ? Ltrue : Lfalse;
}

object f_concat(object arglist, Environment env, Environment globals)
{
  string res="";
  do {
    if (!arglist->car->is_string)
      return 0;
    res += arglist->car->value;
  } while( (arglist = arglist->cdr) != Lempty);
  return String( res );
}

object f_read_string(object arglist, Environment env, Environment globals)
{
  if (arglist->car->is_string)
    return Parser(arglist->car->to_string())->read();
}

object f_readline(object arglist, Environment env, Environment globals)
{
  if (!arglist->car->is_string)
    return 0;
  readline->set_prompt(arglist->car->to_string());
  string s = readline->read();
  return s ? String(s) : Lfalse;
}

object f_display(object arglist, Environment env, Environment globals)
{
  while(arglist != Lempty)
  {
    write(arglist->car->print(1));
    arglist = arglist->cdr;
  }
  write("\n");
  return Lfalse;
}

object f_global_environment(object arglist, Environment env, Environment globals)
{
  return globals;
}

//! Adds the special functions quote, set!, setq,
//! while, define, defmacro, lambda, if, and, or,
//! begin and catch to the @[environment].
void init_specials(Environment environment)
{
  environment->extend(make_symbol("quote"), Special(s_quote));
  environment->extend(make_symbol("set!"), Special(s_setq));
  // environment->extend(make_symbol("setq"), Special(s_setq));
  environment->extend(make_symbol("while"), Special(s_while));
  environment->extend(make_symbol("define"), Special(s_define));
  environment->extend(make_symbol("defmacro"), Special(s_defmacro));
  environment->extend(make_symbol("lambda"), Special(s_lambda));
  environment->extend(make_symbol("if"), Special(s_if));
  environment->extend(make_symbol("and"), Special(s_and));
  environment->extend(make_symbol("or"), Special(s_or));
  environment->extend(make_symbol("begin"), Special(s_begin));
  environment->extend(make_symbol("catch"), Special(s_catch));
}

//! Adds the functions +, *, -, =, <, >,
//! concat, read-string, eval,
//! apply, global-environment, var, cdr, null?,
//! setcar!, setcdr!, cons and list to the
//! @[environment].
void init_functions(Environment environment)
{
  environment->extend(make_symbol("+"), Builtin(f_add));
  environment->extend(make_symbol("*"), Builtin(f_mult));
  environment->extend(make_symbol("-"), Builtin(f_subtract));
  environment->extend(make_symbol("="), Builtin(f_equal));
  environment->extend(make_symbol("<"), Builtin(f_lt));
  environment->extend(make_symbol(">"), Builtin(f_gt));

  environment->extend(make_symbol("concat"), Builtin(f_concat));

  environment->extend(make_symbol("read-string"), Builtin(f_read_string));
  // environment->extend(make_symbol("print"), Builtin(f_print));
  // environment->extend(make_symbol("princ"), Builtin(f_print));
  environment->extend(make_symbol("eval"), Builtin(f_eval));
  environment->extend(make_symbol("apply"), Builtin(f_apply));
  environment->extend(make_symbol("global-environment"),
		      Builtin(f_global_environment));
  environment->extend(make_symbol("car"), Builtin(f_car));
  environment->extend(make_symbol("cdr"), Builtin(f_cdr));
  environment->extend(make_symbol("null?"), Builtin(f_null));
  environment->extend(make_symbol("setcar!"), Builtin(f_setcar));
  environment->extend(make_symbol("setcdr!"), Builtin(f_setcdr));
  environment->extend(make_symbol("cons"), Builtin(f_cons));
  environment->extend(make_symbol("list"), Builtin(f_list));
}

object default_boot_code =
Parser(
  "(begin\n"
  "  (defmacro (cddr x)\n"
  "    (list (quote cdr) (list (quote cdr) x)))\n"
  "  (defmacro (cadr x)\n"
  "    (list (quote car) (list (quote cdr) x)))\n"
  "  (defmacro (cdar x)\n"
  "    (list (quote cdr) (list (quote car) x)))\n"
  "  (defmacro (caar x)\n"
  "    (list (quote car) (list (quote car) x)))\n"
  "\n"
  //"  (defmacro (defun name arguments . body)\n"
  //"    (cons (quote define) (cons (cons name arguments) body)))\n"
  //"\n"
  "  (defmacro (when cond . body)\n"
  "    (list (quote if) cond\n"
  "	  (cons (quote begin) body)))\n"
  "  \n"
  "  (define (map fun list)\n"
  "    (if (null? list) (quote ())\n"
  "      (cons (fun (car list))\n"
  "	         (map fun (cdr list)))))\n"
  "\n"
  "  (defmacro (let decl . body)\n"
  "    (cons (cons (quote lambda)\n"
  "		(cons (map car decl) body))\n"
  "	  (map cadr decl))))")->read();

//! Creates a new environment on which
//! it runs init_functions, init_specials
//! and the following boot code.
//! @code
//! (begin
//!   (defmacro (cddr x)
//!     (list (quote cdr) (list (quote cdr) x)))
//!   (defmacro (cadr x)
//!     (list (quote car) (list (quote cdr) x)))
//!   (defmacro (cdar x)
//!     (list (quote cdr) (list (quote car) x)))
//!   (defmacro (caar x)
//!     (list (quote car) (list (quote car) x)))
//!
//!   (defmacro (when cond . body)
//!     (list (quote if) cond
//! 	  (cons (quote begin) body)))
//!
//!   (define (map fun list)
//!     (if (null? list) (quote ())
//!       (cons (fun (car list))
//! 	         (map fun (cdr list)))))
//!
//!   (defmacro (let decl . body)
//!     (cons (cons (quote lambda)
//! 		(cons (map car decl) body))
//! 	  (map cadr decl))))
//! @endcode
Environment default_environment()
{
  Environment env = Environment();
  init_specials(env);
  init_functions(env);
  default_boot_code->eval(env, env);
  return env;
}

//! Instantiates a copy of the default environment and
//! starts an interactive main loop that connects to
//! standard I/O. The main loop is as follows:
//! @code
//! (begin
//!    (define (loop)
//!      (let ((line (read-line \"PLIS: \")))
//!          (if line
//!              (let ((res (catch (eval (read-string line)
//!                                     (global-environment)))))
//!                 (display res)
//!                (loop)))))
//!    (loop))
//! @endcode
void main()
{
  Environment e = default_environment();
  // e->extend(make_symbol("global-environment"), e);
  readline = Stdio.Readline();
  readline->enable_history(512);
  e->extend(make_symbol("read-line"), Builtin(f_readline));
  e->extend(make_symbol("display"), Builtin(f_display));

  object o = Parser(
    "(begin\n"
    "   (define (loop)\n"
    "     (let ((line (read-line \"PLIS: \")))\n"
    "         (if line \n"
    "    	 (let ((res (catch (eval (read-string line)\n"
    "                                    (global-environment)))))\n"
    "    	    (display res)\n"
    "               (loop)))))\n"
    "   (loop))\n")->read();

  o->eval(e, e);
}
