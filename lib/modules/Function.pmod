#pike __REAL_VERSION__

constant defined = __builtin.function_defined;

//! Calls the given function with the @[args] array plus the optional
//! extra arguments as its arguments and returns the result.
//!
//! Most useful in conjunction with @[map], and particularly in combination
//! with @[sscanf] with @expr{"...%{...%}..."@} scan strings (which indeed
//! was what it was invented for in the first place).
//!
//! @param args
//!  The first arguments the function @[f] expects.
//! @param f
//!  The function to apply the arguments on.
//! @param extra
//!  Optional extra arguments to send to @[f].
//! @returns
//!  Whatever the supplied function @[f] returns.
//!
//! @example
//! @code
//!   class Product(string name, string version)
//!   {
//!     string _sprintf()
//!     {
//!       return sprintf("Product(%s/%s)", name, version);
//!     }
//!   }
//!   map(({ ({ "pike",   "7.1.11" }),
//!          ({ "whitefish", "0.1" }) }),
//!       Function.splice_call, Product);
//!   ({ /* 2 elements */
//!	 Product(pike/7.1.11),
//!	 Product(whitefish/0.1)
//!   })
//! @endcode
mixed splice_call(array args, function f, mixed|void ... extra)
{
  return f(@args, @extra);
}


//! The dreaded fixpoint combinator "Y".
//!
//! The Y combinator is useful when writing recursive lambdas.  It
//! converts a lambda that expects a self-reference as its first argument
//! into one which can be called without this argument.
//!
//! @example
//! This example creates a lambda that computes the faculty function.
//! @code
//!   Function.Y(lambda(function f, int n) { return n>1? n*f(n-1) : 1; })
//! @endcode
//!
//! @seealso
//!   @[this_function]
function Y(function f)
{
  return lambda(function p) {
	   return lambda(mixed ... args) {
		    return f(p(p), @args);
		  };
	 } (lambda(function p) {
	      return lambda(mixed ... args) {
		       return f(p(p), @args);
		     };
	    });
}


//! Partially evaluate a function call.
//!
//! This function allows N parameters to be given to a function taking
//! M parameters (N<=M), yielding a new function taking M-N parameters.
//!
//! What is actually returned from this function is a function taking N
//! parameters, and returning a function taking M-N parameters.
//!
//! @example
//! This example creates a function adding 7 to its argument.
//! @code
//!   Function.curry(`+)(7)
//! @endcode
function(mixed...:function(mixed...:mixed|void)) curry(function f)
{
  return lambda(mixed ... args1) {
	   return lambda(mixed ... args2) {
		    return f(@args1, @args2);
		  };
	 };
}

//! Partially evaluate a function call.
//!
//! This function returns a function that when called will do the
//! specified argument mapping. It is similar to @[curry], but allows
//! more dynamic changes of the argument evaluation, you can leave the
//! first argument unspecified while setting others, or simply change
//! the argument order.
//!
//! @param f
//!   The first argument is the function to be called.
//!
//! @param bind_args
//!   All other arguments are either a generic value, which will be sent
//!   as-is to the function or one of the placeholder values defined in
//!   @[Function.Placeholder], or one of your own implementation (inherit
//!   @[Function.Placeholder.Base] and implement the value function.).
//!
//! @example
//!   This example returns a function that limits the given argument
//!   to between 0 and 99.
//!
//!   @code
//!     import Function.Placeholder;
//!     function clip = Function.bind(limit, 0, arg0, 100);
//!   @endcode
class bind(function f, mixed ... bind_args)
{
    protected string _sprintf(int c) {
        return sprintf("Function.bind(%O%{, %O%})",f,bind_args);
    }

    protected mixed `()(mixed ... args)
    {
        array processed = ({});
        for(int i=0; i<sizeof(bind_args);i++)
        {
            if( objectp(bind_args[i]) && bind_args[i]->_is_placeholder )
            {
                mixed val = bind_args[i]->value(this,args);
                if( bind_args[i]->_splice )
                    processed += val;
                else
                    processed += ({val});
            }
            else
                processed += ({bind_args[i]});
        }
        return f(@processed);
    }
}

//! This function, given a function taking N parameters, returns a new
//! function taking N+1 parameters. The first argument will be
//! ignored.
//!
//! @example
//! @code
//!  >  Function.uncurry(`+)(7,2,3)
//!  Result: 5
//! @endcode
function(mixed...:mixed) uncurry(function f)
{
  return lambda(mixed ... args1) {
       return f(@args1[1..]);
   };
}

