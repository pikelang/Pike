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
mixed splice_call(array args, function f, mixed|void ... extra)
{
  return f(@args, @extra);
}
