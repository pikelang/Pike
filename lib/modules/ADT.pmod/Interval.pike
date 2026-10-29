// -*- mode: Pike; c-basic-offset: 4; -*-

//! An interval with independently open or closed endpoints.
//!
//! Plain endpoint values create a closed interval. To exclude an endpoint,
//! pass an @[Open] boundary instead; @[Closed] explicitly includes it.
//! Endpoint values must support ordering comparisons.
//!
//! @example
//!   ADT.Interval(<int>) i = ADT.Interval(<int>)(1, 4);
//!   i->contains(1);                 // 1
//!   i->contains(5);                 // 0
//!   sizeof(i);                     // 3: the distance between endpoints
//!   array endpoints = (array)i;     // ({ 1, 4 })
//!
//!   // Boundary classes belong to an interval instance.
//!   ADT.Interval(<int>) j = i->clone(i->Open(1), i->Closed(4));
//!   j->contains(1);                 // 0
//!   j->contains(4);                 // 1
//!
//! Intervals support intersection (@expr{&@}), combination
//! (@expr{|@} or @expr{+@}), and subtraction (@expr{-@}). The result must
//! be representable as a single interval; see the individual operators
//! for their restrictions. An empty result is represented by zero.
//!
//! @bugs
//!   Equal-coordinate boundary selection uses the same @[min()] and
//!   @[max()] rules for lower and upper endpoints. Consequently,
//!   @[contains()] can include an open upper endpoint, and combining two
//!   intervals with equal upper endpoint values can choose an open upper
//!   boundary even when one interval includes that endpoint. Intersection
//!   and subtraction inherit these endpoint limitations.
//!
//! @seealso
//!   @[ADT.CritBit.RangeSet]

#pike __REAL_VERSION__

//! Type of the endpoint values. Values must support @expr{<@} and
//! @expr{>@}; numeric endpoints also support @[sizeof()].
__generic__ mixed RangeType;

//! Base class for an endpoint. Use @[Open] or @[Closed] to specify
//! whether the endpoint is included in an interval.
//!
//! @param x
//!   The endpoint value, also available as the public member @expr{x@}.
//!
//! Boundary comparisons order endpoint values. An open boundary also
//! compares less than and greater than the equal plain value, allowing
//! it to exclude that value at either end of an interval.
class Boundary (< RangeType = RangeType >) (RangeType x) {
    //! Compare this endpoint value with a plain value or another boundary.
    //! The boundary kind is ignored by this base implementation.
    protected int (0..1) `<(RangeType|Boundary(<RangeType>) b) {
	if (!objectp(b) || !Program.inherits(object_program(b), Boundary)) {
	    return x < b;
	}
	return x < b->x;
    }

    //! Compare this endpoint value with a plain value or another boundary.
    //! The boundary kind is ignored by this base implementation.
    protected int (0..1) `>(RangeType|Boundary(<RangeType>) b) {
	if (!objectp(b) || !Program.inherits(object_program(b), Boundary)) {
	    return x > b;
	}
	return x > b->x;
    }

    //! Format the endpoint value using @expr{%O@}.
    protected string _sprintf(int type) {
	return sprintf("%O", x);
    }

    //! Return @expr{x->unix_time()@}. Requires an endpoint object providing
    //! that method, such as a Calendar time range.
    int unix_time() {
        return ([object](mixed)x)->unix_time();
    }

    //! Property alias for @[unix_time()].
    int `ux() {
	return unix_time();
    }

    //! Subtract another boundary's endpoint value from this one.
    //! Currently implemented for numeric endpoint values only; pass a
    //! boundary object as the argument.
    protected RangeType `-(RangeType|Boundary(<RangeType>) b) {
	if (intp(x) || floatp(x)) {
            return [object(RangeType)](mixed)
                (([int|float](mixed)x) - ([int|float](mixed)b->x));
	} else if (stringp(x)) {
	    error("lets see how we do this ;) \n");
	}
    }

