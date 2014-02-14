#pike __REAL_VERSION__
#require constant(ADT._CritBit)

//! @ignore
inherit ADT._CritBit : C;
//! @endignore

//! Creates a CritBit tree for keys of type @expr{type@}. If no argument is given,
//! an instance of @[ADT.CritBit.StringTree] is returned. Supported types are @expr{"string"@},
//! @expr{"int"@}, @expr{"float"@}, @expr{"ipv4"@} and @expr{Calendar.TimeRange@}.
object Tree(void|string|program|mapping type) {
    if (type==Calendar.TimeRange)
      return DateTree();
    switch (type) {
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
        e = limit(0, e, 32);
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
    prefix = limit(0, prefix, 32);
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
    inherit C::IntTree;
    mapping(int:Calendar.TimeRange) backwards = ([]);

    //! Encodes a Calendar.TimeRange object into unix timestanp.
    int encode_key(object|int o) {
	if (objectp(o) && Program.inherits(object_program(o),
					   Calendar.TimeRange)) {
	    int t = o->unix_time();
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

    mixed `[]=(mixed key, mixed v) {
	if (objectp(key)) {
	    int k = encode_key(key);
	    backwards[k] = key;
	}

	return ::`[]=(key, v);
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

class Reverse(object tree) {
    mixed `[](mixed k) {
	return tree[k];
    }

    mixed `[]=(mixed k, mixed v) {
	return tree[k] = v;
    }

    mixed next(mixed k) {
	return tree->previous(k);
    }

    mixed previous(mixed k) {
	return tree->next(k);
    }

    mixed first() {
	return tree->last();
    }

    mixed last() {
	return tree->first();
    }

    object _get_iterator(void|int step, void|mixed start, void|mixed stop) {
	return tree->_get_iterator(-1*(step||1), stop, start);
    }

    mixed `[..](mixed a, int atype, mixed b, int btype) {
	return tree->`[..](b, btype, a, atype);
    }

    array _values() {
	return reverse(values(tree));
    }

    array _indices() {
	return reverse(indices(tree));
    }

    object copy() {
	return this_program(tree->copy());
    }

    object get_subtree(mixed k) {
	return this_program(tree->subtree(k));
    }

    int(0..1) _equal(mixed other) {
	if (!(objectp(other) && Program.inherits(object_program(other), this_program)))
	    return 0;
	return equal(tree, other->tree);
    }

    mixed _m_delete(mixed idx) {
	return m_delete(tree, idx);
    }

    mixed nth(int n) {
	return tree->nth(sizeof(tree)-n);
    }

    mixed _random() {
	return random(tree);
    }

    int(0..) depth() {
	return tree->depth();
    }

    mixed common_prefix() {
	return tree->common_prefix();
    }

    mixed cast(string type) {
	return tree->cast(type);
    }

    this_program `+(mixed o) {
	if (objectp(o)) {
	    if (Program.inherits(object_program(o), this_program))
		return this_program(tree + o->tree);
	    else if (Program.inherits(object_program(o), object_program(tree)))
		return this_program(tree + o);
	}
	error("Bad argument 1 to `+()\n");
    }

    this_program `-(mixed o) {
	if (objectp(o)) {
	    if (Program.inherits(object_program(o), this_program))
		return this_program(tree - o->tree);
	    else if (Program.inherits(object_program(o), object_program(tree)))
		return this_program(tree - o);
	}
	error("Bad argument 1 to `-()\n");
    }

    int(0..) _sizeof() {
	return sizeof(tree);
    }
}

class MultiTree {
    array(object) trees;
    int itree(mixed k);

    object tree(mixed k) {
	int i = itree(k);
	if (i == -1) error("Indexing %O with wrong type %O\n", this, k);
	return trees[i];
    }

    void create(array|mapping|void init) {
	if (mappingp(init)) {
	    foreach (init; int i; mixed v) {
		tree(i)[i] = v;
	    }
	} else if (arrayp(init)) {
	    for (int j = 0; j < sizeof(init); j+=2) {
		tree(init[j])[init[j]] = init[j+1];
	    }
	}
    }

    mixed `[](mixed i) {
	return tree(i)[i];
    }

    mixed `[]=(mixed i, mixed m) {
	return tree(i)[i] = m;
    }

    this_program `[..](mixed a, mixed atype, mixed b, mixed btype) {
	this_program ret = this_program();
	int ia = atype == Pike.OPEN_BOUND ? 0 : itree(a);
	int ib = btype == Pike.OPEN_BOUND ? sizeof(trees)-1 : itree(b);

	if (ia == ib) {
	    ret->trees[ia] = trees[ia]->`[..](a, atype, b, btype);
	} else {
	    ret->trees[ia] = trees[ia]->`[..](a, atype, UNDEFINED, Pike.OPEN_BOUND);
	    ret->trees[ib] = trees[ib]->`[..](UNDEFINED, Pike.OPEN_BOUND, b, btype);
	    while (++ia < ib) {
		ret->trees[ia] = trees[ia]->copy();
	    }
	}

