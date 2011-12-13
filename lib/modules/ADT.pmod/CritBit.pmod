#pike __REAL_VERSION__

//! @ignore
#if !constant (ADT._CritBit)
constant this_program_does_not_exist = 1;
#else

inherit ADT._CritBit;
//! @endignore

//! Creates a CritBit tree for keys of type @expr{type@}. If no argument is given,
//! an instance of @[ADT.CritBit.StringTree] is returned. Supported types are @expr{"string"@},
//! @expr{"int"@}, @expr{"float"@}, @expr{"ipv4"@} and @expr{Calendar.TimeRange@}.
object Tree(void|string|program|mapping type) {
    switch (type) {
    case Calendar.TimeRange:
	return DateTree();
    case "int": 
	return IntTree();
    case "ipv4":
	return IPv4Tree();
    case "string":
    case 0:
	return StringTree();
    case "float":
	return FloatTree();
    default:
	if (mappingp(type))
	    return StringTree(type);

	error("No tree available for type %O\n", type);
    }
}

string iptosortable(string ip) {
    int a, b, c, d, e, i;
    string s;

    switch (sscanf(ip, "%d.%d.%d.%d/%d", a, b, c, d, e)) {
    case 4:
	e = 32;
    case 5:
	e = max(0, min(32, e));
	s = sprintf("%c%c%c%c", a, b, c, d);
	sscanf(s, "%4c", i);
	i &= ~(0xffffffff >> e);
	s = sprintf("%4c%c%c%c%c%c", i, a, b, c, d, e);

	break;
    default:
	error("NON-ip %O got passed to iptoint.\n", ip);
    }


    return s;
}

//! Sorts an ARRAY OF IPv4-Adresses (and optional netmasks) given in dotted
//! decimal representation with the /23 netmask notation.
//!
//! @example
//! 	@code
//!> array(string) a = ({ "127.0.0.121",
//!>	 "127.0.0.0/16",
//!>	 "127.0.0.1/8",
//!>	 "127.0.0.0/8",
//!>	 "128.0.0.0/1",
//!>	 "192.168.21.3",
//!>	 "8.8.8.8" });
//!> write("%O\n", CritBit.sort_ipv4(a));
//!({ /* 7 elements */
//!	"8.8.8.8",
//!	"127.0.0.0/8",
//! 	"127.0.0.0/16",
//! 	"127.0.0.1/8",
//! 	"127.0.0.121",
//! 	"128.0.0.0/1",
//! 	"192.168.21.3"
//!})
//! 	@endcode
//! @
array(string) sort_ipv4(array(string) a, array ... data) {
    array b = map(a, iptosortable);

    sort(b, a, @data);

    return a;
}

string get_ipv4(int ip, void|int prefix) {
    string ret;

    if (undefinedp(prefix)) prefix = 32;
    prefix = min(32, max(prefix, 0));
    ip &= ~(0xffffffff >> prefix);
    ret = sprintf("%d.%d.%d.%d",
		  (ip & 0xff000000) >> 24,
		  (ip & 0xff0000) >> 16,
		  (ip & 0xff00) >> 8,
		  (ip & 0xff));
    if (prefix != 32) {
	ret += sprintf("/%d", prefix);
    }

    return ret;
}

//! This module offers CritBit tree implementations for different
//! key types.
//! @note
//! 	These CritBit trees support prefixes as proper keys. Hence
//! 	they should really be called Tries.

//!
class DateTree {
    inherit IntTree;
    mapping(int:Calendar.TimeRange) backwards = ([]);

    //! Encodes a Calendar.TimeRange object into unix timestanp.
    int encode_key(object|int o) {
	if (objectp(o) && Program.inherits(object_program(o),
					   Calendar.TimeRange)) {
	    int t = o->unix_time();
	    backwards[t] = o;
	    return t;
	}
	return o;
    }

    //! Decodes an integer back to a @[Calendar.TimeRange] object. Keeps
    //! a mapping of all keys stored in the tree to transform back.
    int|object decode_key(int i) {
	return has_index(backwards, i)
	    ? backwards[i]
	    : Calendar.Second("unix", i);
    }

    //! Copy callback to also clone backwards
    this_program copy() {
	this_program t = ::copy();
	t->backwards = copy_value(backwards);
	return t;
    }

    mixed _m_delete(mixed o) {
	mixed v = ::_m_delete(o);
	m_delete(backwards, decode_key(o));
	return v;
    }
}

