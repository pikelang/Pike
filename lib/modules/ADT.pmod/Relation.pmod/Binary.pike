// $Id: Binary.pike,v 1.2 2002/06/14 14:46:56 nilsson Exp $
// An abstract data type for binary relations.

private mapping val   = ([]);
private mixed   id;
private int     items = 0;
private int     need_recount = 0;

constant is_binary_relation = 1;

//! Return true/false: does the relation "@[left] R @[right]" exist? 
mixed contains(mixed left, mixed right)
{
  return val[left] && val[left][right];
}

//! Does the same as the @[contains] function: returns true if the
//! relation "@[left] R @[right]" exists, and otherwise false.
mixed `()(mixed left, mixed right)
{
  return contains(left, right);
}

//! Adds "@[left] R @[right]" as a member of the relation. Returns
//! the same relation.
mixed add(mixed left, mixed right)
{
  if (!val[left])
    val[left] = (<>);
  if (!val[left][right])
    ++items, val[left][right] = 1;
  return this_object();
}

//! Removes "@[left] R @[right]" as a member of the relation. Returns
//! the same relation.
mixed remove(mixed left, mixed right)
{
  if (val[left] && val[left][right])
    --items, val[left][right] = 0;
  return this_object();
}

//! Maps every entry in the relation. The function f gets two
//! arguments: the left and the right relation value. Returns
//! an array with the return values of f for each and every
//! mapped entry.
//!
//! Note: since the entries in the relation are not ordered,
//! the returned array will have its elements in no particular
//! order. If you need to know which relation entry produced which
//! result in the array, you have to make that information part
//! of the value that @[f] returns. 
array map(function f)
{
  array a = ({});
  foreach(indices(val), mixed left)
    foreach(indices(val[left]), mixed right)
      a += ({ f(left, right) });
  return a;
}

//! Filters the entries in the relation, and returns a relation with
//! all those entries for which the filtering function @[f] returned
//! true. The function @[f] gets two arguments: the left and the right
//! value for every entry in the relation.
object filter(function f)
{
  ADT.Relation.Binary res = ADT.Relation.Binary(id);
  foreach(indices(val), mixed left)
    foreach(indices(val[left]), mixed right)
      if (f(left, right))
        res->add(left, right);
  return res;
}

//! Filters the entries in the relation destructively, removing all
//! entries for which the filtering function @[f] returns false.
//! The function @[f] gets two arguments: the left and the right value
//! for each entry in the relation.
object filter_destructively(function f)
{
  foreach(indices(val), mixed left)
  {
    foreach(indices(val[left]), mixed right)
      if (!f(left, right))
        remove(left, right);
    if (sizeof(val[left]) == 0)
      val[left] = 0;
  }
  return this_object();
}

//! Returns the number of relation entries in the relation. (Or with
//! other words: the number of relations in the relation set.)
mixed _sizeof()
{
  if (need_recount)
  {
    int i = 0;
    need_recount = 0;
    foreach(indices(val), mixed left)
      i += sizeof(val[left]);
    items = i;
  }
  return items;
}

//! The expression `rel1 <= rel2' returns true if every relation entry
//! in rel1 is also present in rel2.
mixed `<=(object rel)
{
  foreach(indices(val), mixed left)
    foreach(indices(val[left]), mixed right)
      if (!rel(left, right))
        return 0;
  return 1;
}

mixed `==(mixed rel)
{
  if (predef::`==(rel, this_object()))
    return 1; // equal because of being identical

  if (!objectp(rel) || !rel->is_binary_relation)
    return 0; // different because of having different types

  return this_object() <= rel && rel <= this_object();
}

mixed `>=(object rel)
{
  return rel <= this_object();
}

mixed `!=(mixed rel)
{
  return !(this_object() == rel);
}

//! The expression `rel1 & rel2' returns a new relation which has
//! those and only those relation entries that are present in both
//! rel1 and rel2.
mixed `&(mixed rel)
{
  return filter(lambda (mixed left, mixed right)
                       { return rel->contains(left, right);});
}

//! The expression `rel1 | rel2' returns a new relation which has
//! all the relation entries present in rel1, or rel2, or both.
mixed `|(mixed rel)
{
  ADT.Relation.Binary res = ADT.Relation.Binary(id, rel);
  foreach(indices(val), mixed left)
    foreach(indices(val[left]), mixed right)
      res->add(left, right);
  return res;
}

mixed `+ = `|;

//! The expression `rel1 - rel2' returns a new relation which has
//! those and only those relation entries that are present in rel1
//! and not present in rel2.
mixed `-(mixed rel)
{
  return filter(lambda (mixed left, mixed right)
                       { return !rel->contains(left, right);});
}

string _sprintf(int mode)
{
  if (mode == 'O')
    return sprintf("ADT.Relation.Binary(%O)", id);
  else
    return "ADT.Relation.Binary";
}

//! Return the ID value which was given as first argument to create().
mixed get_id()
{
  return id;
}

void create(void|mixed _id, void|mapping|object _initial)
{
  id = _id;
  if (objectp(_initial) && _initial->is_binary_relation)
    val = _initial->filter(lambda (mixed left, mixed right) { return 1;})->val;
  else if (mappingp(_initial))
    foreach(indices(_initial), mixed key)
      add(key, _initial[key]);
  else if (_initial)
    error("Bad initializer for ADT.Relation.Binary.\n");
}

//! An iterator which makes all the left/right entities in the relation
//! available as index/value pairs.
class Iterator {

  static int(0..) ipos;
  static int(0..) vpos;
  static int(0..1) finished = 1;

  static array lefts;
  static array rights;

  void create() {
    first();
  }

  mixed index() {
    return lefts[ipos];
  }

  mixed value() {
    return rights[vpos];
  }

  int(0..1) `!() {
    return finished;
  }

  int(0..1) next() {

    if(finished || (ipos==sizeof(lefts)-1 &&
		    vpos==sizeof(rights)-1)) {
      finished = 1;
      return 0;
    }

    vpos++;
    if(vpos>sizeof(rights)-1 && !finished) {
      ipos++;
      rights = indices(val[lefts[ipos]]);
      vpos = 0;
    }

    return 1;
  }

  this_program `+=(int steps) {
    while(steps--)
      next();
    return this_object();
  }

  int(0..1) first() {
    lefts = indices(val);
    if(sizeof(lefts)) {
      rights = indices(val[lefts[0]]);
      finished = 0;
    }
    else
      finished = 1;
    return !finished;
  }
}
