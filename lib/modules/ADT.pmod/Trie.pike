/*
 * $Id: Trie.pike,v 1.2 2007/09/06 14:28:31 grubba Exp $
 *
 * An implementation of a trie.
 *
 * 2007-08-24 Henrik Grubbström
 */


int offset;
mixed value = UNDEFINED;
string|array(int) path = ({});
mapping(int:this_program) trie;

static array(int) index;

static void create(string|array(int)|void key, int|void offset)
{
  path = key || ({});
  this_program::offset = offset || sizeof(path);
  path = path[..this_program::offset-1];
}

static void low_merge(int key, this_program o)
{
  if (!trie) {
    trie = ([ key : o ]);
    return;
  }
  this_program old = trie[key];
  if (!old) {
    trie[key] = o;
    index = 0;
    return;
  }
  int split = -1;
  if (old->offset <= o->offset) {
    for (int i = offset+1; i < old->offset; i++) {
      if (o->path[i] != old->path[i]) {
	split = i;
	break;
      }
    }
    if (split == -1) {
      old->merge(o);
      return;
    }
  } else {
    for (int i = offset+1; i < o->offset; i++) {
      if (o->path[i] != old->path[i]) {
	split = i;
	break;
      }
    }
    if (split == -1) {
      // FIXME: Ought to perform a reverse merge!
      o->merge(old);
      trie[o->path[offset]] = o;
      return;
    }
  }
  this_program new = ADT.Trie(o->path[..split-1]);
  new->merge(old);
  new->merge(o);
  trie[new->path[offset]] = new;
}

static void merge_trie(mapping(int:this_program) other_trie)
{
  if (!other_trie) return;
  foreach(other_trie; int key; this_program o) {
    low_merge(key, o);
  }
}

void merge(this_program o)
{
  if (o->offset < offset) {
    error("Problem!\n");
  }
  if (o->offset == offset) {
    if (!zero_type(o->value)) {
      value = o->value;
    }
    merge_trie(o->trie);
  } else {
    low_merge(o->path[offset], o);
  }
}

void insert(string|array(int) key, mixed val)
{
  this_program o;
  if (sizeof(key) == offset) {
    value = val;
  } else if (!trie) {
    trie = ([ key[offset]: (o = ADT.Trie(key)) ]);
    o->value = val;
  } else if (!(o = trie[key[offset]])) {
    trie[key[offset]] = o = ADT.Trie(key);
    o->value = val;
    index = 0;	// Invalidated.
  } else {
    int max = o->offset;
    int split = -1;
    if (max > sizeof(key)) {
      split = max = sizeof(key);
    }
    int i;
    for (i = offset+1; i < max; i++) {
      if (key[i] != o->path[i]) {
	split = i;
	break;
      }
    }
    if (split > -1) {
      this_program new_o = ADT.Trie(key, split);
      new_o->trie = ([ o->path[split] : o ]);
      o = trie[key[offset]] = new_o;
    }      
    o->insert(key, val);
  }
}

mixed lookup(string|array(int) key)
{
  if (sizeof(key) < offset) return UNDEFINED;
  if (sizeof(key) == offset) {
    foreach(key; int i; int|string val) {
      if (path[i] != val) return UNDEFINED;
    }
    return value;
  }
  this_program o = trie[key[offset]];
  if (!o) return UNDEFINED;
  return o->lookup(key);
}

static void update_index()
{
  if (!index) {
    index = sort(indices(trie));
  }
}

string|array(int) first()
{
  if (!trie) return UNDEFINED;
  update_index();
  this_program o = trie[index[0]];
  if (zero_type(o->value)) return o->first();
  return o->path;
}

string|array(int) next(string|array(int) base)
{
  if (!trie) return UNDEFINED;
  if (sizeof(base) <= offset) {
    return first();
  }
  this_program o = trie[base[offset]];
  if (o) {
    for (int i = offset+1; i < o->offset; i++) {
      if ((i >= sizeof(base)) || (o->path[i] > base[i])) {
	// o is a suffix to base, or is larger.
	if (zero_type(o->value)) return o->first();
	return o->path;
      } else if (o->path[i] < base[i]) break;
    }
    string|array(int) res = o->next(base);
    if (res) return res;
  }
  update_index();
  for (int i = 0; i < sizeof(index); i++) {
    if (index[i] > base[offset]) {
      o = trie[index[i]];
      if (zero_type(o->value)) return o->first();
      return o->path;
    }
  }
  return UNDEFINED;
}

static string render_path()
{
  if (arrayp(path)) {
    return sprintf("({%s})",
		   map(path, lambda(int val) {
			       return sprintf("%O", val);
			     }) * ", ");
  }
  return sprintf("%O", path);
}

static string _sprintf(int c, mapping|void attrs)
{
  if (c == 'O') {
    string res = sprintf("ADT.Trie(%s, ([", render_path());
    if (trie) {
      res += "\n";
      foreach(trie || ([]); int key; this_program o) {
	res += sprintf("  %O: %O,\n", key, o);
      }
    }
    if (zero_type(value)) {
      return res + "]))";
    } else {
      return res + sprintf("]): %O)", value);
    }
  } else {
    return sprintf("ADT.Trie(%s)", render_path());
  }
}
