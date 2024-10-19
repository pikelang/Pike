#pike __REAL_VERSION__

/*
 * An implementation of a trie.
 *
 * 2007-08-24 Henrik Grubbstr�m
 */


int offset;
mixed value = UNDEFINED;
string|array(int) path = ({});
mapping(int:this_program) trie;

protected array(int) index;
protected string|array(int) iterator_position;

protected void create(string|array(int)|void key, int|void offset)
{
  path = key || ({});
  this::offset = offset || sizeof(path);
  path = path[..this::offset-1];
}

protected void low_merge(int key, this_program o)
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

protected void merge_trie(mapping(int:this_program) other_trie)
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
    if (!undefinedp(o->value)) {
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
  if (undefinedp(val)) return;
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

mixed remove(string|array(int) key)
{
  mixed val;
  this_program o;
  if (sizeof(key) == offset) {
    val = value;
    value = UNDEFINED;
    index = 0;	// Invalidated.
    return val;
  } else if (!trie) {
    return UNDEFINED;
  } else if (!(o = trie[key[offset]])) {
    return UNDEFINED;
  } else {
    if (o->offset > sizeof(key)) {
      return UNDEFINED;
    }
    int i;
    for (i = offset+1; i < o->offset; i++) {
      if (key[i] != o->path[i]) {
	return UNDEFINED;
      }
    }
    if (!undefinedp(val = o->remove(key))) {
      if (undefinedp(o->value)) {
	if (!o->trie) {
	  m_delete(trie, key[offset]);
	  index = 0;	// Invalidated.
	  if (!sizeof(trie)) trie = 0;
	} else if (sizeof(o->trie) == 1) {
	  // Get rid of intermediate level.
	  trie[key[offset]] = values(o->trie)[0];
	}
      }
    }
    return val;
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
  if (!trie) return UNDEFINED;
  this_program o = trie[key[offset]];
  if (!o) return UNDEFINED;
  return o->lookup(key);
}

protected void update_index()
{
  if (!index) {
    index = sort(indices(trie));
  }
}

protected string|array(int) _first()
{
  if (!trie) return UNDEFINED;
  update_index();
  this_program o = trie[index[0]];
  if (undefinedp(o->value)) return o->first();
  return o->path;
}

string|array(int) first()
{
  iterator_position = UNDEFINED;
  return _first();
}

string|array(int) next(string|array(int) base)
{
  if (!trie) return UNDEFINED;
  if (sizeof(base) <= offset) {
    return _first();
  }
  this_program o = trie[base[offset]];
  if (o) {
    for (int i = offset+1; i < o->offset; i++) {
      if ((i >= sizeof(base)) || (o->path[i] > base[i])) {
	// o is a suffix to base, or is larger.
	if (undefinedp(o->value)) return o->first();
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
      if (undefinedp(o->value)) return o->first();
      return o->path;
    }
  }
  return UNDEFINED;
}

protected string|array(int) _iterator_next()
{
  if (!iterator_position) {
    return iterator_position = _first();
  }
  return iterator_position = next(iterator_position);
}

protected mixed _iterator_value()
{
  if (!iterator_position) return UNDEFINED;
  return lookup(iterator_position);
}

protected string|array(int) _iterator_index()
{
  if (!iterator_position) return UNDEFINED;
  return iterator_position;
}

protected string render_path()
{
  if (arrayp(path)) {
    return sprintf("({%s})",
		   map(path, lambda(int val) {
			       return sprintf("%O", val);
			     }) * ", ");
  }
  return sprintf("%O", path);
}

protected string _sprintf(int c, mapping|void attrs)
{
#ifdef ADT_TRIE_DEBUG
  if (c == 'O') {
    string res = sprintf("ADT.Trie(%s, ([", render_path());
    if (trie) {
      res += "\n";
      foreach(trie || ([]); int key; this_program o) {
	res += sprintf("  %O: %O,\n", key, o);
      }
    }
    if (undefinedp(value)) {
      return res + "]))";
    } else {
      return res + sprintf("]): %O)", value);
    }
  }
#endif
  return sprintf("ADT.Trie(%s)", render_path());
}
