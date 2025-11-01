//! An abstract data type for binary relations.
//!
//! This datatype implements something similar to
//! a set of tuples <@tt{left@}, @tt{right@}>, or
//! a multi-valued mapping.

#pike __REAL_VERSION__

//! Type for the left values in the relation.
__generic__ LeftType;

//! Type for the right values in the relation.
__generic__ RightType = LeftType;

private mapping(LeftType:multiset(RightType)) val          = ([]);
private mixed                   id;
private int                     items        = 0;
private int                     need_recount = 0;

constant is_binary_relation = 1;

//! Return true/false: does the relation "@[left] R @[right]" exist?
bool contains(LeftType left, RightType right)
{
  return val[left] && val[left][right];
}

//! Does the same as the @[contains] function: returns true if the
//! relation "@[left] R @[right]" exists, and otherwise false.
protected bool `()(LeftType left, RightType right)
{
  return contains(left, right);
}

//! Adds "@[left] R @[right]" as a member of the relation. Returns
//! the same relation.
this_program(<LeftType, RightType>) add(LeftType left, RightType right)
{
  if (!val[left])
    val[left] = (<>);
  if (!val[left][right])
    ++items, val[left][right] = 1;
  return this;
}

//! Removes "@[left] R @[right]" as a member of the relation. Returns
//! the same relation.
this_program(<LeftType, RightType>) remove(LeftType left, RightType right)
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
array map(function(LeftType, RightType:mixed) f)
{
  array a = ({});
  foreach(indices(val), LeftType left)
    foreach(indices(val[left]), RightType right)
      a += ({ f(left, right) });
  return a;
}

//! Filters the entries in the relation, and returns a relation with
//! all those entries for which the filtering function @[f] returned
//! true. The function @[f] gets two arguments: the left and the right
//! value for every entry in the relation.
//!
//! @seealso
//!   @[filter_destructively()]
ADT.Relation.Binary(<LeftType, RightType>) filter(function(LeftType, RightType: mixed) f)
{
  ADT.Relation.Binary(<LeftType, RightType>) res =
    ADT.Relation.Binary(<LeftType, RightType>)(id);
  foreach(indices(val), LeftType left)
    foreach(indices(val[left]), RightType right)
      if (f(left, right))
        res->add(left, right);
  return res;
}

//! Filters the entries in the relation destructively, removing all
//! entries for which the filtering function @[f] returns false.
//! The function @[f] gets two arguments: the left and the right value
//! for each entry in the relation.
//!
//! @seealso
//!   @[filter()]
this_program(<LeftType, RightType>) filter_destructively(function(LeftType,
                                                                  RightType:mixed) f)
{
  foreach(indices(val), LeftType left)
  {
    foreach(indices(val[left]), RightType right)
      if (!f(left, right))
        remove(left, right);
    if (sizeof(val[left]) == 0)
      val[left] = 0;
  }
  return this;
}

//! Returns the number of relation entries in the relation. (Or with
//! other words: the number of relations in the relation set.)
protected int(0..) _sizeof()
{
  if (need_recount)
  {
    int i = 0;
    need_recount = 0;
    foreach(indices(val), LeftType left)
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

//! The expression `´@expr{rel1 & rel2@} returns a new relation which has
//! those and only those relation entries that are present in both
//! rel1 and rel2.
protected ADT.Relation.Binary(<LeftType, RightType>) `&(ADT.Relation.Binary rel)
{
  return filter(lambda (LeftType left, RightType right)
                       { return rel->contains(left, right);});
}

//! @decl ADT.Relation.Binary(<LeftType, RightType>) @
//!         `+(mapping(LeftType:RightType)|this_program rel)
//! @decl ADT.Relation.Binary(<LeftType, RightType>) @
//!         `|(mapping(LeftType:RightType)|this_program rel)
//! The expressions @expr{rel1 | rel2@} and @expr{rel1 + rel2@} return a new
//! relation which has all the relation entries present in rel1,
//! or rel2, or both.