    //! Return a boundary at the same value with the inclusion reversed.
    //! Implemented by @[Open] and @[Closed].
    protected Boundary (< RangeType >) `~();
}

//! An endpoint excluded from the interval.
//!
//! At equal coordinates, two open boundaries compare equal. Comparisons
//! with a plain value exclude equality; comparisons with another boundary
//! order their coordinates.
//!
//! @seealso
//!   @[Closed], @[Boundary]
class Open (< RangeType = RangeType >) {
    //! Inherit the endpoint value and common boundary operations.
    inherit Boundary (< RangeType >);

    //! Return true for another open boundary at the same value.
    protected int (0..1) `==(RangeType|Boundary(<RangeType>) b) {
	return objectp(b) && Program.inherits(object_program(b), Open)
		&& b->x == x;
    }

    //! Compare endpoint coordinates. Against a plain value, equality also
    //! returns true, so an open upper boundary excludes that value.
    protected int (0..1) `<(RangeType|Boundary(<RangeType>) b) {
	if (!objectp(b) || !Program.inherits(object_program(b), Boundary)) {
	    return x <= b;
	}
	return ::`<(b);
    }

    //! Compare endpoint coordinates. Against a plain value, equality also
    //! returns true, so an open lower boundary excludes that value.
    protected int (0..1) `>(RangeType|Boundary(<RangeType>) b) {
	if (!objectp(b) || !Program.inherits(object_program(b), Boundary)) {
	    return x >= b;
	}
	return ::`>(b);
    }

    //! Test this as an upper boundary against the lower boundary @[b].
    //! Returns true when this coordinate is greater than the other
    //! coordinate. Equal coordinates do not overlap when @[b] is a
    //! boundary; an equal plain value is accepted.
    int(0..1) overlaps(RangeType|Boundary(<RangeType>) b) {
	if (this > b) return 1;
	return 0;
    }

    //! Test this as an upper boundary against a lower boundary @[b].
    //! Returns true for an overlap, or equal coordinates when @[b] is closed.
    int(0..1) touches(Boundary(<RangeType>) b) {
	werror("%O touches %-O == %d\n", this, b, overlaps(b) || (this->x == b->x && Program.inherits(object_program(b), Closed)));
	return overlaps(b) || (this->x == b->x && Program.inherits(object_program(b), Closed));
    }

    //! Format with a parenthesis marking exclusion. The left-alignment
    //! flag selects a lower boundary, e.g. @expr{(1@}; otherwise the result
    //! is an upper boundary, e.g. @expr{1)@}.
    protected string _sprintf(int type, mapping info) {
	if (info->flag_left) {
	    return sprintf("(%O", x);
	}
	return sprintf("%O)", x);
    }

    //! Return a @[Closed] boundary at the same value.
    protected Boundary `~() {
	return Closed(x);
    }
}

//! An endpoint included in the interval.
//!
//! Two closed boundaries compare equal when their endpoint values are
//! equal. Ordering comparisons use the endpoint values.
//!
//! @seealso
//!   @[Open], @[Boundary]
class Closed (< RangeType = RangeType >) {
    //! Inherit the endpoint value and common boundary operations.
    inherit Boundary (< RangeType >);

    //! Return true for another closed boundary at the same value.
    protected int (0..1) `==(RangeType|Boundary(<RangeType>) b) {
	return objectp(b) && Program.inherits(object_program(b), Closed)
		&& b->x == x;
    }

    //! Test this as an upper boundary against a lower boundary @[b].
    //! At equal coordinates, returns true only when @[b] is also closed.
    int(0..1) overlaps(RangeType|Boundary(<RangeType>) b) {
	if (this > b || (Program.inherits(object_program(b), Closed) && b == this)) return 1;
	return 0;
    }

    //! Test this as an upper boundary against a lower boundary @[b].
    //! Returns true when this coordinate is greater than or equal to
    //! the other coordinate, regardless of its boundary kind.
    int(0..1) touches(RangeType|Boundary(<RangeType>) b) {
	werror("%O touches %-O == %d\n", this, b, this > b);
	if (this < b) return 0;
	return 1;
    }

    //! Format with a square bracket marking inclusion. The left-alignment
    //! flag selects a lower boundary, e.g. @expr{[1@}; otherwise the result
    //! is an upper boundary, e.g. @expr{1]@}.
    protected string _sprintf(int type, mapping info) {
	if (info->flag_left) {
	    return sprintf("[%O", x);
	}
	return sprintf("%O]", x);
    }

    //! Return an @[Open] boundary at the same value.
    protected Boundary (< RangeType >) `~() {
	return Open(x);
    }
}

//! Return the boundary with the smaller coordinate.
//! At equal coordinates, prefer a closed boundary over an open one.
//!
//! @seealso
//!   @[max()]
Boundary(<RangeType>) min(Boundary(<RangeType>) a, Boundary(<RangeType>) b) {
    if (a < b) return a;
    if (b < a) return b;

    if (Program.inherits(object_program(a), Closed)) return a;
    if (Program.inherits(object_program(b), Closed)) return b;
    return a;
}

//! Return the boundary with the larger coordinate.
//! At equal coordinates, prefer an open boundary over a closed one.
//!
//! @seealso
//!   @[min()]
Boundary(<RangeType>) max(Boundary(<RangeType>) a, Boundary(<RangeType>) b) {
    if (a == min(a,b)) return b;
    else return a;
}

//! The lower and upper boundaries, respectively. They retain both the
//! endpoint value and whether that endpoint is included.
//!
//! @note
//!   Direct assignment does not check that the interval remains nonempty.
//!
//! @seealso
//!   @[start], @[stop]
Boundary(<RangeType>) a, b;

//! The lower endpoint value, without its inclusion information.
//!
//! @seealso
//!   @[a], @[beginning()]
RangeType `->start() { return a->x; }

