// -*- mode: Pike; c-basic-offset: 4; -*-

#pike __REAL_VERSION__

//! Type for the limits of the interval.
__generic__ mixed RangeType;

//!
class Boundary (< RangeType = RangeType >) (RangeType x) {
    //!
    protected int (0..1) `<(RangeType|Boundary(<RangeType>) b) {
	if (!objectp(b) || !Program.inherits(object_program(b), Boundary)) {
	    return x < b;
	}
	return x < b->x;
    }

    //!
    protected int (0..1) `>(RangeType|Boundary(<RangeType>) b) {
	if (!objectp(b) || !Program.inherits(object_program(b), Boundary)) {
	    return x > b;
	}
	return x > b->x;
    }

    //!
    protected string _sprintf(int type) {
	return sprintf("%O", x);
    }

    //!
    int unix_time() {
        return ([object](mixed)x)->unix_time();
    }

    //!
    int `ux() {
	return unix_time();
    }

    //!
    protected RangeType `-(RangeType|Boundary(<RangeType>) b) {
	if (intp(x) || floatp(x)) {
            return [object(RangeType)](mixed)
                (([int|float](mixed)x) - ([int|float](mixed)b->x));
	} else if (stringp(x)) {
	    error("lets see how we do this ;) \n");
	}
    }

    //!
    protected Boundary (< RangeType >) `~();
}

//!
class Open (< RangeType = RangeType >) {
    //!
    inherit Boundary (< RangeType >);

    //!
    protected int (0..1) `==(RangeType|Boundary(<RangeType>) b) {
	return objectp(b) && Program.inherits(object_program(b), Open)
		&& b->x == x;
    }

    //!
    protected int (0..1) `<(RangeType|Boundary(<RangeType>) b) {
	if (!objectp(b) || !Program.inherits(object_program(b), Boundary)) {
	    return x <= b;
	}
	return ::`<(b);
    }

    //!
    protected int (0..1) `>(RangeType|Boundary(<RangeType>) b) {
	if (!objectp(b) || !Program.inherits(object_program(b), Boundary)) {
	    return x >= b;
	}
	return ::`>(b);
    }

    //!
    int(0..1) overlaps(RangeType|Boundary(<RangeType>) b) {
	if (this > b) return 1;
	return 0;
    }

    //!
    int(0..1) touches(Boundary(<RangeType>) b) {
	werror("%O touches %-O == %d\n", this, b, overlaps(b) || (this->x == b->x && Program.inherits(object_program(b), Closed)));
	return overlaps(b) || (this->x == b->x && Program.inherits(object_program(b), Closed));
    }

    //!
    protected string _sprintf(int type, mapping info) {
	if (info->flag_left) {
	    return sprintf("(%O", x);
	}
	return sprintf("%O)", x);
    }

    //!
    protected Boundary `~() {
	return Closed(x);
    }
}

//!
class Closed (< RangeType = RangeType >) {
    //!
    inherit Boundary (< RangeType >);

    //!
    protected int (0..1) `==(RangeType|Boundary(<RangeType>) b) {
	return objectp(b) && Program.inherits(object_program(b), Closed)
		&& b->x == x;
    }

    //!
    int(0..1) overlaps(RangeType|Boundary(<RangeType>) b) {
	if (this > b || (Program.inherits(object_program(b), Closed) && b == this)) return 1;
	return 0;
    }

    //!
    int(0..1) touches(RangeType|Boundary(<RangeType>) b) {
	werror("%O touches %-O == %d\n", this, b, this > b);
	if (this < b) return 0;
	return 1;
    }

    //!
    protected string _sprintf(int type, mapping info) {
	if (info->flag_left) {
	    return sprintf("[%O", x);
	}
	return sprintf("%O]", x);
    }

    //!
    protected Boundary (< RangeType >) `~() {
	return Open(x);
    }
}

//!
Boundary(<RangeType>) min(Boundary(<RangeType>) a, Boundary(<RangeType>) b) {
    if (a < b) return a;
    if (b < a) return b;

    if (Program.inherits(object_program(a), Closed)) return a;
    if (Program.inherits(object_program(b), Closed)) return b;
    return a;
}

//!
Boundary(<RangeType>) max(Boundary(<RangeType>) a, Boundary(<RangeType>) b) {
    if (a == min(a,b)) return b;
    else return a;
}

//!
Boundary(<RangeType>) a, b;

//!
RangeType `->start() { return a->x; }

//!
RangeType `->stop() { return b->x; }

//!
RangeType `->start=(RangeType v) {
    a = Closed(v);
    return v;
}

//!
RangeType `->stop=(RangeType v) {
    b = Closed(v);
    return v;
}

//!
protected string _sprintf(int type) {
    return sprintf("%-O..%O", a, b);
}

//!
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

//!
protected int(0..1) `==(mixed i) {
    return objectp(i) && Program.inherits(object_program(i), this_program) && a == i->a && b == i->b;
}

//  0	(..)..[..]
//  1	(..[..)..]
//  2	[..(..)..]
//  3	[..(..]..)
//  4	[..]..(..)

//!
protected this_program(<RangeType>)|zero `&(this_program(<RangeType>) i) {
    Boundary(<RangeType>) l, r;

    l = max(a, i->a);
    r = min(b, i->b);

    if (r->overlaps(l))
	return clone(l, r);
    return 0;
}

//!
int(0..1) overlaps(this_program(<RangeType>) i) {
    Boundary(<RangeType>) l, r;

    l = max(a, i->a);
    r = min(b, i->b);

    return r->overlaps(l);
}

//!
int(0..1) touches(this_program(<RangeType>) i) {
    Boundary(<RangeType>) l, r;

    l = max(a, i->a);
    r = min(b, i->b);

    return r->touches(l);
}

//!
this_program(<RangeType>) clone(mixed ... args) {
    return this_program(<RangeType>)(@args);
}


//!
protected this_program(<RangeType>) `|(this_program(<RangeType>) i) {
    if ((this & i)
    || (b->x <= i->a->x && b->touches(i->a))
    || (i->b->x <= a->x && i->b->touches(a))) {
	return clone(min(a, i->a), max(b, i->b));
    }

    error("%O and %O need to touch.\n", this, i);
}

//!
protected this_program(<RangeType>) `+(this_program(<RangeType>) i) {
    return this | i;
}

//!
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

//!
protected int|float _sizeof() {
    return b-a;
}

//!
int(0..1) contains(RangeType|this_program(<RangeType>) x) {
    if (!objectp(x) || !Program.inherits(object_program(x), ADT.Interval)) {
	x = this_program(Closed(x), Closed(x));
    }
    return !!(this&x);
}

/* TODO:
 * implement or remap the api offered by the timerange thing.
 */

//!
RangeType beginning() { return start; }

//!
RangeType end() { return stop; }

protected mixed cast(string type) {
  if( type=="array" )
    return ({ start, stop });
  return UNDEFINED;
}