protected ADT.Relation.Binary `|(mapping(LeftType:RightType)|this_program rel)
{
  ADT.Relation.Binary res = ADT.Relation.Binary(id, rel);
  foreach(indices(val), LeftType left)
    foreach(indices(val[left]), RightType right)
      res->add(left, right);
  return res;
}

protected function(mapping(LeftType:RightType)|this_program:
                   ADT.Relation.Binary) `+ = `|;

//! The expression @expr{rel1 - rel2@} returns a new relation which has
//! those and only those relation entries that are present in rel1
//! and not present in rel2.
protected ADT.Relation.Binary(<LeftType, RightType>) `-(this_program rel)
{
  return filter(lambda (LeftType left, RightType right)
                       { return !rel->contains(left, right);});
}

//! Makes the relation symmetric, i.e. makes sure that if xRy is part
//! of the relation set, then yRx should also be a part of the relation
//! set.
//!
//! @note
//!   This operation modifies the current object.
this_program(<LeftType|RightType, RightType|LeftType>) make_symmetric()
{
  foreach(indices(val), LeftType left)
    foreach(indices(val[left]), RightType right)
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
//! The argument @[avoiding] is either @expr{0@} (or omitted), or
//! a multiset of nodes that must not be part of the path.
array(LeftType|RightType)|zero find_shortest_path(LeftType|RightType from,
                                                  LeftType|RightType to,
                                                  multiset(LeftType|RightType) avoiding = (<>))
{
  if (avoiding[from] || avoiding[to])
     return 0;
  if (from == to)
     return ({ from });
  if (!val[from])
     return 0;
  if (contains(from, to))
     return ({ from, to });

  // NOTE: This is a simple, more or less depth-first search. Worst-
  // case time complexity could probably be improved, e.g. by a
  // breadth-first search (such as Dijkstra's path-finding algorithm).
  // But those algorithms typically have worse space complexity, so
  // this will do for now.

  array(LeftType|RightType)|zero subpath, found = 0;
  avoiding[from] = 1;
  foreach(indices(val[from]), LeftType right) {
    if (right == to) {
      found = ({ from, to });
      // We can't find a shorter path than this, so there's no
      // point in looking for more alternatives.
      break;
    }

    if (!avoiding[right])
      if (subpath = find_shortest_path(right, to, avoiding))
        if (!found || sizeof(subpath)+1 < sizeof(found))
        {
          found = ({ from, @subpath });
        }
  }
  avoiding[from] = 0;
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

//! Initialize a new @[ADT.Relation.Binary] object.
//!
//! @param id
//!   Identifier for the relation.
//!
//! @param initial
//!   Initial contents of the relation.
//!
//! @seealso
//!   @[get_id()]
protected void create(void|mixed id,
                      void|mapping(LeftType:RightType)|
                      this_program(<LeftType, RightType>) initial)
{
  this::id = id;
  if (objectp(initial) && initial->is_binary_relation)
    ([object]initial)->map(lambda (LeftType left, RightType right) {
      add(left, right);
      return 0;
    });
  else if (mappingp(initial))
    foreach([mapping]initial; LeftType left; RightType right)
      add(left, right);
  else if (initial)
    error("Bad initializer for ADT.Relation.Binary.\n");
}

//! An iterator which makes all the left/right entities in the relation
//! available as index/value pairs.
protected class _get_iterator (< LeftType = LeftType,
                                 RightType = RightType >)
{
  protected int(-1..) ipos = -1;
  protected int(-1..) vpos = -1;

  protected array(LeftType) lefts = indices(val);
  protected array(RightType) rights = ({});

  protected LeftType _iterator_index() {
    if (ipos < 0) return UNDEFINED;
    return lefts[ipos];
  }

  protected RightType _iterator_value() {
    if (ipos < 0) return UNDEFINED;
    return rights[vpos];
  }

  protected int(0..1) _iterator_next()
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

    return 1;
  }

  protected this_program(<LeftType, RightType>) `+=(int steps) {
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