//! The upper endpoint value, without its inclusion information.
//!
//! @seealso
//!   @[b], @[end()]
RangeType `->stop() { return b->x; }

//! Set the lower endpoint and make it closed.
//! This assignment does not revalidate the ordering of the endpoints.
RangeType `->start=(RangeType v) {
    a = Closed(v);
    return v;
}

//! Set the upper endpoint and make it closed.
//! This assignment does not revalidate the ordering of the endpoints.
RangeType `->stop=(RangeType v) {
    b = Closed(v);
    return v;
}

//! Format the interval using brackets for closed boundaries and
//! parentheses for open ones, e.g. @expr{[1..4)@}.
protected string _sprintf(int type) {
    return sprintf("%-O..%O", a, b);
}

//! Create an interval from its lower and upper endpoints.
//!
//! @param a
//!   The lower value or boundary.
//! @param b
//!   The upper value or boundary.
//!
//! Plain values are wrapped in @[Closed] boundaries. The endpoints are
//! not reordered. An error is thrown if the upper boundary does not
//! overlap the lower one. Equal endpoint values are accepted only when
//! both boundaries are closed.
protected void create(RangeType|Boundary(<RangeType>) a,
                      RangeType|Boundary(<RangeType>) b) {
    if (!objectp(a) || !Program.inherits(object_program(a), Boundary)) {
	a = Closed(a);
    }
    if (!objectp(b) || !Program.inherits(object_program(b), Boundary)) {
	b = Closed(b);
    }
    if (!b->overlaps(a)) error("Trying to create empty interval.\n");
    this::a = a;
    this::b = b;
}

//! Return true when @[i] is an interval of this program with equal
//! endpoint values and matching open/closed boundary kinds.
protected int(0..1) `==(mixed i) {
    return objectp(i) && Program.inherits(object_program(i), this_program) && a == i->a && b == i->b;
}

//  0	(..)..[..]
//  1	(..[..)..]
//  2	[..(..)..]
//  3	[..(..]..)
//  4	[..]..(..)

//! Intersect this interval with @[i]. Return zero if they do not overlap;
//! otherwise return an interval whose bounds are @expr{max(a, i->a)@}
//! and @expr{min(b, i->b)@}.
//!
//! @note
//!   Equal-coordinate boundary selection follows @[min()] and @[max()],
//!   including their preference for closed and open boundaries.
//!
//! @seealso
//!   @[overlaps()]
protected this_program(<RangeType>)|zero `&(this_program(<RangeType>) i) {
    Boundary(<RangeType>) l, r;

    l = max(a, i->a);
    r = min(b, i->b);

    if (r->overlaps(l))
	return clone(l, r);
    return 0;
}

//! Return true if the intersection with @[i] is nonempty, using the same
//! boundary selection as @[lfun::`&()]. Two intervals meeting only at an
//! endpoint overlap when both include that endpoint.
int(0..1) overlaps(this_program(<RangeType>) i) {
    Boundary(<RangeType>) l, r;

    l = max(a, i->a);
    r = min(b, i->b);

    return r->overlaps(l);
}