//! Data structure representing a set of disjunct @[ADT.Interval] objects. Can be thought
//! of as an interval with gaps.
class RangeSet {
    object tree;

    //! Create a RangeSet from a given tree program.
    void create(function|object|program tree) {
	this_program::tree = objectp(tree) ? tree : tree();
    }

    mixed `[](mixed key) {
	if (objectp(key) && Program.inherits(object_program(key), ADT.Interval))
	    return contains(key);
	mixed next = tree[key];
	if (!undefinedp(next)) return next->contains(key);
	next = tree->next(key);

	if (next) {
	    object interval = tree[next];
	    return interval->contains(key);
	}

	return UNDEFINED;
    }

    int(0..1) overlaps(mixed o) {
	if (objectp(o)) {
	    if (Program.inherits(object_program(o), ADT.Interval)) {
		ADT.Interval r = tree[o->b->x];
		if (!r) {
		    mixed next = tree->next(o->b->x);
		    if (next) r = tree[next];
		}
		if (!r) return 0;
		return r->overlaps(o);
	    } else if (Program.inherits(object_program(o), this_program)) {
		// TODO
	    }
	}
	return 0;
    }

    int(0..1) contains(mixed o) {
	if (objectp(o)) {
	    if (Program.inherits(object_program(o), ADT.Interval)) {
		ADT.Interval r = tree[o->b->x];
		if (!r) {
		    mixed next = tree->next(o->b->x);
		    if (next) r = tree[next];
		}
		if (!r) return 0;
		return r->contains(o);
	    } else if (Program.inherits(object_program(o), this_program)) {
		// TODO
	    }
	}
	return 0;
    }

    mixed `[]=(mixed i, mixed v) {
	if (objectp(i) && Program.inherits(object_program(i), ADT.Interval)) {
	    merge(i);
	    return v;
	} else error("Bad key type: %t. Expects an instance of type ADT.Interval.\n", i);
    }

    array(mixed) _indices() {
	return values(tree);
    }

    void merge(mixed i) {
	werror("merge(%O)\n", i);
	array a = values(tree[i->a->x..i->b->x]);
	mixed next = tree->next(i->b->x);

	if (!undefinedp(next)) a += ({ tree[next] });

	werror("all: %O\n", a);
	a = filter(a, i->touches);
	werror("filtered: %O\n", a);

	if (sizeof(a)) {
	    i |= a[0];
	    if (sizeof(a) > 1)
		i |= a[sizeof(a)-1];
	    foreach (a;; mixed b) m_delete(tree, b->b->x);
	}

	tree[i->b->x] = i;
    }

    this_program `|(object o) {
	this_program new = this_program(tree->copy());
	if (Program.inherits(object_program(o), this_program)) {
	    foreach (indices(o); ; mixed i) new->merge(i);
	} else {
	    new->merge(o);
	}
	return new;
    }

    string _sprintf(int type) {
	if (type == 'O') {
	    array a = indices(this);
	    return sprintf("{%s}", map(a, Function.curry(sprintf)("%O"))*", ");
	}

	return 0;
    }
}

class MultiRangeSet {
    // mapping from left/right boundary to interval
    object tree;
    int|float max_len;

    void create(function|program|object tree) {
	this_program::tree = objectp(tree) ? tree : tree();
    }

    mixed `[](mixed key) {

	if (!objectp(key) || !Program.inherits(object_program(key),
					       ADT.Interval)) {
	    key = ADT.Interval(key, key);
	}

	return contains(key);
    }

    mixed contains(mixed key) {
	array ret = ({});

	foreach (tree->Iterator(tree, -1, key->b->x);
		 ; object i) {
	    if (key->a - i[0]->a > max_len) break;
	    ret += filter(i, i->contains(key));
	}

	return ret;
    }

    mixed overlaps(mixed key) {
	array ret = ({});

	foreach (tree->Iterator(tree, -1, key->b->x);
		 ; array(object) i) {
	    if (key->a - i[0]->a > max_len) {
		werror("stopping early.\n");
		break;
	    }

	    ret += filter(i, i->overlaps(key));
	}

	return ret;
    }

    mixed `[]=(mixed i, mixed v) {
	max_len = max(sizeof(i), max_len);
	if (v) i->val = v;
	if (tree[i->a->x])
	    tree[i->a->x] += ({ i });
	else tree[i->a->x] = ({ i });
    }

    string _sprintf(int type) {
	return sprintf("%O", values(tree));
    }
}

#endif	// constant (@module@)
