// Table.pmod by Fredrik Noring, 1998
// $Id: Table.pmod,v 1.3 1998/03/25 16:59:16 noring Exp $

#define TABLE_ERR(msg) throw(({ "(Table) "+msg+"\n", backtrace() }))

class table {
  static private mapping fieldmap;
  static private array table, fields, types;
  
  static private array|int remap(array|string|int cs, int|void forgive)
  {
    array v = ({});
    int ap = arrayp(cs);
    if(!ap) cs = ({ cs });
    foreach(cs, string|int f)
      if(zero_type(intp(f)?f:fieldmap[lower_case(f)])) {
	if(!forgive)
	  TABLE_ERR("Unknown field '"+f+"'");
      } else
	v += ({ intp(f)?f:fieldmap[lower_case(f)] });
    return ap?v:v[0];
  }

  object copy(array|void tab, array|void fie, array|void typ)
  {
    return object_program(this_object())(tab||table,fie||fields,typ||types);
  }

  string encode()
  {
    return encode_value(([ "table":table,"fields":fields,"types":types ]));
  }

  object decode(string s)
  {
    mapping m = decode_value(s);
    return copy(m->table, m->fields, m->types);
  }
  
  mixed cast(string type)
  {
    switch(type) {
    case "array":
      return copy_value(table);
#if 0 // This works only in Pike 0.6.
    case "string":
      return ASCII->encode(this_object());
#endif
    }
  }

  array _indices()
  {
    return copy_value(fields);
  }  

  array _values()
  {
    return copy_value(table);
  }  
  
  int _sizeof()
  {
    return sizeof(table);
  }

  object reverse()
  {
    return copy(predef::reverse(table), fields, types);
  }
  
  array col(int|string c)
  {
    return copy_value(column(table, remap(c)));
  }

  array row(int r)
  {
    return copy_value(table[r]);
  }

  array `[](int|string c)
  {
    return col(c);
  }

  int `==(object t)
  {
    return (equal(Array.map(fields, lower_case),
		  Array.map(indices(t), lower_case)) &&
	    equal(table, values(t)));
  }

  object append_bottom(object t)
  {
    if(!equal(Array.map(indices(t), lower_case),
	      Array.map(fields, lower_case)))
      TABLE_ERR("Table fields are not equal.");
    return copy(table+values(t), fields, types);
  }

  object append_right(object t)
  {
    if(sizeof(t) != sizeof(table))
      TABLE_ERR("Table sizes are not equal.");
    fields += indices(t);
    array v = values(t);
    for(int r = 0; r < sizeof(table); r++)
      table[r] = table[r] + v[r];
    return this_object();
  }

  static private mixed op_col(function f, int|string c, mixed ... args)
  {
    c = remap(c);
    mixed x = table[0][c];
    for(int r = 1; r < sizeof(table); r++)
      f(x, table[r][c], @args);
    return x;
  }

  mixed sum_col(int|string c)
  {
    return `+(@column(table, remap(c)));
  }

  mixed average_col(int|string c)
  {
    return sum_col(c)/sizeof(table);
  }

  mixed min_col(int|string c)
  {
    return op_col(min, c);
  }

  mixed max_col(int|string c)
  {
    return op_col(max, c);
  }

  object select(int|string ... cs)
  {
    array t = ({});
    cs = remap(cs);
    for(int r = 0; r < sizeof(table); r++)
      t += ({ rows(table[r], cs) });
    return copy(t, rows(fields, cs), rows(types, cs));
  }

  object remove(int|string ... cs)
  {
    return select(@remap(fields) - remap(cs, 1));
  }

  object filter(function f, array(int|string)|int|string cs, mixed ... args)
  {
    array t = ({});
    cs = remap(arrayp(cs)?cs:({ cs }));
    foreach(table, mixed row)
      if(f(@rows(row, cs), @args))
	t += ({ row });
    return copy(t, fields, types);
  }

  object map(mapping(int|string:function) f, mixed ... args)
  {
    if(!sizeof(table))
      return this_object();

    mapping m = ([]);
    array cs = remap(indices(f));
    f = mkmapping(cs, values(f));
    array(int) keys = indices(fields) - cs;
    foreach(table, array row) {
      string key = encode_value(rows(row, keys));
      if(array a = m[key])
	foreach(cs, int c)
	  a[c] = f[c](a[c], row[c], @args);
      else
	m[key] = copy_value(row);
    }
    return copy(values(m), fields, types);
  }

  object sum(int|string ... cs)
  {
    array v = ({});
    for(int i = 0; i < sizeof(cs); i++)
      v += ({ `+ });
    return map(mkmapping(cs, v));
  }

  object distinct(int|string ... cs)
  {
    if(!sizeof(cs))
      return sum();
    array f = remap(fields) - remap(cs);
    mapping m = mkmapping(f, Array.map(f, lambda()
					  { return lambda(mixed x1,
							  mixed x2)
						   { return x1; }; }));
    return map(m);
  }

  object map_col(function f,array(int|string)|int|string cs,mixed ... args)
  {
    int ap = arrayp(cs);
    array t = copy_value(table);
    if(!catch(cs = remap(ap?cs:({ cs })))) {
      for(int r = 0; r < sizeof(t); r++) {
	mixed v = f(@rows(t[r], cs), @args);
	if(arrayp(v))
	  for(int i = 0; i < sizeof(v); i++)
	    t[r][cs[i]] = v[i];
	else
	  t[r][cs[0]] = v;
      }
    }
    return copy(t, fields, types);
  }

  object sort(int|string ... cs)
  {
    if(!sizeof(cs))
      return this_object();
    int c;
    array t = copy_value(table);
    if(!catch(c = remap(cs[-1]))) {
      mapping m = ([]);
      for(int r = 0; r < sizeof(t); r++) {
	mixed d;
	if(!m[d = t[r][c]])
	  m[d] = ({ t[r] });
	else
	  m[d] += ({ t[r] });
      }
      array i = indices(m), v = values(m);
      predef::sort(i, v);
      t = v*({});
    }
    return copy(t, fields, types)->sort(@cs[0..(sizeof(cs)-2)]);
  }

  object rsort(int|string ... cs)
  {
    return sort(@cs)->reverse();
  }
  
  object truncate(int n)
  {
    return copy(table[0..(n-1)], fields, types);
  }

  object rename(string from, string to)
  {
    return copy(table, replace(fields, fields[remap(from)], to), types);    
  }
  
  mapping type(int|string c, void|mapping m)
  {
    if(query_num_arg() == 2)
      types[remap(c)] = copy_value(m);
    return copy_value(types[remap(c)]);
  }

  void create(array(array) tab, array(string) fie, array|void typ)
  {
    if(!arrayp(tab))
      TABLE_ERR("Table not array");
    if(!arrayp(fie))
      TABLE_ERR("Fields not array");
    if(sizeof(tab) && sizeof(tab[0]) != sizeof(fie))
      TABLE_ERR("Table and field sizes differ");
    foreach(fie, string s)
      if(!stringp(s))
	TABLE_ERR("Field name not string");

    table = copy_value(tab);
    fields = copy_value(fie);
    types = allocate(sizeof(fie));

    if(typ)
      for(int i = 0; i < sizeof(fields); i++)
	if(!typ[i] || mappingp(typ[i]))
	  types[i] = copy_value(typ[i]);
	else
	  TABLE_ERR("Field type not mapping");

    array a = indices(allocate(sizeof(fields)));
    fieldmap = mkmapping(Array.map(fields, lower_case), a);
  }
}

