#pike __REAL_VERSION__
#pragma strict_types

//!   Generic lightweight range type.  Supports any values for lower
//!   and upper boundaries that implement the @expr{`<@} lfun.
//! @note
//!   Can only contain a single contiguous range.
//! @note
//!   The empty range must be stored as @expr{(Math.inf, -Math.inf)@}
//!   if assigned directly to @[from] and @[till].

constant is_range = 1;

//!  The lower inclusive boundary.
mixed from;

//!  The upper exclusive boundary.
mixed till;

array(mixed) _encode() {
  return ({from, till});
}

void _decode(array(mixed) x) {
  from = x[0];
  till = x[1];
}

//! @param from
//!   Lower inclusive boundary for the range.  Specify no lower-boundary
//!   by filling in @expr{-Math.inf@}.
//! @param till
//!   Upper exclusive boundary for the range.  Specify no upper-boundary
//!   by filling in @expr{Math.inf@}.
//! @seealso
//!   @[Math.inf]
protected variant void create(mixed from, mixed till) {
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
protected mixed `-(mixed that) {
  this_program n = this_program(max(from, ([object]that)->from),
                                min(till, ([object]that)->till));
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
protected mixed `+(mixed that) {
  if (from != ([object]that)->till && ([object]that)->from != till
      && !(this & ([object]that)))
    error("Result of range union would not be contiguous\n");
  return this_program(min(from, ([object]that)->from),
                      max(till, ([object]that)->till));
}

//! Intersection
protected mixed `*(mixed that) {
  return this_program(max(from, ([object]that)->from),
                      min(till, ([object]that)->till));
}

//! Overlap: have points in common.
protected int(0..1) `&(mixed that) {
  return till > ([object]that)->from && ([object]that)->till > from;
}

//! Is adjacent to
protected int(0..1) `|(mixed that) {
  return till == ([object]that)->from || from == ([object]that)->till;
}

//! Strictly left of
protected int(0..1) `<<(mixed that) {
  return till <= ([object]that)->from;
}

//! Strictly right of
protected int(0..1) `>>(mixed that) {
  return from >= ([object]that)->till;
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
protected inline int(0..1) `!() {
  return from >= till;
}

//! @returns
//!   True if range is empty.
inline int(0..1) isempty() {
  return !this;
}

//! @param other
//!  Extends the current range to the smallest range which encompasses
//!  itself and all other given ranges.
this_program merge(this_program ... other) {
  from = min(from, @other->from);
  till = max(till, @other->till);
  return this;
}

//! @returns
//!  True if this range fully contains another range or element.
int(0..1) contains(mixed other) {
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

//! Casting a range to string delivers an SQL-compliant value.
protected mixed cast(string to) {
  if (to == "string")
    return !this ? "empty" : sprintf("%c%s,%s)",
      from == -Math.inf ? '(' : '[', from == -Math.inf ? "" : (string)from,
      till == Math.inf ? "" : (string)till);
  return UNDEFINED;
}

protected string _sprintf(int fmt, mapping(string:mixed) params) {
  switch (fmt) {
    case 'O': return sprintf("this_program( %s )", (string)this);
    case 's': return (string)this;
  }
  return sprintf(sprintf("%%*%c", fmt), params, 0);
}
