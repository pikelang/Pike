/* PLIS.pmod
 *
 * PLIS (Permuted Lisp). A Lisp language somewhat similar to scheme.
 */

/* Data shared between all Lisp objects */
mapping symbol_table = ([ ]);

object Lempty = Nil();
object Lfalse = Boolean();
object Ltrue = Boolean();


/* Lisp types */

class LObject 
{
}

class SelfEvaluating 
{
  inherit LObject;
  object eval(object env, object globals)
    {
      return this_object();
    }
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
      car = a; cdr = d;
    }

  object mapcar(string|function fun, mixed ...extra)
    {
      object new_car, new_cdr;
      new_car = stringp(fun)? car[fun](@extra) : fun(car, @extra);
      if (!new_car)
      {
	werror("No car");
	return 0;
      }
    
      object new_cdr = (cdr != Lempty) ? cdr->mapcar(fun, @extra)
	: cdr;
      if (cdr) 
	return object_program(this_object())(new_car, new_cdr);
      else
      {
	werror("No cdr");
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
  
  object eval(object env, object globals)
    {
      object fun = car->eval(env, globals);
      if (fun && fun->is_special)
	return fun->apply(cdr, env, globals);

      object args = cdr->mapcar("eval", env, globals);
      if (args)
	return fun->apply(args, env, globals);
      else
      {
	werror("No function to eval");
	return 0;
      }
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

  object eval(object env, object globals)
    {
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
    }
  
  object mapcar(mixed ...ignored) { return this_object(); }
  object map(mixed ...ignored) { return this_object(); }
}

class String 
{
  inherit SelfEvaluating;
  string value;

  constant is_string = 1;
  
  void create(string s)
    {
      value = s;
    }
  
  string print(int display)
    {
      return display ? ("\"" + replace(value, ({ "\"", "\n",}),
				       ({ "\\\"", "\\n"}) ) + "\"")
	: value;
    }
  
  string to_string() { return value; }
}

class Number 
{
  inherit SelfEvaluating;
  int|float|object value;

  constant is_number = 1;
  
  void create(int|float|object x) { value = x; }

  string print(int display) { return (string) value; }
}
  
class Binding 
{
  object value;
  object query() { return value; }
  void set(object v) { value = v; }
  void create(object v) { value = v; }
}
  
class Environment 
{
  inherit LObject;
  // int eval_limit; // ugly hack..
      
  /* Mapping of symbols and their values.
   * As a binding may exist in several environments, they
   * are accessed indirectly. */
  mapping env = ([ ]);
  // object id; // roxen typ ID.

  object query_binding(object symbol)
    {
      return env[symbol];
    }

  void create(mapping|void bindings)
    {
      env = bindings || ([ ]);
    }

  object copy() { return object_program(this_object())(copy_value(env)); };

  object extend(object symbol, object value)
    {
      //     werror(sprintf("Binding '%s'\n", symbol->print(1)));
      env[symbol] = Binding(value);
    }

  string print(int display) 
    {
      string res="";
      foreach(indices(env), object s)
      {
	if(env[s]->value != this_object())
	  res += s->print(display)+": "+env[s]->value->print(display)+"\n";
	else
	  res += "global-environment: ...\n";
      }
      return res;
    }
}
  
class Lambda
{
  inherit LObject;

  object formals; /* May be a dotted list */
  object list; /* expressions */

  void create(object formals_list, object expressions)
    {
      formals = formals_list;
      list = expressions;
    }

  string print(int display) { return "lambda "+list->print(display); }

  int build_env1(object env, object symbols, object arglist)
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

  object build_env(object env, object arglist)
    {
      object res = env->copy();
      return build_env1(res, formals, arglist) ? res : 0;
    }

  object new_env(object env, object arglist);
  
  object apply(object arglist, object env, object globals)
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
  object apply(object arglist, object env, object globals)
    {
      return ::apply(arglist, env, globals)->eval(env, globals);
    }
}

class Dynamic 
{
  inherit Lambda;
  object new_env(object env, object arglist)
    {
      return build_env(env, arglist);
    }
}

class Builtin 
{
  inherit LObject;
  
  function apply;

  void create(function f)
    {
      apply = f;
    }

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
object make_symbol(string name)
{
  return symbol_table[name] || Symbol(name, symbol_table);
}

object make_list(object ...args)
{
  object res = Lempty;
  for (int i = sizeof(args) - 1; i >= 0; i--)
    res = Cons(args[i], res);
  return res;
}


/* Parser */

class Parser 
{
  object number_re = Regexp("^(-|)([0-9]+)");
  object symbol_re = Regexp("^([^0-9 \t\n(.)\"]+)");
  object space_re = Regexp("^([ \t\n]+)");
  object comment_re = Regexp("^(;[^\n]*\n)");
  object string_re = Regexp("^(\"[^\"]*\")");
  
  string buffer;
  object globals;
  
  void create(string s, object ctx)
    {
      buffer = s;
      globals = ctx;
    }

  object read_list();
  
  mixed _read()
    {
      if (!strlen(buffer))
      {
	return 0;
      }
      array a;
      if (a = space_re->split(buffer) || comment_re->split(buffer))
      {
	//	werror(sprintf("Ignoring space and comments: '%s'\n", a[0]));
	buffer = buffer[strlen(a[0])..];
	return _read();
      }
      if (a = number_re->split(buffer))
      {
	//	werror("Scanning number\n");
	string s = `+(@ a);
	buffer = buffer[ strlen(s) ..];
	return Number(Gmp.mpz(s));
      }
      if (a = symbol_re->split(buffer))
      {
	// 	werror("Scanning symbol\n");
	buffer = buffer[strlen(a[0])..];
	return globals->make_symbol(a[0]);
      }
      if (a = string_re->split(buffer))
      {
	//	werror("Scanning string\n");
	buffer = buffer[strlen(a[0])..];
	return String(a[0][1 .. strlen(a[0]) - 2]);
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
	if (strlen(buffer) < 2)
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
	  object final = _read();
	  if (intp(final) || (_read() != ')'))
	  {
	    return 0;
	  }
	  return final;
	default:
	  throw( ({ "lisp->parser: internal error\n",
		    backtrace() }) );
	}
      return Cons(item , read_list());
    }
}


/* Lisp special forms */

object s_quote(object arglist, object env, object globals)
{
  return arglist->car;
}

object s_setq(object arglist, object env, object globals)
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

object s_define(object arglist, object env, object globals)
{
  object symbol, value;
  if (arglist->car->car)
  { /* Function definition */
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

object s_defmacro(object arglist, object env, object globals)
{
  object symbol = arglist->car->car;
  object value = Macro(env, arglist->car->cdr, arglist->cdr);
  if (!value)
    return 0;
  env->extend(symbol, value);
  return symbol;
}
  
object s_if(object arglist, object env, object globals)
{
  if (arglist->car->eval(env, globals) != Lfalse)
    return arglist->cdr->car->eval(env, globals);
  object arglist = arglist->cdr->cdr;
  return (arglist != Lempty) ? arglist->car->eval(env, globals) : Lfalse;
}

object s_and(object arglist, object env, object globals)
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

object s_or(object arglist, object env, object globals)
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

object s_begin(object arglist, object env, object globals)
{
  return arglist->map("eval", env, globals);
}

object s_lambda(object arglist, object env, object globals)
{
  return Lexical(env, arglist->car, arglist->cdr);
}

/* In general, errors are signaled by returning 0, and are
 * fatal.
 *
 * The catch special form catches errors, returning Lfalse
 * if an error occured. */
object s_catch(object arglist, object env, object globals)
{
  return s_begin(arglist, env, globals) || Lfalse;
}

object s_while(object arglist, object env, object globals)
{
  object expr = arglist->car, res;
  object to_eval = arglist->cdr;
  werror( to_eval->print(1) );
  
  while ( (res = expr->eval(env,globals)) != Lfalse)
    to_eval->map("eval", env, globals);//f_eval (to_eval,env,globals);
  return res;
}


/* Functions */

object f_car(object arglist, object env, object globals)
{
  return arglist->car->car;
}

object f_cdr(object arglist, object env, object globals)
{
  return arglist->car->cdr;
}

object f_cons(object arglist, object env, object globals)
{
  return MutableCons(arglist->car, arglist->cdr->car);
}

object f_list(object arglist, object env, object globals)
{
  return arglist;
}

object f_setcar(object arglist, object env, object globals)
{
  if (arglist->car->is_mutable_cons)
    return arglist->car->car = arglist->cdr->car;
  return 0;
}

object f_setcdr(object arglist, object env, object globals)
{
  if (arglist->car->is_mutable_cons)
    return arglist->car->cdr = arglist->cdr->car;
  return 0;
}

object f_eval(object arglist, object env, object globals)
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

object f_apply(object arglist, object env, object globals)
{
  return arglist->car->apply(arglist->cdr->car, env, globals);
}

object f_add(object arglist, object env, object globals)
{
  object sum = Gmp.mpz(0);
  while(arglist != Lempty)
  {
    if (!arglist->car->is_number)
      return 0;
    sum += arglist->car->value;
    arglist = arglist->cdr;
  }
  return Number(sum);
}

object f_mult(object arglist, object env, object globals)
{
  object product = Gmp.mpz(1);
  while(arglist != Lempty)
  {
    if (!arglist->car->is_number)
      return 0;
    product *= arglist->car->value;
    arglist = arglist->cdr;
  }
  return Number(product);
}

object f_subtract(object arglist, object env, object globals)
{
  if (arglist == Lempty)
    return Number(Gmp.mpz(0));

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

object f_equal(object arglist, object env, object globals)
{
  return ( (arglist->car == arglist->cdr->car)
	   || (arglist->car->value == arglist->cdr->car->value))
    ? Ltrue : Lfalse;
}

object f_lt(object arglist, object env, object globals)
{
  return (arglist->car->value < arglist->cdr->car->value)
    ? Ltrue : Lfalse;
}

object f_gt(object arglist, object env, object globals)
{
  return (arglist->car->value > arglist->cdr->car->value)
    ? Ltrue : Lfalse;
}

object f_concat(object arglist, object env, object globals)
{
  string res="";
  do {
    if (!arglist->car->is_string)
      return 0;
    res += arglist->car->value;
  } while( (arglist = arglist->cdr) != Lempty);
  return String( res );
}

void init_specials(object environment)
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

void init_functions(object environment)
{
  environment->extend(make_symbol("+"), Builtin(f_add));
  environment->extend(make_symbol("*"), Builtin(f_mult));
  environment->extend(make_symbol("-"), Builtin(f_subtract));
  environment->extend(make_symbol("="), Builtin(f_equal));
  environment->extend(make_symbol("<"), Builtin(f_lt));
  environment->extend(make_symbol(">"), Builtin(f_gt));

  environment->extend(make_symbol("concat"), Builtin(f_concat));

  // environment->extend(make_symbol("read"), Builtin(f_read));
  // environment->extend(make_symbol("print"), Builtin(f_print));
  // environment->extend(make_symbol("princ"), Builtin(f_print));
  environment->extend(make_symbol("eval"), Builtin(f_eval));
  environment->extend(make_symbol("apply"), Builtin(f_apply));
  // environment->extend(make_symbol("global-environment"), environment);
  environment->extend(make_symbol("car"), Builtin(f_car));
  environment->extend(make_symbol("cdr"), Builtin(f_cdr));
  environment->extend(make_symbol("setcar!"), Builtin(f_setcar));
  environment->extend(make_symbol("setcdr!"), Builtin(f_setcdr));
  environment->extend(make_symbol("cons"), Builtin(f_cons));
  environment->extend(make_symbol("list"), Builtin(f_list));
}

