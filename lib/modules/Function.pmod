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
function curry(function f)
{
  return lambda(mixed ... args1) {
	   return lambda(mixed ... args2) {
		    return f(@args1, @args2);
		  };
	 };
}
