/*
 * $Id: mysql_result.pike,v 1.1 2004/06/13 21:47:21 mast Exp $
 *
 * Glue for the old broken Mysql-module which fetched all rows at once always
 */

#pike 7.7

private int recno;
private array(mapping(string:mixed)) fields;
private array(string|int) stash;

void create(object res) {
  fields=res->fetch_fields();
  array(string|int) row;
  for(stash=({});row=res->fetch_row();stash+=({row}));
}

int|array(string|int) fetch_row() {
  return recno<sizeof(stash) && stash[recno++];
}

array(mapping(string:mixed)) fetch_fields() {
  return fields;
}

void seek(int skip) {
  recno+=skip;
}

int eof() {
  return recno>sizeof(stash);
}

int num_rows() {
  return sizeof(stash);
}

int num_fields() {
  return sizeof(fields);
}
