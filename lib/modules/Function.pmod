#pike __REAL_VERSION__

constant defined = __builtin.function_defined;

//! Calls the given function with the @tt{args@} array plus the optional
//! extra arguments as its arguments and returns the result.
//!
//! Most useful in conjunction with @ref{map@}, and particularly in combination
//! with @ref{sscanf@} with @tt{"...%{...%}..."@} scan strings (which indeed
//! was what it was invented for in the first place).
//!
//! @example
//!   @code{
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
//!   @}
//!
//! @param args
//!  the first arguments the function @tt{f@} expects
//! @param f
//!  the function to apply the arguments on
//! @param extra
//!  optional extra arguments to send to @tt{f@}
//! @returns
//!  whatever the supplied function @tt{f@} returns
mixed splice_call(array args, function f, mixed|void ... extra)
{
  return f(@args, @extra);
}
