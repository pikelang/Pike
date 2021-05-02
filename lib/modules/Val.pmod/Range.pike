#pike __REAL_VERSION__
#pragma strict_types

//!   Generic lightweight range type.  Supports any values for lower
//!   and upper boundaries that implement @[lfun::`<()] and @[lfun::`-@()],
//!   and preferrably also cast to @expr{int@} and @expr{string@}.
//! @note
//!   Can only contain a single contiguous range.
//! @note
//!   The empty range must be stored as @expr{(Math.inf, -Math.inf)@}
//!   if assigned directly to @[from] and @[till].

constant is_range = 1;

protected typedef int|float|string|object value_type;

//!  The lower inclusive boundary.
value_type from;

//!  The upper exclusive boundary.
value_type till;

array(value_type) _encode() {
  return ({from, till});
}

void _decode(array(value_type) x) {
  from = x[0];
  till = x[1];
}

protected int __hash() {
  catch {
    return (int)from ^ (int)till;
  };
  return 0;
}

//! @param from
//!   Lower inclusive boundary for the range.  Specify no lower-boundary
//!   by filling in @expr{-Math.inf@}.
//! @param till
//!   Upper exclusive boundary for the range.  Specify no upper-boundary
//!   by filling in @expr{Math.inf@}.
//! @seealso
//!   @[Math.inf]
protected variant void create(value_type from, value_type till) {
  if (from >= till) {
    from = Math.inf;
    till = -Math.inf;
  }
  this::from = from;
  this::till = till;
}
protected variant void create(this_program copy) {
  from = copy->from;
  till = copy->till;
}
protected variant void create() {
}

//! Difference
protected this_program `-(this_program that) {
  this_program n = this_program(max(from, that->from),
				min(till, that->till));
  if (!n)
    return this;
  if (till == n->till) {
    n->till = n->from;
    n->from = from;
    return n;
  }
  if (from == n->from) {
    n->from = n->till;
    n->till = till;
    return n;
  }
  error("Result of range difference would not be contiguous\n");
}

//! Union
protected this_program `+(this_program that) {
  if (from != that->till && that->from != till
      && !(this & that))
    error("Result of range union would not be contiguous\n");
  return this_program(min(from, that->from),
		      max(till, that->till));
}

//! Intersection
protected this_program `*(this_program that) {
  return this_program(max(from, that->from),
                      min(till, that->till));
}

//! Overlap: have points in common.
protected int(0..1) `&(this_program that) {
  return till > that->from && that->till > from;
}

//! Is adjacent to
//!
//! @fixme
//!   This does NOT seem like a good operator for this operation.
protected int(0..1) `|(this_program that) {
  return till == that->from || from == that->till;
}

//! Strictly left of
protected int(0..1) `<<(this_program that) {
  return till <= that->from;
}

//! Strictly right of
protected int(0..1) `>>(this_program that) {
  return from >= that->till;
}

protected int(0..1) `<(mixed that) {
  return from < ([object]that)->from
    || from == ([object]that)->from && till < ([object]that)->till;
}

protected int(0..1) `==(mixed that) {
  return objectp(that) && ([object]that)->is_range
   && from == ([object]that)->from && till == ([object]that)->till;
}

//! @returns
//!   True if range is empty.
//!
//! @seealso
//!   @[isempty()]
protected inline int(0..1) `!() {
  return from >= till;
}

//! @returns
//!   True if range is empty.
//!
//! @seealso
//!   @[`!()]
inline int(0..1) isempty() {
  return !this;
}

//! @param other
//!  Extends the current range to the smallest range which encompasses
//!  itself and all other given ranges.
//!
//! @fixme
//!   This seems like the operation that @expr{`|()@} ought to do.
this_program merge(this_program ... other) {
  from = [object(value_type)]min(from, @other->from);
  till = [object(value_type)]max(till, @other->till);
  return this;
}

//! @returns
//!  True if this range fully contains another range or element.
int(0..1) contains(object(this_program)|value_type other) {
  return objectp(other) && ([object]other)->is_range
    ? from <= ([object]other)->from && ([object]other)->till <= till
    : from <= other && other < till;
}

//! @returns
//!   Calculates the value of the interval: @expr{till - from@}.
//!   Returns @expr{0@} if the range is empty.
mixed interval() {
  return !this ? 0 : till - from;
}

//! @returns
//!  A string representing the range using SQL-compliant syntax.
final string sql() {
  return !this ? "empty" : sprintf("%c%s,%s)",
    from == -Math.inf ? '(' : '[', from == -Math.inf ? "" : (string)from,
    till == Math.inf ? "" : (string)till);
}

public string encode_json() {
  return sprintf("\"%s\"", this);
}

protected mixed cast(string to) {
  if (to!="string") return UNDEFINED;
  if (!this) return "[]";
  return sprintf("[%s..%s]",
                 from != -Math.inf ? (string)from : "",
                 till != Math.inf ? (string)till : "");
}

protected string _sprintf(int fmt, mapping(string:mixed) params) {
  if (!this)					// Not in destructed objects
    return "(destructed)";
  switch (fmt) {
    case 'O': return sprintf("this_program( %s )", (string)this);
    case 's': return (string)this;
  }
  return sprintf(sprintf("%%*%c", fmt), params, 0);
}
