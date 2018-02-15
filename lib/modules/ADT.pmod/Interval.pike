#pike __REAL_VERSION__

class Boundary(mixed x) {
    int (0..1) `<(object b) {
	if (!objectp(b) || !Program.inherits(object_program(b), Boundary)) {
	    return x < b;
	}
	return x < b->x;
    }

    int (0..1) `>(object b) {
	if (!objectp(b) || !Program.inherits(object_program(b), Boundary)) {
	    return x > b;
	}
	return x > b->x;
    }

    string _sprintf(int type) {
	return sprintf("%O", x);
    }

    int unix_time() {
	return x->unix_time();
    }

    int `ux() {
	return unix_time();
    }

    mixed `-(object b) {
	if (intp(x) || floatp(x)) {
	    return x - b->x;
	} else if (stringp(x)) {
	    error("lets see how we do this ;) \n");
	}
    }
}

class Open {
    inherit Boundary;

    int (0..1) `==(object b) {
	return objectp(b) && Program.inherits(object_program(b), Open)
		&& b->x == x;
    }

    int (0..1) `<(mixed b) {
	if (!objectp(b) || !Program.inherits(object_program(b), Boundary)) {
	    return x <= b;
	}
	return ::`<(b);
    }

    int (0..1) `>(mixed b) {
	if (!objectp(b) || !Program.inherits(object_program(b), Boundary)) {
	    return x >= b;
	}
	return ::`>(b);
    }

    int(0..1) overlaps(object b) {
	if (this > b) return 1;
	return 0;
    }

    int(0..1) touches(object b) {
	werror("%O touches %-O == %d\n", this, b, overlaps(b) || (this->x == b->x && Program.inherits(object_program(b), Closed)));
	return overlaps(b) || (this->x == b->x && Program.inherits(object_program(b), Closed));
    }

    string _sprintf(int type, mapping info) {
	if (info->flag_left) {
	    return sprintf("(%O", x);
	}
	return sprintf("%O)", x);
    }

    Boundary `~() {
	return Closed(x);
    }
}

class Closed {
    inherit Boundary;

    int (0..1) `==(object b) {
	return objectp(b) && Program.inherits(object_program(b), Closed)
		&& b->x == x;
    }

    int(0..1) overlaps(object b) {
	if (this > b || (Program.inherits(object_program(b), Closed) && b == this)) return 1;
	return 0;
    }

    int(0..1) touches(object b) {
	werror("%O touches %-O == %d\n", this, b, this > b);
	if (this < b) return 0;
	return 1;
    }
    string _sprintf(int type, mapping info) {
	if (info->flag_left) {
	    return sprintf("[%O", x);
	}
	return sprintf("%O]", x);
    }

    Boundary `~() {
	return Open(x);
    }
}

Boundary min(Boundary a, Boundary b) {
    if (a < b) return a;
    if (b < a) return b;

    if (Program.inherits(object_program(a), Closed)) return a;
    if (Program.inherits(object_program(b), Closed)) return b;
    return a;
}

Boundary max(Boundary a, Boundary b) {
    if (a == min(a,b)) return b;
    else return a;
}

Boundary a, b;

mixed `->start() { return a->x; }
mixed `->stop() { return b->x; }

mixed `->start=(mixed v) {
    a = Closed(v);
    return v;
}
mixed `->stop=(mixed v) {
    b = Closed(v);
    return v;
}

string _sprintf(int type) {
    return sprintf("%-O..%O", a, b);
}

void create(mixed a, mixed b) {
    if (!objectp(a) || !Program.inherits(object_program(a), Boundary)) {
	a = Closed(a);
    }
    if (!objectp(b) || !Program.inherits(object_program(b), Boundary)) {
	b = Closed(b);
    }
    if (!b->overlaps(a)) error("Trying to create empty interval.\n");
    this_program::a = a;
    this_program::b = b;
}

int(0..1) `==(mixed i) {
    return objectp(i) && Program.inherits(object_program(i), this_program) && a == i->a && b == i->b;
}

//  0	(..)..[..]
//  1	(..[..)..]
//  2	[..(..)..]
//  3	[..(..]..)
//  4	[..]..(..)

this_program `&(this_program i) {
    Boundary l, r;

    l = max(a, i->a);
    r = min(b, i->b);

    if (r->overlaps(l))
	return clone(l, r);
    return 0;
}

int(0..1) overlaps(this_program i) {
    Boundary l, r;

    l = max(a, i->a);
    r = min(b, i->b);

    return r->overlaps(l);
}

int(0..1) touches(this_program i) {
    Boundary l, r;

    l = max(a, i->a);
    r = min(b, i->b);

    return r->touches(l);
}

this_program clone(mixed ... args) {
    return this_program(@args);
}


this_program `|(this_program i) {
    if ((this & i)
    || (b->x <= i->a->x && b->touches(i->a))
    || (i->b->x <= a->x && i->b->touches(a))) {
	return clone(min(a, i->a), max(b, i->b));
    }

    error("%O and %O need to touch.\n", this, i);
}

this_program `+(this_program i) {
    return this | i;
}

this_program `-(this_program interval) {
    this_program i = interval & this;
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

int|float _sizeof() {
    return b-a;
}

int(0..1) contains(mixed x) {
    if (!objectp(x) || !Program.inherits(object_program(x), ADT.Interval)) {
	x = this_program(Closed(x), Closed(x));
    }
    return !!(this&x);
}

/* TODO:
 * implement or remap the api offered by the timerange thing.
 */

mixed beginning() { return start; }
mixed end() { return stop; }

mixed cast(string type) {
    switch (type) {
    case "array":
	return ({ start, stop });
    }
}
