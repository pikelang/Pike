//! An abstract data type for binary relations.
//!
//! This datatype implements something similar to
//! a set of tuples <@tt{left@}, @tt{right@}>, or
//! a multi-valued mapping.

#pike __REAL_VERSION__

private mapping(mixed:multiset) val          = ([]);
private mixed                   id;
private int                     items        = 0;
private int                     need_recount = 0;

constant is_binary_relation = 1;

//! Return true/false: does the relation "@[left] R @[right]" exist?
mixed contains(mixed left, mixed right)
{
  return val[left] && val[left][right];
}

//! Does the same as the @[contains] function: returns true if the
//! relation "@[left] R @[right]" exists, and otherwise false.
protected mixed `()(mixed left, mixed right)
{
  return contains(left, right);
}

//! Adds "@[left] R @[right]" as a member of the relation. Returns
//! the same relation.
this_program add(mixed left, mixed right)
{
  if (!val[left])
    val[left] = (<>);
  if (!val[left][right])
    ++items, val[left][right] = 1;
  return this;
}

//! Removes "@[left] R @[right]" as a member of the relation. Returns
//! the same relation.
this_program remove(mixed left, mixed right)
{
  if (val[left] && val[left][right])
    --items, val[left][right] = 0;
  if (!sizeof(val[left]))
      m_delete(val, left);
  return this;
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
this_program filter_destructively(function f)
{
  foreach(indices(val), mixed left)
  {
    foreach(indices(val[left]), mixed right)
      if (!f(left, right))
        remove(left, right);
    if (sizeof(val[left]) == 0)
      val[left] = 0;
  }
  return this;
}

//! Returns the number of relation entries in the relation. (Or with
//! other words: the number of relations in the relation set.)
protected mixed _sizeof()
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

protected int(0..1) `==(mixed rel)
{
  if (!objectp(rel) || !rel->is_binary_relation)
    return 0; // different because of having different types

  return this <= rel && rel <= this;
}

protected int(0..1) `>=(object rel)
{
  return rel <= this;
}

protected int(0..1) `!=(mixed rel)
{
  return !(this == rel);
}

//! The expression `rel1 & rel2' returns a new relation which has
//! those and only those relation entries that are present in both
//! rel1 and rel2.
protected mixed `&(mixed rel)
{
  return filter(lambda (mixed left, mixed right)
                       { return rel->contains(left, right);});
}

//! @decl mixed `+(mixed rel)
//! @decl mixed `|(mixed rel)
//! The expression `rel1 | rel2' and `rel1 + rel2' returns a new
//! relation which has all the relation entries present in rel1,
//! or rel2, or both.

protected mixed `|(mixed rel)
{
  ADT.Relation.Binary res = ADT.Relation.Binary(id, rel);
  foreach(indices(val), mixed left)
    foreach(indices(val[left]), mixed right)
      res->add(left, right);
  return res;
}

protected mixed `+ = `|;

//! The expression `rel1 - rel2' returns a new relation which has
//! those and only those relation entries that are present in rel1
//! and not present in rel2.
protected mixed `-(mixed rel)
{
  return filter(lambda (mixed left, mixed right)
                       { return !rel->contains(left, right);});
}

//! Makes the relation symmetric, i.e. makes sure that if xRy is part
//! of the relation set, then yRx should also be a part of the relation
//! set.
this_program make_symmetric()
{
  foreach(indices(val), mixed left)
    foreach(indices(val[left]), mixed right)
      add(right, left);
  return this;
}

//! Assuming the relation's domain and range sets are equal, and that
//! the relation xRy means "there is a path from node x to node y",
//! @[find_shortest_path] attempts to find a path with a minimum number
//! of steps from one given node to another. The path is returned as an
//! array of nodes (including the starting and ending node), or 0 if no
//! path was found. If several equally short paths exist, one of them
//! will be chosen pseudorandomly.
//!
//! Trying to find a path from a node to itself will always succeed,
//! returning an array of one element: the node itself. (Or in other
//! words, a path with no steps, only a starting/ending point).
//!
//! The argument @[avoiding] is either 0 (or omitted), or a multiset of
//! nodes that must not be part of the path.
array|zero find_shortest_path(mixed from, mixed to, void|multiset avoiding)
{
  if (from == to)
     return ({ from });
  if (!val[from])
     return 0;
  if (avoiding && avoiding[to])
     return 0;
  if (contains(from, to))
     return ({ from, to });

  // NOTE: This is a simple, more or less depth-first search. Worst-
  // case time complexity could probably be improved, e.g. by a
  // breadth-first search (such as Dijkstra's path-finding algorithm).
  // But those algorithms typically have worse space complexity, so
  // this will do for now.

  array|zero subpath, found = 0;
  avoiding = avoiding ? avoiding + (< from >) : (< from >);
  foreach(indices(val[from]), mixed right)
    if (!avoiding[right])
      if (subpath = find_shortest_path(right, to, avoiding))
        if (!found || sizeof(subpath)+1 < sizeof(found))
        {
          found = ({ from, @subpath });
          if (sizeof(subpath) == 1)
          {
            // We can't find a shorter path than this, so there's no
            // point in looking for more alternatives.
            break;
          }
        }
  return found;
}

protected string _sprintf(int mode)
{
  return mode=='O' && sprintf("%O(%O %O)", this_program, id, _sizeof());
}

//! Return the ID value which was given as first argument to create().
mixed get_id()
{
  return id;
}

protected void create(void|mixed _id, void|mapping|object _initial)
{
  id = _id;
  if (objectp(_initial) && _initial->is_binary_relation)
    _initial->map(lambda (mixed left, mixed right) { add(left,right); });
  else if (mappingp(_initial))
    foreach(indices(_initial), mixed key)
      add(key, _initial[key]);
  else if (_initial)
    error("Bad initializer for ADT.Relation.Binary.\n");
}

//! An iterator which makes all the left/right entities in the relation
//! available as index/value pairs.
protected class _get_iterator {

  protected int(-1..) ipos = -1;
  protected int(-1..) vpos = -1;

  protected array lefts = indices(val);
  protected array rights = ({});

  protected mixed _iterator_index() {
    if (ipos < 0) return UNDEFINED;
    return lefts[ipos];
  }

  protected mixed _iterator_value() {
    if (ipos < 0) return UNDEFINED;
    return rights[vpos];
  }

  protected int(0..) _iterator_next()
  {
    vpos++;
    if (vpos >= sizeof(rights)) {
      ipos++;
      vpos = 0;
      if (ipos >= sizeof(lefts)) {
	ipos = -1;
	vpos = -1;
	return UNDEFINED;
      }
      rights = indices(val[lefts[ipos]]);
      // NB: We assume that rights is not empty.
    }

    return ipos;
  }

  protected this_program `+=(int steps) {
    if (steps < 0) error ("Cannot step backwards.\n");
    while(steps--) {
      if (zero_type(_iterator_next())) {
	// Stop at the last element.
	ipos = sizeof(lefts) - 1;
	vpos = sizeof(rights) - 1;
	break;
      }
    }
    return this;
  }

  int(0..1) first()
  {
    ipos = -1;
    vpos = -1;
    return !!sizeof(lefts);
  }
}

protected mixed cast(string to)
{
  if( to=="mapping" )
    return copy_value(val);
  return UNDEFINED;
}