//! Call a callback function, but send throws from the callback
//! function (ie, errors) to master()->handle_error.
//! Also accepts if f is zero (0) without error.
//!
//! @example
//! @code
//!   Functions.call_callback(the_callback,some,arguments);
//! @endcode
//! equals
//! @code
//!   {
//!      mixed err=catch { if (the_callback) the_callback(some,arguments); };
//!      if (err) master()->handle_error(err);
//!   }
//! @endcode
//! (Approximately, since call_callback also calls handle_error
//! if 0 were thrown.)
void call_callback(function f,mixed ... args)
{
   if (!f) return;
   mixed err=catch { f(@args); return; };
   handle_error(err);
}

private function handle_error = master()->handle_error;

//! Creates a composite function of the provided functions. The
//! composition function of f() and g(), q(x)=f(g(x)), is created by
//! @expr{function q = Function.composite(f, g);@}.
//!
//! @example
//! @code
//!  map(input/",",
//!      Function.composite(String.trim, upper_case));
//! @endcode
function composite(function ... f)
{
  return lambda(mixed args)
         {
           for(int i; i<sizeof(f); i++)
             args = f[-i-1](args);
           return args;
         };
}


//! @module Placeholder
//!
//! Placeholder arguments for @[Function.bind]

//! @ignore
object Placeholder = class
{
//! @endignore

    class Base
    {
        constant _is_placeholder = true;
    }

    class Arg(int num)
    //! Arg(x) returns the value of argument X
    {
        inherit Base;
        protected string _sprintf(int c) {
            return sprintf("arg%d",num);
        }

        //! @decl mixed value(bind x, array args);
        //!
        //! The function that is called to return the argument value.
        mixed value(bind x, array args)
        {
            if(num>=sizeof(args) || -num>sizeof(args))
                error("No argument %d given\n",num);
            return args[num];
        }
    };

    class Splice(int from, void|int end)
    //! Splice(from) adds all arguments starting with argument number @expr{from@},
    //! optionally ending with end.
    //! Equivalent to @expr{args[from .. end]@}
    {
        inherit Base;
        constant _splice = true;
        array(mixed) value(bind x, array args)
        {
            if( end )
                return args[from..end];
            return args[from..];
        }
    }

    //! @decl Arg rest;
    //! Return all arguments not used by any @[Arg] or @[Splice].
    //!
    //! Unlike @[Splice] this will return non-continous unused arguments.
    //!
    //! @example
    //!  This creates a version of call_out that has the function argument as
    //!  the last argument
    //! @code
    //!  import Function.Placeholder;
    //!  Function.bind( call_out, Arg(-1), rest)
    //! @endcode
    object rest = class {
        inherit Base;
        constant _splice = true;

        array(mixed) value( bind x, array args )
        {
            array ret = copy_value(args);
            foreach(x->bind_args, mixed arg)
            {
                if( Program.inherits(arg,Arg) )
                {
                    int a = arg->num;
                    ret[a] = UNDEFINED;
                }
                else if( Program.inherits(arg,Splice) )
                {
                    int e = min(arg->end+1,sizeof(args));
                    if( e==1 ) e = sizeof(args);
                    for(int i=arg->start; i<e; i++)
                        ret[i] = UNDEFINED;
                }
            }
            return ret - ({ UNDEFINED });
        }
     }();

    //! @decl constant arg0;
    //! @decl constant arg1;
    //! @decl constant arg2;
    //! @decl constant arg3;
    //! @decl constant arg4;
    //! @decl constant arg5;
    //! @decl constant arg6;
    //! @decl constant arg7;
    //! @decl constant arg8;
    //! @decl constant arg9;
    //! arg<n> will return an instance of @[Arg] that returns the n:th arg.
    //! For convenience for c++11 developers _0, _1 etc also works.
    //!
    //! Note that arg0 is the first argument, not arg1

    class Expr(function func, void|bool _splice)
    //! Expr(x) returns the result of calling @[x].
    //! The function will be passed the list of arguments.
    //!
    //! If _splice is true, zero or more argument is returned in an array
    //!
    //! Function.Placeholder.arg1 is thus more or less equivalent to
    //! @code
    //!  Expr(lambda(array args){return args[1];});
    //! @endcode
    {
        inherit Base;
        protected string _sprintf(int c) {
            return sprintf("Expr(%O)",value);
        }
        mixed value( bind x, array args )
        {
            return func(args);
        }
    }

    private mapping _cache = ([]);
    mixed `[](string name)
    {
        if( _cache[name] )
            return _cache[name];
        mixed tmp;
        if(sscanf(name, "arg%d", tmp) || sscanf(name, "_%d", tmp))
            return _cache[name] = Arg(tmp);
        return ::`[](name);
    };
//! @ignore
}();
//! @endignore

//! @endmodule