	return ret;
    }

    array(int) _indices() {
	return predef::`+(@map(trees, indices));
    }

    array(int) _values() {
	return predef::`+(@map(trees, values));
    }

    int(0..) _sizeof() {
	return predef::`+(@map(trees, sizeof));
    }

    array nth(int n) {
	if (n >= 0) for (int i = 0; i < sizeof(trees); i++) {
	    if (n < sizeof(trees[i])) {
		return trees[i]->nth(n);
	    }
	    n -= sizeof(trees[i]);
	}

	error("Index out of bounds.\n");
    }

    mixed _m_delete(mixed idx) {
	return m_delete(tree(idx), idx);
    }

    mixed _random() {
	int n = random(sizeof(this));
	return nth(n);
    }

    int first() {
	foreach (trees;; object t) {
	    if (sizeof(t)) return t->first();
	}
    }

    int last() {
	for (int i = sizeof(trees)-1; i; i --) {
	    if (sizeof(trees[i])) return trees[i]->last();
	}
    }

    mixed next(mixed k) {
	mixed ret;
	int i = itree(k);
	if (i == -1) error("Indexing %O with wrong type %O\n", this, k);
	if (undefinedp(ret = trees[i]->next(k))) {
	    for (i++; i < sizeof(trees); i++) {
		if (sizeof(trees[i])) return trees[i]->first();
	    }
	}
	return ret;
    }

    mixed previous(mixed k) {
	mixed ret;
	int i = itree(k);
	if (i == -1) error("Indexing %O with wrong type %O\n", this, k);
	if (undefinedp(ret = trees[i]->previous(k))) {
	    for (i--; i; i--) {
		if (sizeof(trees[i])) return trees[i]->last();
	    }
	}
	return ret;
    }

    this_program copy() {
	this_program ret = this_program();
	ret->trees = trees->copy();
	return ret;
    }

    int depth() {
	return max(@trees->depth());
    }

    this_program get_subtree(mixed k) {
	this_program ret = this_program();
	int i = itree(k);
	ret->trees[i] = trees[i]->get_subtree(k);
	return ret;
    }

    int(0..1) _equal(mixed o) {
	if (!(objectp(o) && Program.inherits(object_program(o), this_program))) return 0;
	if (sizeof(o->trees) != sizeof(trees)) return 0;
	foreach (trees; int i; object t) {
	    if (!equal(t, o->trees[i])) return 0;
	}
	return 1;
    }

    object _get_iterator(void|int(-1..-1)|int(1..1) step, mixed ... args) {
	return MultiIterator(step, @args);
    }

    mixed cast(string type) {
	return predef::`+(@trees->cast(type));
    }

    this_program `+(mixed o) {
	if (!(objectp(o) && Program.inherits(object_program(o), this_program))) error("Bad argument one to `+()\n");
	this_program ret = this_program();
	foreach (trees; int i; object t) {
	    ret->trees[i] = t + o->trees[i];
	}
	return ret;
    }

    this_program `-(mixed o) {
	if (!(objectp(o) && Program.inherits(object_program(o), this_program))) error("Bad argument one to `+()\n");
	this_program ret = this_program();
	foreach (trees; int i; object t) {
	    ret->trees[i] = t - o->trees[i];
	}
	return ret;
    }
	
    class MultiIterator {
	array oit, it;

	void create(int step, mixed|void start, mixed|void stop) {
	    int i = zero_type(start) ? 0 : itree(start);
	    int j = zero_type(stop) ? sizeof(trees)-1 : itree(stop);

	    array t = trees[i..j];
	    j = sizeof(t);

	    if (j == 1) {
		t[0] = t[0]->_get_iterator(step, start, stop);
	    } else {
		t[0] = t[0]->_get_iterator(step, start);
		for (i = 1; i < j-1; i++)
		    t[i] = t[i]->_get_iterator(step);
		t[-1] = t[-1]->_get_iterator(step, start);
	    }

	    this_program::oit = t;
	    this_program::it = t + ({ });
	}

	int(0..1) `!() {
	    while (sizeof(it) && !it[0]) it = it[1..];
	    return !sizeof(it);
	}

	int(0..1) next() {
	    while (sizeof(it) && !it[0]->next()) it = it[1..];
	    return !!sizeof(it);
	}

	mixed value() {
	    if (!sizeof(it)) return UNDEFINED;
	    return it[0]->value();
	}

	mixed index() {
	    if (!sizeof(it)) return UNDEFINED;
	    return it[0]->index();
	}

	object _get_iterator() {
	    return this;
	}
    }
}

#if constant(ADT._CritBit.BigNumTree)
// Don't let forcibly created "auto"bignums [Int.NATIVE_MIN, Int.NATIVE_MAX]
// anywhere near this (except in the values, that's ok)!
class IntTree {
    inherit MultiTree;
    array(object) trees = ({ Reverse(BigNumTree()), C::IntTree(), BigNumTree() });

    int itree(mixed i) {
	if (!intp(i)) error("Bad Index %O to %O\n", i, this);
	if (is_bignum(i)) {
	    return i < 0 ? 0 : 2;
	} else return 1;
    }
}
#endif // constant(ADT._CritBit.BigNumTree)

#endif // constant(ADT._CritBit)