//! Return true if the selected upper boundary touches the selected lower
//! boundary, including when the intervals overlap. At equal coordinates,
//! at least one of these boundaries must be closed.
//!
//! @seealso
//!   @[overlaps()], @[lfun::`|()]
int(0..1) touches(this_program(<RangeType>) i) {
    Boundary(<RangeType>) l, r;

    l = max(a, i->a);
    r = min(b, i->b);

    return r->touches(l);
}

//! Construct another interval of the same program from @[args], which
//! are passed to @[create()]. This does not copy the current endpoints;
//! supply both new endpoints explicitly.
this_program(<RangeType>) clone(mixed ... args) {
    return this_program(<RangeType>)(@args);
}


//! Combine this interval with @[i]. The result uses @expr{min(a, i->a)@}
//! and @expr{max(b, i->b)@} as its endpoints. An error is thrown if the
//! intervals neither overlap nor meet at an endpoint included by at least
//! one of them.
//!
//! @note
//!   Equal-coordinate boundary selection follows @[min()] and @[max()]
//!   for both endpoints.
//!
//! @seealso
//!   @[lfun::`+()], @[touches()]
protected this_program(<RangeType>) `|(this_program(<RangeType>) i) {
    if ((this & i)
    || (b->x <= i->a->x && b->touches(i->a))
    || (i->b->x <= a->x && i->b->touches(a))) {
	return clone(min(a, i->a), max(b, i->b));
    }

    error("%O and %O need to touch.\n", this, i);
}

//! Equivalent to @[lfun::`|()].
protected this_program(<RangeType>) `+(this_program(<RangeType>) i) {
    return this | i;
}

//! Subtract the intersection with @[interval]. Return zero if the
//! intersection equals this interval, or this object if there is no
//! intersection. Otherwise return the remaining interval with the cut
//! boundary's inclusion reversed.
//!
//! @throws
//!   Throws an error if the intersection reaches neither endpoint, since
//!   the result would require two separate intervals.
protected this_program(<RangeType>)|zero `-(this_program(<RangeType>) interval) {
    this_program(<RangeType>) i = interval & this;
    if (i) {
	if (i == this) return 0;

	if (a == i->a) {
	    return clone(~i->b, b);
	} else if (b == i->b) {
	    return clone(a, ~i->a);
	}

	error("%O and %O may not be contained.\n", this, i);
    }
    return this;
}

//! Return the distance between the endpoint values, @expr{b - a@}.
//! This is not the number of contained integers and does not depend on
//! whether the endpoints are open or closed. Currently requires numeric
//! endpoint values.
protected int|float _sizeof() {
    return b-a;
}

//! Test membership of a value, or intersection with another interval.
//!
//! For a plain value, test whether its closed, single-point interval
//! intersects this interval. The endpoint limitations described in
//! @[ADT.Interval] apply, including at an open upper endpoint.
//!
//! @note
//!   When @[x] is an interval, this method returns true for any nonempty
//!   intersection, just like @[overlaps()]. It does not test whether the
//!   whole argument interval is contained.
int(0..1) contains(RangeType|this_program(<RangeType>) x) {
    if (!objectp(x) || !Program.inherits(object_program(x), ADT.Interval)) {
	x = this_program(Closed(x), Closed(x));
    }
    return !!(this&x);
}

/* TODO:
 * implement or remap the api offered by the timerange thing.
 */

//! Return @[start].
//!
//! @seealso
//!   @[end()]
RangeType beginning() { return start; }

//! Return @[stop].
//!
//! @seealso
//!   @[beginning()]
RangeType end() { return stop; }

//! Cast to @expr{"array"@} to obtain @expr{({ start, stop })@}.
//! The result contains endpoint values only; open/closed information is
//! not preserved.
protected mixed cast(string type) {
  if( type=="array" )
    return ({ start, stop });
  return UNDEFINED;
}