object Separated = class {
  static private string _string(mixed x) { return (string)x; }
  
  object decode(string s, void|mapping options)
  {
    string rowsep = options->rowsep||"\n";
    string colsep = options->colsep||"\t";
    array t = Array.map(s/rowsep, `/, colsep);
    return table(t[1..], t[0], options->types);
  }
  
  mixed encode(object t, void|mapping options)
  {
    string rowsep = options->rowsep||"\n";
    string colsep = options->colsep||"\t";
    return Array.map(({ indices(t) }) + values(t),
           lambda(array r, string colsep)
	   { return Array.map(r, _string)*colsep; }, colsep)*rowsep;
  }
}();

object ASCII = class {
  object decode(string s, void|mapping options)
  {
    // Yet to be done
    return 0;
  }

  string encode(object t, void|mapping options)
  {
    options = options || ([]);
    mapping sizes = ([]);
    array fields = indices(t);
    string indent = String.strmult(" ", options->indent);
    
    t = t->copy(({ fields }) + values(t));
    foreach(indices(fields), string field)
      t = (t->map_col(lambda(mixed m, string field, mapping sizes)
		      { m = (string)m;
		        sizes[field] = max(sizeof(m), sizes[field]);
			return m; }, field, field, sizes)->
	   map_col(lambda(string s, string size, int num)
		   { return sprintf("%"+(num?"":"-")+size+"s", s); },
		   field, (string)sizes[field],
		   (t->type(field)||([]))->type == "num"));

    string l = (indent+"-"+
		Array.map(values(sizes),
			  lambda(int n)
			  { return String.strmult("-", n); })*"---"+"-");
    array table = values(t);
    return (indent+" "+table[0]*"   "+"\n"+l+"\n"+
	    Array.map(table[1..], lambda(array row, string indent)
				  { return indent+" "+row*"   "; },
		      indent)*"\n"+"\n"+l+"\n");
  }
}();

// Experimental
object SQL = class {
  object decode(array t, void|mapping options)
  {
    // Yet to be done
    return 0;
  }

  array encode(object t, void|mapping options)
  {
    options = options||([]);
    string tablename = options->tablename||"sql_encode_default_table";

    array queries = ({});
    string fields = indices(t)*", ";
    foreach(values(t), array row)
      queries += ({ "insert into "+tablename+" ("+fields+") "
		    "values("+
		    Array.map(row, lambda(mixed x)
				   { return "'"+(string)x+"'"; })*", "+
		    ")" });
    return queries;
  }
}();

// Huh? Experimental help..
string help()
{
  return ("This is the experimental help system for Table.pmod.\n"
	  "Table.pmod contains the following classes and objects:\n"
	  "\n"
	  "  Table.table(array(array) table, array(string) fields, "
	  "array(mapping) types)\n"
	  "  Table.ASCII.encode\n"
	  "  Table.ASCII.decode\n"
	  "  Table.Separated.decode\n"
	  "  Table.Separated.encode\n"
	  "  Table.SQL.decode\n"
	  "  Table.SQL.encode\n");
}
