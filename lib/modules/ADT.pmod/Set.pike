// $Id: Set.pike,v 1.1 2004/10/10 23:05:30 stensson Exp $

//! ADT.Set implements a datatype for sets. These sets behave much
//! like multisets, except that they are restricted to containing only
//! one instance of each member value.
//!
//! From a performance viewpoint, it is probably more efficient for a
//! Pike program to use mappings to serve as sets, rather than using
//! an ADT.Set,so ADT.Set is mainly provided for the sake of completeness
//! and code readability.

#pike __REAL_VERSION__


private multiset set;


//! Remove all items from the set.
void reset()
{
  set = (<>);
}


//! Add @[items] to the set.
void add(mixed ... items)
{
  foreach(items, mixed item)
    if (!set[item])
      set[item] = 1;
}


//! Remove @[items] from the set.
void remove(mixed ... items)
{
  foreach(items, mixed item)
    if (set[item])
      set[item] = 0;
}


//! Check whether a value is a member of the set.
int(0..1) contains(mixed item)
{
  return set[item];
}


//! Returns 1 if the set is empty, otherwise 0.
int(0..1) is_empty()
{
  return sizeof(set) == 0;
}


//! Map the values of a set: calls the map function @[f] once for each
//! member of the set, returning an array which contains the result of
//! each one of those function calls. Note that since a set isn't
//! ordered, the values in the returned array will be in more or less
//! random order. If you need to know which member value produced which
//! result, you have to make that a part of what the filtering function
//! returns.
//!
//! The filtering function @[f] is called with a single, mixed-type
//! argument which is the member value to be mapped.
array(mixed) map(function f)
{
  array a = allocate(sizeof(set));
  int   i = 0;

  foreach(indices(set), mixed item)
    a[i++] = f(item);

  return a;
}


//! Return a filtered version of the set, containing only those members
//! for which the filtering function @[f] returned true.
//!
//! The filtering function is called with a single mixed-type argument
//! which is the member value to be checked.
this_program filter(function f)
{
  ADT.Set result = ADT.Set();

  foreach(indices(set), mixed item)
    if (f(item))
      result->add(item);

  return result;
}


//! Destructively filter the set, i.e. remove every element for which
//! the filtering function @[f] returns 0, and then return the set.
//!
//! The filtering function is called with a single mixed-type argument
//! which is the member value to be checked.
this_program filter_destructively(function f)
{
  ADT.Set result = ADT.Set();

  foreach(indices(set), mixed item)
    if (f(item))
      result->add(item);

  return this;
}


/////////////////
/// OPERATORS ///
/////////////////


//! Subset. A <= B returns true if all items in A are also present in B.
int(0..1) subset(ADT.Set other)
{
  foreach(indices(set), mixed item)
    if (!other->contains(item))
      return 0;

  return 1;
}


//! Superset. A >= B returns true if all items in B are also present in A.
int(0..1) superset(ADT.Set other)
{
  return other <= this;
}


//! Equality. A == B returns true if all items in A are present in B,
//! and all items in B are present in A. Otherwise, it returns false.
int(0..1) `==(ADT.Set other)
{
  if (predef::`==(this, other))
    return 1; // Identical.

  foreach(indices(set), mixed item)
    if (!other->contains(item))
      return 0;

  foreach(indices(other), mixed item)
    if (!contains(item))
      return 0;

  return 1;
}


//! True subset. A < B returns true if each item in A is also present
//! in B, and B contains at least one item not present in A.
int(0..1) `<(ADT.Set other)
{
  if (sizeof(this) >= sizeof(other))
    return 0;

  return subset(other);
}


//! True superset. A > B returns true if each item in B is also present
//! in A, and A contains at least one item not present in B.
int(0..1) `>(ADT.Set other)
{
  if (sizeof(this) <= sizeof(other))
    return 0;

  return superset(other);
}


//! Subset operator.
function `<= = subset;


//! Superset operator.
function `>= = superset;


//! Inequality. A != B is equivalent to !(A == B).
int(0..1) `!=(ADT.Set other)
{
  return !(this == other);
}


//! Union. Returns a set containing all elements present in either
//! or both of the operand sets.
this_program `|(ADT.Set other)
{
  ADT.Set result = ADT.Set(this);

  foreach(indices(other), mixed item)
    result->add(item);

  return result;
}

mixed `+ = `|; // Addition on sets works the same as union on sets.


//! Intersection. Returns a set containing those values that were
//! present in both the operand sets.
this_program `&(ADT.Set other)
{
  return filter(lambda (mixed x) { return other->contains(x);});
}


//! Difference. The expression 'A - B', where A and B are sets, returns
//! all elements in A that are not also present in B.
this_program `-(ADT.Set other)
{
  return filter(lambda (mixed x) { return !other->contains(x);});
}


//! Indexing a set with a value V gives 1 if V is a member of the
//! set, otherwise 0.
int(0..1) `[](mixed item)
{
  return set[item];
}


//! Setting an index V to 0 removes V from the set. Setting it to
//! a non-0 value adds V as a member of the set.
int `[]=(mixed item, int value)
{
  if (value)
    add(item);
  else
    remove(item);

  return value;
}


////////////////
/// SPECIALS ///
////////////////

//! In analogy with multisets, indices() of an ADT.Set givess an array
//! containing all members of the set.
array(mixed) _indices()
{
  return indices(set);
}


//! In analogy with multisets, values() of an ADT.Set givess an array
//! indicating the number of occurrences in the set for each position
//! in the member array returned by indices(). (Most of the time, this
//! is probably rather useless for sets, since the result is an array
//! which just contain 1's, one for each member of the set. Still,
//! this function is provided for consistency.
array(mixed) _values()
{
  return values(set);
}


//! An ADT.Set can be cast to an array or a multiset.
mixed cast(string to)
{
  switch(to)
  {
    case "object":
      return this;

    case "array":
      return indices(set);

    case "multiset":
      return copy_value(set);

    default:
      error("Cannot cast ADT.Set to %s.\n", to);
  }
}


//! Number of items in the set.
int _sizeof()
{
  return sizeof(set);
}


//! Printable representation of the set.
string _sprintf(int t) {
  return t=='O' && sprintf("%O%O", this_program, cast("array"));
}


//! Create an ADT.Set, optionally initialized from another ADT.Set or
//! a compatible type. If no initial data is given, the set will start
//! out empty.
void create(void|ADT.Set|array|multiset|mapping initial_data)
{
  reset();
  if (arrayp(initial_data))
    add(@initial_data);
  else if (initial_data)
    add(@indices(initial_data));
}
