
#pike __REAL_VERSION__

inherit __builtin.Sql.Result;

array _values;

protected void create(array res) {
  if(!res || !arrayp(res))
    error("Bad argument.\n");
  _values = res;
}

int num_rows() {
  return sizeof(_values);
}

int num_fields() {
  return sizeof(_values[0]);
}

int eof() {
  return index >= sizeof(_values);
}

// Only supports the name field.
array(mapping(string:mixed)) fetch_fields() {
  array(mapping(string:mixed)) res = allocate(sizeof(_values[0]));
  int i;

  foreach(sort(indices(_values[0])), string name)
    res[i++] = ([ "name": name ]);

  return res;
}

void seek(int skip) {
  if(skip<0) error("Skip argument not positive.\n");
  index += skip;
}

int|array(string|int) fetch_row() {
  array res;

  if (index >= sizeof(_values))
    return 0;

  sort(indices(_values[index]), res = values(_values[index]));
  index++;
  return res;
}

this_program next_result()
{
  return 0;
}
