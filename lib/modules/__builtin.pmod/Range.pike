#pike __REAL_VERSION__
#pragma strict_types

//!   Generic lightweight range type.  Supports any values for lower
//!   and upper boundaries that implement the @expr{`<@} lfun.
//! @note
//!   Can only contain a single contiguous range.

//!  The lower inclusive boundary.
mixed from;

//!  The upper exclusive boundary.
mixed till;

//! @param from
//!   Lower inclusive boundary for the range.  Specify no lower-boundary
//!   by filling in @expr{-Math.inf@}.
//! @param till
//!   Upper exclusive boundary for the range.  Specify no upper-boundary
//!   by filling in @expr{Math.inf@}.
//! @seealso
//!   [Math.inf]
protected variant void create(mixed from, mixed till) {
  this::from = from;
  this::till = till;
}
protected variant void create(object/*this_program*/ copy) {
  from = copy->from;
  till = copy->till;
}
protected variant void create() {
}

//! Difference
protected mixed `-(mixed that) {
  this_program n = this_program(max(from, ([object]that)->from),
                                min(till, ([object]that)->till));
  if (n->from >= n->till)
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

protected int(0..1) `<(mixed that) {
  return from < ([object]that)->from
    || from == ([object]that)->from && till < ([object]that)->till;
}

protected int(0..1) `==(mixed that) {
  return objectp(that)
   && from == ([object]that)->from && till == ([object]that)->till;
}

//! Casting a range to string delivers an SQL-compliant value.
protected mixed cast(string to) {
  if (to == "string")
    return from >= till ? "empty" : sprintf("%c%s,%s)",
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
