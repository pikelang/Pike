// Table.pmod by Fredrik Noring, 1998
// $Id: Table.pmod,v 1.28 2004/09/25 02:51:20 nilsson Exp $

#pike __REAL_VERSION__
#define TABLE_ERR(msg) error("(Table) "+msg+"\n")

//! ADT.Table is a generic module for manipulating tables.
//!
//! Each table contains one or several columns.
//! Each column is associated with a name, the column name.
//! Optionally, one can provide a column type. The Table module can do a number
//! of operations on a given table, like computing the sum of a column,
//! grouping, sorting etc.
//! 
//! All column references are case insensitive. A column can be referred to by
//! its position (starting from zero). All operations are non-destructive. That
//! means that a new table object will be returned after, for example, a sort.

//! The table base-class.
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

  this_program copy(array|void tab, array|void fie, array|void typ)
  {
    return this_program(tab||table,fie||fields,typ||types);
  }

  //! This method returns a binary string representation of the table. It is
  //! useful when one wants to store a the table, for example in a file.
  string encode()
  {
    return encode_value(([ "table":table,"fields":fields,"types":types ]));
  }

  //! This method returns a table object from a binary string
  //! representation of a table, as returned by @[encode()].
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
    case "string":
      return ASCII->encode(this);
    }
  }

  //! This method returns the column names for the table. The case used when
  //! the table was created will be returned.
  array(string) _indices()
  {
    return copy_value(fields);
  }  

  //! This method returns the contents of a table as a two dimensional array.
  //! The format is an array of rows. Each row is an array of columns.
  array(array) _values()
  {
    return copy_value(table);
  }  

  //! This method returns the number of rows in the table.
  int _sizeof()
  {
    return sizeof(table);
  }

  //! This method reverses the rows of the table and returns a
  //! new table object.
  object reverse()
  {
    return copy(predef::reverse(table), fields, types);
  }
  
  //! This method returns the contents of a given column as an array.
  array col(int|string column)
  {
    return copy_value(predef::column(table, remap(column)));
  }

  //! This method returns the contents of a given row as an array.
  array row(int row_number)
  {
    return copy_value(table[row_number]);
  }

  //! Same as @[col()].
  array `[](int|string column)
  {
    return col(column);
  }

  //! This method compares two tables. They are equal if the contents
  //! of the tables and the column names are equal. The column name
  //! comparison is case insensitive.
  int `==(object table)
  {
    return (equal(Array.map(fields, lower_case),
		  Array.map(indices(table), lower_case)) &&
	    equal(this_program::table, values(table)));
  }

  //! This method appends two tables. The table given as an argument will be
  //! added at the bottom of the current table. Note, the column names must
  //! be equal. The column name comparison is case insensitive.
  object append_bottom(object table)
  {
    if(!equal(Array.map(indices(table), lower_case),
	      Array.map(fields, lower_case)))
      TABLE_ERR("Table fields are not equal.");
    return copy(this_program::table+values(table), fields, types);
  }

  //! This method appends two tables. The table given as an argument will be
  //! added on the right side of the current table. Note that the number of
  //! rows in both tables must be equal.
  object append_right(object table)
  {
    if(sizeof(table) != sizeof(this_program::table))
      TABLE_ERR("Table sizes are not equal.");
    array v = values(table);
    for(int r = 0; r < sizeof(this_program::table); r++)
      v[r] = this_program::table[r] + v[r];
    return copy(v, fields+indices(table), types+table->all_types());
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

  //! This method returns a new table object with the selected columns only.
  object select(int|string ... columns)
  {
    array t = ({});
    columns = remap(columns);
    for(int r = 0; r < sizeof(table); r++)
      t += ({ rows(table[r], columns) });
    return copy(t, rows(fields, columns), rows(types, columns));
  }

  //! Like @[select()], but the given @[columns] will not be in the
  //! resulting table.
  object remove(int|string ... columns)
  {
    return select(@remap(fields) - remap(columns, 1));
  }

  //! This method calls the function for each row. If the function
  //! returns zero, the row will be thrown away. If the function
  //! returns something non-zero, the row will be kept. The result
  //! will be returned as a new table object.
  object where(array(int|string)|int|string columns, function f,
	       mixed ... args)
  {
    array t = ({});
    f = f || lambda(mixed x) { return x; };
    columns = remap(arrayp(columns)?columns:({ columns }));
    foreach(table, mixed row)
      if(f(@rows(row, columns), @args))
	t += ({ row });
    return copy(t, fields, types);
  }

  //! This method calls the function @[f] for each column each time a
  //! non uniqe row will be joined. The table will be grouped by the
  //! columns not listed. The result will be returned as a new table object.
  this_program group(mapping(int|string:function)|function f, mixed ... args)
  {
    if(!sizeof(table))
      return this;

    if(functionp(f)) {
      if(!arrayp(args[0]))
	args[0] = ({ args[0] });
      f = mkmapping(args[0], allocate(sizeof(args[0]), f));
      args = args[1..];
    }
    
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

  //! This method sums all equal rows. The table will be grouped by the
  //! columns not listed. The result will be returned as a new table object.
  this_program sum(int|string ... columns)
  {
    return group(`+, columns);
  }

  //! This method groups by the given columns and returns a table with only
  //! unique rows. When no columns are given, all rows will be unique. A new
  //! table object will be returned.
  this_program distinct(int|string ... columns)
  {
    if(!sizeof(columns))
      return sum();
    array f = remap(fields) - remap(columns);
    mapping m = mkmapping(f, Array.map(f, lambda(mixed unused)
					  { return lambda(mixed x1,
							  mixed x2)
						   { return x1; }; }));
    return group(m);
  }

  //! This method calls the function @[f] for all rows in the table.
  //! The value returned will replace the values in the columns given
  //! as argument to map. If the function returns an array, several
  //! columns will be replaced. Otherwise the first column will be
  //! replaced. The result will be returned as a new table object.
  object map(function f, array(int|string)|int|string columns, mixed ... args)
  {
    int ap = arrayp(columns);
    array t = copy_value(table);
    if(!catch(columns = remap(ap?columns:({ columns })))) {
      for(int r = 0; r < sizeof(t); r++) {
	mixed v = f(@rows(t[r], columns), @args);
	if(arrayp(v))
	  for(int i = 0; i < sizeof(v); i++)
	    t[r][columns[i]] = v[i];
	else
	  t[r][columns[0]] = v;
      }
    }
    return copy(t, fields, types);
  }

  static private this_program _sort(int is_reversed, int|string ... cs)
  {
    if(!sizeof(cs))
      return this;
    int c;
    array t = copy_value(table);
    if(!catch(c = remap(cs[-1])))
    {
      mapping m = ([]);
      for(int r = 0; r < sizeof(t); r++)
      {
	mixed d;
	if(!m[d = t[r][c]])
	  m[d] = ({ t[r] });
	else
	  m[d] += ({ t[r] });
      }
      array i = indices(m), v = values(m);
      if(types[c] && types[c]->type=="num")
	i = (array(float))i;
      predef::sort(i, v);
      t = (is_reversed ? predef::reverse(v) : v)*({});
    }
    return is_reversed ?
      copy(t, fields, types)->rsort(@cs[0..(sizeof(cs)-2)]) :
      copy(t, fields, types)->sort(@cs[0..(sizeof(cs)-2)]);
  }

  //! This method sorts the table in ascendent order on one or several columns
  //! and returns a new table object. The left most column is sorted last. Note
  //! that the sort is stable.
  //!
  //! @seealso
  //! @[rsort()]
  //!
  object sort(int|string ... columns)
  {
    return _sort(0, @columns);
  }
  
  //! Like @[sort()], but in descending order.
  object rsort(int|string ... columns)
  {
    return _sort(1, @columns);
  }

  //! This method truncates the table to the first @[n] rows and returns
  //! a new object.
  object limit(int n)
  {
    return copy(table[0..(n-1)], fields, types);
  }

  //! This method renames the column named @[from] to @[to] and
  //! returns a new table object. Note that @[from] can be the column
  //! position.
  object rename(string|int from, string to)
  {
    array a = copy_value(fields);
    a[remap(from)] = to;
    return copy(table, a, types);
  }

  //! This method gives the type for the given @[column].
  //!
  //! If a second argument is given, the old type will be replaced
  //! with @[type]. The column type is only used when the table is displayed.
  //! The format is as specified in @[create()].
  mapping type(int|string column, void|mapping type)
  {
    if(query_num_arg() == 2)
      types[remap(column)] = copy_value(type);
    return copy_value(types[remap(column)]);
  }

  array all_types()
  {
    return copy_value(types);
  }

  //!   The @[ADT.Table.table] class takes two or three arguments:
  //!
  //! @param table
  //!   The first argument is a two-dimensional array consisting of
  //!   one array of columns per row. All rows must have the same
  //!   number of columns as specified in @[column_names].
  //!
  //! @param column_names
  //!   This argument is an array of column names associated with each
  //!   column in the table. References by column name are case insensitive.
  //!   The case used in @[column_names] will be used when the table is
  //!   displayed. A column can also be referred to by its position,
  //!   starting from zero.
  //!
  //! @param column_types
  //!   This is an optional array of mappings. The column type
  //!   information is only used when displaying the table. Currently, only the
  //!   keyword @expr{"type"@} is recognized. The type can be specified as
  //!   @expr{"text"@} or @expr{"num"@} (numerical). Text columns are left
  //!   adjusted, whereas numerical columns are right adjusted. If a mapping
  //!   in the array is 0 (zero), it will be assumed to be a text column.
  //!   If @[column_types] is omitted, all columns will displayed as text.
  //!
  //!   See @[ADT.Table.ASCII.encode()] on how to display a table.
  //!
  //! @seealso
  //!   @[ADT.Table.ASCII.encode()]
  //!
  void create(array(array) table, array(string) column_names,
	      array(mapping(string:string))|void column_types)
  {
    if(!arrayp(table))
      TABLE_ERR("Table not array");
    if(!arrayp(column_names))
      TABLE_ERR("Fields not array");
    if(sizeof(table) && sizeof(table[0]) != sizeof(column_names))
      TABLE_ERR("Table and field sizes differ");
    foreach(column_names, string s)
      if(!stringp(s))
	TABLE_ERR("Field name not string");

    this_program::table = copy_value(table);
    fields = copy_value(column_names);
    types = allocate(sizeof(column_names));

    if(column_types)
      for(int i = 0; i < sizeof(fields); i++)
	if(!column_types[i] || mappingp(column_types[i]))
	  types[i] = copy_value(column_types[i]);
	else
	  TABLE_ERR("Field type not mapping");

    array(int) a = indices(allocate(sizeof(fields)));
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
    options = options || ([]);
    string rowsep = options->rowsep||"\n";
    string colsep = options->colsep||"\t";
    return Array.map(({ indices(t) }) + values(t),
           lambda(array r, string colsep)
	   { return Array.map(r, _string)*colsep; }, colsep)*rowsep;
  }
}();

//! @module ASCII

//! @ignore
object ASCII = class {
//! @endignore
  object decode(string s, void|mapping options)
  {
    // Yet to be done.
    return 0;
  }

  //! @decl string encode(object table, void|mapping options)
  //!
  //! This method returns a table represented in ASCII suitable for
  //! human eyes.  @[options] is an optional mapping. If the keyword
  //! @expr{"indent"@} is used with a number, the table will be
  //! indented with that number of space characters.

  string encode(object t, void|mapping options)
  {
    options = options || ([]);
    mapping sizes = ([]);
    array fields = indices(t);
    string indent = " " * options->indent;
    
    t = t->copy(({ fields }) + values(t));
    for(int field = 0; field < sizeof(fields); field++)
      t = (t->map(lambda(mixed m, int field, mapping sizes)
		  {
		    m = (string)m;
		    sizes[field] = max(sizeof(m), sizes[field]);
		    return m;
		  },
		  field, field, sizes)->
	   map(lambda(string s, string size, int num)
	       {
		 return sprintf("%"+(num?"":"-")+size+"s", s);
	       },
	       field, (string)sizes[field],
	       (t->type(field)||([]))->type == "num"));

    string l = (indent+"-"+
		Array.map(values(sizes),
			  lambda(int n)
			  { return "-" * n; })*"---"+"-");
    array table = values(t);
    return (indent+" "+table[0]*"   "+"\n"+l+"\n"+
	    Array.map(table[1..], lambda(array row, string indent)
				  { return indent+" "+row*"   "; },
		      indent)*"\n"+(sizeof(table)>1?"\n":"")+l+"\n");
  }
//! @ignore
}();
//! @endignore

//! @endmodule

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
