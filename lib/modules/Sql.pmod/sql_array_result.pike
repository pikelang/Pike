
#pike __REAL_VERSION__

inherit .sql_result;

array master_res;

void create(array res) {
  if(!res || !arrayp(res))
    error("Bad argument.\n");
  master_res = res;
}

int num_rows() {
  return sizeof(master_res);
}

int num_fields() {
  return sizeof(master_res[0]);
}

int eof() {
  return index >= sizeof(master_res);
}

// Only supports the name field.
array(mapping(string:mixed)) fetch_fields() {
  array(mapping(string:mixed)) res = allocate(sizeof(master_res[0]));
  int i;

  foreach(sort(indices(master_res[0])), string name)
    res[i++] = ([ "name": name ]);

  return res;
}

void seek(int skip) {
  if(skip<0) error("Skip argument not positive.\n");
  index += skip;
}

int|array(string|int) fetch_row() {
  array res;
      
  if (index >= sizeof(master_res))
    return 0;

  sort(indices(master_res[index]), res = values(master_res[index]));
  index++;
  return res;
}
