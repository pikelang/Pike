
#pike 7.7

inherit Sql.Sql;

static array(mapping(string:mixed)) res_obj_to_array(object res_obj)
{
  if (res_obj)
  {
    // Not very efficient, but sufficient
    array(mapping(string:mixed)) res = ({});
    array(string) fieldnames;
    array(mixed) row;

    array(mapping) fields = res_obj->fetch_fields();
    if(!sizeof(fields)) return ({});

    fieldnames = (map(fields,
		      lambda (mapping(string:mixed) m) {
			return (m->table||"") + "." + m->name;
		      }) +
		  fields->name);

    if (case_convert)
      fieldnames = map(fieldnames, lower_case);

    while (row = res_obj->fetch_row())
      res += ({ mkmapping(fieldnames, row + row) });

    return res;
  }
  return 0;
}
