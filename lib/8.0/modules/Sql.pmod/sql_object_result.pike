
#pike 8.1

#pragma no_deprecation_warnings

inherit .sql_result;

object master_res;

void create(object res) {
  if(!res || !objectp(res))
    error("Bad argument.\n");
  master_res = res;
}

int num_rows() {
  return master_res->num_rows();
}

int num_fields()
{
  return master_res->num_fields();
}

int eof() {
  return master_res->eof();
}

array(mapping(string:mixed)) fetch_fields() {
  return master_res->fetch_fields();
}

void seek(int skip) {

  if(functionp(master_res->seek)) {
    if(skip<0) error("Skip argument not positive.\n");
    index += skip;
    master_res->seek(skip);
  }
  else
    ::seek(skip);
}

int|array(string|int) fetch_row() {
  index++;
  return master_res->fetch_row();
}

this_program next_result()
{
  if (master_res->next_result) return master_res->next_result();
  return 0;
}
