// $Id$

//! This module contains special values used by various modules, e.g.
//! a null value used both by @[Sql] and @[Standards.JSON].
//!
//! In many ways these values should be considered constant, but it is
//! possible for a program to replace them with extended versions,
//! provided they don't break the behavior of the base classes defined
//! here. Since there is no good mechanism to handle such extending in
//! several steps, pike libraries should preferably ensure that the
//! base classes defined here provide required functionality directly.
//!
//! @note
//! Since resolving using the dot operator in e.g. @[Val.null] is
//! done at compile time, replacement of these values often must take
//! place very early (typically in a loader before the bulk of the
//! pike code is compiled) to be effective in such cases. For this
//! reason, pike libraries should use dynamic resolution through e.g.
//! @expr{->@} or @expr{master()->resolv()@} instead.

class True
//! Type for the @[Val.true] object. Do not create more instances of
//! this - use @[Val.true] instead.
{
  constant is_val_true = 1;
  //! Nonzero recognition constant.

  string encode_json() {return "true";}

  // The following shouldn't be necessary if there's only one
  // instance, but that might not always be the case.
  protected int __hash()
    {return 34123;}
  protected int `== (mixed other)
    {return objectp (other) && other->is_val_true;}

  protected mixed cast (string type)
  {
    switch (type) {
      case "int": return 1;
      case "string": return "1";
      default: error ("Cannot cast %O to %s.\n", this, type);
    }
  }

  protected string _sprintf (int flag)
  {
    return flag == 'O' && "Val.true";
  }
}

class False
//! Type for the @[Val.false] object. Do not create more instances of
//! this - use @[Val.false] instead.
{
  constant is_val_false = 1;
  //! Nonzero recognition constant.

  protected int `!() {return 1;}

  string encode_json() {return "false";}

  protected int __hash()
    {return 54634;}
  protected int `== (mixed other)
    {return objectp (other) && other->is_val_false;}

  protected mixed cast (string type)
  {
    switch (type) {
      case "int": return 0;
      case "string": return "0";
      default: error ("Cannot cast %O to %s.\n", this, type);
    }
  }

  protected string _sprintf (int flag)
  {
    return flag == 'O' && "Val.false";
  }
}

True true = True();
False false = False();
//! Objects that represents the boolean values true and false. In a
//! boolean context these evaluate to true and false, respectively.
//!
//! They produce @expr{1@} and @expr{0@}, respectively, when cast to
//! integer, and @expr{"1"@} and @expr{"0"@} when cast to string. They
//! do however not compare as equal to the integers 1 and 0, or any
//! other values. @[Val.true] only compares (and hashes) as equal with
//! other instances of @[True] (although there should be as few as
//! possible). Similarly, @[Val.false] is only considered equal to
//! other @[False] instances.
//!
//! @[Protocols.JSON] uses these objects to represent the JSON
//! literals @expr{true@} and @expr{false@}.
//!
//! @note
//! The correct way to programmatically recognize these values is
//! something like
//!
//! @code
//!   if (objectp(something) && something->is_val_true) ...
//! @endcode
//!
//! and
//!
//! @code
//!   if (objectp(something) && something->is_val_false) ...
//! @endcode
//!
//! respectively. See @[Val.null] for rationale.

class Null
//! Type for the @[Val.null] object. Do not create more instances of
//! this - use @[Val.null] instead.
{
  inherit Builtin.SqlNull;

  constant is_val_null = 1;
  //! Nonzero recognition constant.

  protected string _sprintf (int flag)
  {
    return flag == 'O' && "Val.null";
  }
}

Null null = Null();
//! Object that represents a null value.
//!
//! In general, null is a value that represents the lack of a real
//! value in the domain of some type. For instance, in a type system
//! with a null value, a variable of string type typically can hold
//! any string and also null to signify no string at all (which is
//! different from the empty string). Pike natively uses the integer 0
//! (zero) for this, but since 0 also is a real integer it is
//! sometimes necessary to have a different value for null, and then
//! this object is preferably used.
//!
//! This object is false in a boolean context. It does not cast to
//! anything, and it is not equal to anything else but other instances
//! of @[Null] (which should be avoided).
//!
//! This object is used by the @[Sql] module to represent SQL NULL,
//! and it is used by @[Protocols.JSON] to represent the JSON literal
//! @expr{null@}.
//!
//! @note
//! Do not confuse the null value with @[UNDEFINED]. Although
//! @[UNDEFINED] often is used to represent the lack of a real value,
//! and it can be told apart from an ordinary zero integer with some
//! effort, it is transient in nature (for instance, it automatically
//! becomes an ordinary zero when inserted in a mapping). That makes
//! it unsuitable for use as a reliable null value.
//!
//! @note
//! The correct way to programmatically recognize @[Val.null] is
//! something like
//!
//! @code
//!   if (objectp(something) && something->is_val_null) ...
//! @endcode
//!
//! That way it's possible for other code to replace it with an
//! extended class, or create their own variants which needs to behave
//! like @[Val.null].
//!
//! @fixme
//! The Oracle glue currently uses static null objects which won't be
//! affected if this object is replaced.
