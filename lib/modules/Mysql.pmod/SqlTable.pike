#pike __REAL_VERSION__
#require constant(___Mysql)

//! This class provides some abstractions on top of an SQL table.
//!
//! At the core it is generic for any SQL database, but the current
//! implementation is MySQL specific on some points, notably the
//! semantics of AUTO_INCREMENT, the quoting method, knowledge about
//! column types, and some conversion functions. Hence the location in
//! the @[Mysql] module.
//!
//! Among other things, this class handles some convenient conversions
//! between SQL and pike data types:
//!
//! @ul
//! @item
//!   Similar to @[Sql.big_typed_query], SQL integer and floating
//!   point columns are converted to/from pike ints and floats, and
//!   SQL NULLs are converted to/from the @[Val.null] object.
//!
//!   MySQL DECIMAL columns are converted to/from @[Gmp.mpq] objects
//!   if they have one or more decimal places, otherwise they are
//!   converted to/from ints.
//!
//! @item
//!   MySQL TIMESTAMP columns are converted to/from pike ints
//!   containing unix timestamps. This conversion is done on the MySQL
//!   side using the UNIX_TIMESTAMP and FROM_UNIXTIME functions, which
//!   means that the conversion is not susceptible to offsets due to
//!   time zone differences etc. There is however one special case
//!   here that MySQL doesn't handle cleanly - see note below.
//!
//! @item
//!   Other SQL types are kept in string form. That includes DATE,
//!   TIME, and DATETIME, which are returned as MySQL formats them.
//!
//!   Note that @[Sql.mysql] can handle conversions to/from Unicode
//!   strings for text data types. If that is enabled then this class
//!   also supports that conversion.
//!
//! @item
//!   There are debug checks (with the DEBUG define) that verify the
//!   incoming pike types, to avoid bugs which could otherwise be
//!   hidden by implicit casts on the SQL side. The date and time
//!   types (except TIMESTAMP) can be sent either as strings or
//!   integers (e.g. either "2010-01-01" or 20100101).
//! @endul
//!
//! This class can also optionally simulate an arbitrary set of fields
//! in each table row: If a field name is the same as a column then
//! the column is accessed, otherwise it accesses an entry in a
//! mapping stored in a special BLOB column which is usually called
//! "properties".
//!
//! @note
//! Although SQL is case insensitive on column names, this class
//! isn't.
//!
//! @note
//! The generated SQL queries always quote table and column names
//! according to MySQL syntax using backticks (`). However, literal
//! backticks in names are not quoted and might therefore lead to SQL
//! syntax errors. This might change if it becomes a problem.
//!
//! @note
//! The handling of TIMESTAMP columns in MySQL (as of 5.1 at least)
//! through UNIX_TIMESTAMP and FROM_UNIXTIME has one problem if the
//! active time zone uses daylight savings time:
//!
//! Apparently FROM_UNIXTIME internally formats the integer to a MySQL
//! date/time string, which is then parsed again to set the unix
//! timestamp in the TIMESTAMP column. The formatting and the parsing
//! uses the same time zone, so the conversions generally cancel
//! themselves out. However, there is one exception with the 1 hour
//! overlap in the fall when going from summer time to normal time.
//!
//! E.g. if the active time zone on the connection is Central European
//! Time, which uses DST, then setting 1130630400 (Sun 30 Oct 2005
//! 2:00:00 CEST) through "INSERT INTO foo SET ts =
//! FROM_UNIXTIME(1130630400)" actually sets the ts column to
//! 1130634000 (Sun 30 Oct 2005 2:00:00 CET).
//!
//! The only way around that problem is apparently to ensure that the
//! time zone used on the connection is one which doesn't use DST.
//! E.g. UTC is a reasonable choice, which can be set on the
//! connection through "SET time_zone = '+00:00'". That is not done
//! automatically by this class.

#ifdef SQLTOOLS_UPDATE_DEBUG
#define UPDATE_MSG(X...) werror (X)
#else
#define UPDATE_MSG(X...) 0
#endif

// Ideas for the future:
// o  Generalize to support different db backends.
// o  Add hooks to make it possible to cache the record mappings.
// o  Make it possible to replace the encode/decode functions for the
//    properties column.
// o  Make it possible to specify custom conversion functions for
//    individual fields.
// o  Add optional caching of compiled queries.

function(void:Sql.Sql) get_db;
//! Callback to get a database connection.

string table;
//! The table to query or change. Do not change.

string id_col;
//! The column containing the AUTO_INCREMENT values (if any).

array(string) pk_cols;
//! The column(s) containing the primary key, in order. Typically it
//! is the same as @expr{({@[id_col]})@}.

string prop_col;
//! The column containing miscellaneous properties. May be zero if
//! this feature is disabled. Do not change.

int prop_col_max_length;
//! Maximum length of the value @[prop_col] can hold. Only applicable
//! if @[prop_col] is set. Do not change.

mapping(string:string) col_types;
//! Maps the names of the table columns to the types @[SqlTable] will
//! handle them as. This is queried from the database in @[create]. Do
//! not change.

protected mapping(string:int(1..1)) timestamp_cols;
// Mapping tracking all TIMESTAMP columns.

protected mapping(string:int(1..1)) datetime_cols;
// Mapping tracking all date and time columns (except TIMESTAMP),
// which can be sent either as strings or integers.

protected string query_charset;
// Mysql charset to use in queries. This is dictated by the table and
// column names. If none of them are wide, we always use latin1 to
// avoid the encoding overhead in Sql.mysql (even though that is not
// strictly correct - mysql latin1 have some different chars in the
// high control char range of iso-8859-1, but we ignore that). If wide
// identifiers are found, this is zero to let Sql.mysql fix the
// charsets, and it better be in unicode encode mode then. This does
// not affect string constants since we always specify the charset for
// every string literal.

// Converts mysql types to pike types for col_types. All types not
// mentioned here become "string".
protected constant mysql_types_map = ([
  "float": "float", "double": "float",
  "char": "int", "short": "int",
  "int24": "int", "long": "int", "longlong": "int",
  "timestamp": "int", "bit": "int",
  "decimal": "float", "newdecimal": "float",
]);

// Mysql decimal types. These are represented as integers if they have
// no fraction part, otherwise as Gmp.mpq objects.
protected constant mysql_decimal_types = ([
  "decimal": 1, "newdecimal": 1,
]);

protected constant mysql_datetime_types = ([
  "datetime": 1, "date": 1, "time": 1, "year": 1,
]);

// FIXME: Add a setting to be able to "deprecate" columns for
// migration from a column to a field in the properties blob. The
// deprecation means the column will be queried, but the value will be
// put into the properties mapping on update.

// FIXME: Conversely, add a setting to deprecate properties be able to
// migrate them to columns. Normally it solves itself since properties
// have priority over columns, but select() and get() need to be aware
// of such columns so they don't incorrectly skip prop_col altogether
// in the query.

protected void create (function(void:Sql.Sql) get_db,
		       string table,
		       void|string prop_col)
//! Creates an @[SqlTable] object for accessing (primarily) a specific
//! table.
//!
//! @param get_db
//! A function that will be called to get a connection to the database
//! containing the table.
//!
//! @param table
//! The name of the table.
//!
//! @param prop_col
//! The column in which all fields which don't have explicit columns
//! are stored. It has to be a non-null blob or varbinary column. If
//! this isn't specified and there is such a column called
//! "properties" then it is used for this purpose. Set to @expr{"-"@}
//! to force this feature to be disabled.
{
  this_program::get_db = get_db;
  this_program::table = table;

  Sql.Sql conn = get_db();

  query_charset = String.width (table) == 8 && "latin1";

  {
    col_types = ([]);
    timestamp_cols = ([]);
    datetime_cols = ([]);
    array(mapping(string:mixed)) col_list =
      conn->list_fields (string_to_utf8 (table));
    mapping(string:mixed) prop_col_info;

    foreach (col_list, mapping(string:mixed) col_info) {
      string name = utf8_to_string (col_info->name);
      if (String.width (name) > 8) query_charset = 0;
      if (col_types[name])
	error ("Strange duplicate column %O in %O\n", name, col_list);
      if (mysql_decimal_types[col_info->type] && !col_info->decimals)
	col_types[name] = "int";
      else
	col_types[name] = mysql_types_map[col_info->type] || "string";
      if (col_info->type == "timestamp") timestamp_cols[name] = 1;
      if (mysql_datetime_types[col_info->type]) datetime_cols[name] = 1;
      if (name == (prop_col || "properties"))
	prop_col_info = col_info;
    }

    if (prop_col != "-") {
      if (prop_col_info)
	if (!prop_col_info->flags->binary ||
	    !prop_col_info->flags->not_null ||
	    !(<"var string", "blob">)[prop_col_info->type])
	  prop_col_info = 0;
      if (prop_col && !prop_col_info)
	error ("Table doesn't have a non-null binary column %O "
	       "to store extra fields in.\n", prop_col);
      if (prop_col_info) {
	if (!prop_col_info->length)
	  error ("Unable to determine maximum length of the property "
		 "column %O. Got column info: %O\n", prop_col, prop_col_info);
	this_program::prop_col = prop_col_info->name;
	this_program::prop_col_max_length = prop_col_info->length;
      }
    }
  }

  foreach (conn->query ("SHOW COLUMNS FROM `" + table + "`"),
	   mapping(string:string) col_info)
    if (col_info->Extra == "auto_increment") {
      id_col = col_info->Field;
      break;
    }

  {
    mapping(int:string) pkc = ([]);
    foreach (conn->query ("SHOW INDEX FROM `" + table + "`"),
	     mapping(string:string) ind_col)
      if (ind_col->Key_name == "PRIMARY")
	pkc[(int) ind_col->Seq_in_index] = ind_col->Column_name;
    pk_cols = values (pkc);
    sort (indices (pkc), pk_cols);
  }
}

protected string _sprintf (int flag)
{
  return flag == 'O' &&
    sprintf ("%s(%O)",
	     function_name (object_program (this)) || "SqlTable",
	     table);
}

local string quote (string s)
//! Quotes a string literal for inclusion in an SQL statement, e.g. in
//! a WHERE clause to @[select].
//!
//! @note
//! Most functions here take raw string literals. Quoting is seldom
//! necessary.
{
  // Hardwired mysql quoting since we don't support anything else
  // (yet). The original ought to be in a module.. :P
  return predef::replace(s,
			 ({ "\\", "\"", "\0", "\'", "\n", "\r" }),
			 ({ "\\\\", "\\\"", "\\0", "\\\'", "\\n", "\\r" }));
}

protected int argspec_counter;

local string handle_argspec (array argspec, mapping(string:mixed) bindings)
//! Helper function for use with array style arguments.
//!
//! Many functions in this class can take WHERE expressions etc either
//! as plain strings or as arrays. In array form, they work like when
//! @[Sql.Sql.query] is called with more than one argument:
//!
//! The first element in the array is a string containing the SQL
//! snippet. If the second element is a mapping, it's taken as a
//! bindings mapping, otherwise the remaining elements are formatted
//! using the first in @expr{sprintf@} fashion. See @[Sql.Sql.query]
//! for further details.
//!
//! This function reduces an argument on array form to a simple
//! string, combined with bindings. @[bindings] is a mapping that is
//! modified to contain the new bindings.
//!
//! @note
//! The @[quote] function can be used to quote string literals in the
//! query, to avoid the array format.
//!
//! @returns
//! Return the SQL snippet in string form, possibly with variable
//! bindings referring to @[bindings].
{
  if (sizeof (argspec) == 2 && mappingp (argspec[1])) {
    foreach (argspec[1]; string|int var; mixed val)
      bindings[var] = val;
    return argspec[0];
  }
  else
    return Sql.sql_util.handle_extraargs (argspec[0], argspec[1..],
					  bindings)[0];
}

// Implicit connection handling.

int insert (mapping(string:mixed)... records)
//! Inserts one or more records into the table using an INSERT
//! command. An SQL error is thrown if a record conflicts with an
//! existing one.
//!
//! A record is represented as a mapping with one entry for each
//! column or property (if @[prop_col] is used). The values must be of
//! the right type for the column: Integers for integer columns,
//! floats for floating point columns, strings for all other data
//! types, and @[Val.null] for the SQL NULL value.
//!
//! If the property feature is used (i.e. if @[prop_col] is set) then
//! any entries in the record mapping that don't match a column are
//! treated as properties and are stored encoded in the @[prop_col]
//! column. Note that column names are matched with case sensitivity.
//! Properties may store any pike data type (as long as it is accepted
//! by @[encode_value_canonic]).
//!
//! If @[id_col] is set and that column doesn't exist in a record
//! mapping then the field is added to the mapping with the value that
//! the record got. This currently only works for the first record
//! mapping if there are several.
//!
//! @returns
//! The value of the @[id_col] column for the new record. If several
//! records are inserted at once then the value for the first one is
//! returned. Zero is returned if there is no @[id_col] column.
//!
//! @seealso
//! @[insert_ignore], @[replace], @[insert_or_update]
{
  return conn_insert (get_db(), @records);
}

int insert_ignore (mapping(string:mixed)... records)
//! Inserts one or more records into the table using an INSERT IGNORE
//! command. If some of the given records conflict with existing
//! records then they are ignored.
//!
//! Zero is returned if all records were ignored. The record mapping
//! is updated with the @[id_col] record id only if a single record is
//! given. Otherwise this function behaves like @[insert].
//!
//! @seealso
//! @[insert], @[replace]
{
  return conn_insert_ignore (get_db(), @records);
}

int replace (mapping(string:mixed)... records)
//! Inserts one or more records into the table using a REPLACE
//! command. If some of the given records conflict with existing
//! records then they are replaced.
//!
//! Otherwise this function behaves like @[insert].
//!
//! @seealso
//! @[insert], @[insert_ignore]
{
  return conn_replace (get_db(), @records);
}

void update (mapping(string:mixed) record,
	     void|int(0..2) clear_other_fields)
//! Updates an existing record. This requires a primary key and that
//! @[record] contains values for all primary key columns. If
//! @[record] doesn't correspond to any existing record then nothing
//! happens.
//!
//! Updating a record normally means that all fields in @[record]
//! override those stored in the table row, while all other fields
//! keep their values.
//!
//! It's the same for properties (i.e. fields that don't correspond to
//! columns) which are stored in the @[prop_col] column. If that
//! column needs to be updated then by default the old value is
//! fetched first, which means the update isn't atomic in that case. A
//! property can be removed altogether by giving it the value
//! @[Val.null] in @[record].
//!
//! If @[clear_other_fields] is 1 then all old properties are replaced
//! by the new ones instead of merged with them, which avoids the
//! extra fetch. If @[clear_other_fields] is 2 then additionally all
//! unmentioned columns are reset to their default values.
//!
//! For more details about the @[record] mapping, see @[insert].
//!
//! @seealso
//! @[insert_or_update]
{
  conn_update (get_db(), record, clear_other_fields);
}

int insert_or_update (mapping(string:mixed) record,
		      void|int(0..2) clear_other_fields)
//! Insert a record into the table using an INSERT ... ON DUPLICATE
//! KEY UPDATE command: In case @[record] conflicts with an existing
//! record then it is updated like the @[update] function would do,
//! otherwise it is inserted like @[insert] would do.
//!
//! If @[id_col] is set and that column doesn't exist in @[record]
//! then the field is added to the mapping with the value that the
//! inserted or updated record got.
//!
//! @returns
//! The value of the @[id_col] column for the new or updated record.
//! Zero is returned if there is no @[id_col] column.
//!
//! @note
//! This function isn't atomic if @[clear_other_fields] is unset and
//! @[record] contains fields which do not correspond to real columns,
//! i.e. if the @[prop_col] column may need to be updated.
{
  return conn_insert_or_update (get_db(), record, clear_other_fields);
}

void delete (string|array where, void|string|array rest)
//! Deletes records from the table that matches a condition.
//!
//! Both @[where] and @[rest] may be given as arrays to use bindings
//! or @expr{sprintf@}-style formatting - see @[handle_argspec] for
//! details.
//!
//! @param where
//! The match condition, on the form of a WHERE expression.
//!
//! A WHERE clause will always be generated, but you can put e.g.
//! "TRUE" in the match condition to select all records.
//!
//! @param rest
//! Optional clauses that follows after the WHERE clause in a DELETE,
//! i.e. ORDER BY and/or LIMIT.
//!
//! @seealso
//! @[remove]
//!
//! @fixme
//! Add support for joins.
{
  conn_delete (get_db(), where, rest);
}

void remove (mixed id)
//! Removes the record matched by the primary key value in @[id].
//! Nothing happens if there is no such record.
//!
//! If the table has a multicolumn primary key then @[id] must be an
//! array which has the values for the primary key columns in the same
//! order as @[pk_cols]. Otherwise @[id] is taken directly as the
//! value of the single primary key column.
//!
//! @seealso
//! @[remove_multi], @[delete]
{
  conn_remove (get_db(), id);
}

void remove_multi (array(mixed) ids)
//! Removes multiple records selected by primary key values. It is not
//! an error if some of the @[ids] elements don't match any records.
//!
//! This function currently only works if the primary key is a single
//! column.
//!
//! @seealso
//! @[remove]
{
  conn_remove_multi (get_db(), ids);
}

Result select (string|array where, void|array(string) fields,
	       void|string|array select_exprs, void|string table_refs,
	       void|string|array rest, void|string select_flags)
//! Retrieves all records that matches a condition.
//!
//! This function sends a SELECT statement, and it gives full
//! expressive power to send any SELECT that is based on this table.
//!
//! The only functionality this function adds over
//! @[Sql.big_typed_query] is conversion of TIMESTAMP values (see the
//! class doc), and the (optional) handling of arbitrary properties in
//! addition to the SQL columns. @[fields] may list such properties,
//! and they are returned alongside the real columns. Properties
//! cannot be used in WHERE expressions etc, though.
//!
//! Joins with other tables are possible through @[table_refs], but
//! property columns in those tables aren't decoded.
//!
//! @param where
//! The match condition, on the form of a WHERE expression. It may be
//! given as an array to use bindings or @expr{sprintf@}-style
//! formatting - see @[handle_argspec] for details.
//!
//! A WHERE clause will always be generated, but you can put e.g.
//! "TRUE" in the match condition to select all records.
//!
//! @param fields
//! The fields to retrieve. All fields are retrieved if it's zero or
//! left out.
//!
//! For columns, the result mappings always have corresponding
//! entries. Other fields, i.e. properties, only occur in the result
//! mappings when they match fields in the @[prop_col] column.
//!
//! A @expr{0@} (zero) entry can be used in @[fields] to return all
//! properties without filtering.
//!
//! @param select_exprs
//! Optional extra select expression besides the requested columns.
//! This is just added to the list of selected columns, after a
//! separating ",".
//!
//! @[select_exprs] may be given as an array to use bindings or
//! @expr{sprintf@}-style formatting - see @[handle_argspec] for
//! details.
//!
//! Note that expressions in @[select_exprs] that produce TIMESTAMP
//! values are not converted to unix time integers - they are instead
//! returned as formatted date/time strings.
//!
//! @param table_refs
//! Optional other tables to join into the SELECT. This is inserted
//! between "FROM @[table]" and "WHERE".
//!
//! @param rest
//! Optional clauses that follows after the WHERE clause, e.g. ORDER
//! BY, GROUP BY, and LIMIT. It may be given as an array to use
//! bindings or @expr{sprintf@}-style formatting - see
//! @[handle_argspec] for details.
//!
//! @param select_flags
//! Flags for the SELECT statement. If this string is given, it is
//! simply inserted directly after the "SELECT" keyword.
//!
//! @returns
//! Returns a @[SqlTable.Result] object from which the results can be
//! retrieved. Zero is never returned.
//!
//! @note
//! The result object implements an iterator, so it can be used
//! directly in e.g. a @expr{foreach@}.
//!
//! @note
//! @[quote] may be used to quote string literals if the
//! @expr{sprintf@}-style formats aren't used.
//!
//! @seealso
//! @[select1], @[get], @[get_multi]
{
  return conn_select (get_db(), where, fields, select_exprs, table_refs,
		      rest, select_flags);
}

array select1 (string|array select_expr, string|array where,
	       void|string table_refs, void|string|array rest,
	       void|string select_flags)
//! Convenience variant of @[select] for retrieving only a single
//! column. The return value is an array containing the values in the
//! @[select_expr] column.
//!
//! @param select_expr
//! The field to retrieve. It may name a column or a property, or it
//! may be a select expression like "SHA1(x)". It may be given as an
//! array to use bindings or @expr{sprintf@}-style formatting - see
//! @[handle_argspec] for details.
//!
//! @param where
//! The match condition, on the form of a WHERE expression. A WHERE
//! clause will always be generated, but you can put e.g. "TRUE" in
//! the match condition to select all records.
//!
//! @[where] may be given as an array to use bindings or
//! @expr{sprintf@}-style formatting - see @[handle_argspec] for
//! details.
//!
//! @param table_refs
//! Optional other tables to join into the SELECT. This is inserted
//! between "FROM @[table]" and "WHERE".
//!
//! @param rest
//! Optional clauses that follows after the WHERE clause, e.g. ORDER
//! BY, GROUP BY, and LIMIT. It may be given as an array to use
//! bindings or @expr{sprintf@}-style formatting - see
//! @[handle_argspec] for details.
//!
//! @param select_flags
//! Flags for the SELECT statement. If this string is given, it is
//! simply inserted directly after the "SELECT" keyword.
//!
//! @returns
//! Returns an array with the values in the selected column. If a
//! property is retrieved and some rows don't have the wanted property
//! then @[UNDEFINED] is put into those elements.
//!
//! @seealso
//! @[select], @[get], @[get_multi]
{
  return conn_select1 (get_db(), select_expr, where, table_refs,
		       rest, select_flags);
}

mapping(string:mixed) get (mixed id, void|array(string) fields)
//! Returns the record matched by a primary key value, or zero if
//! there is no such record.
//!
//! @param id
//! The value for the primary key.
//!
//! If the table has a multicolumn primary key then @[id] must be an
//! array which has the values for the primary key columns in the same
//! order as @[pk_cols]. Otherwise @[id] is taken directly as the
//! value of the single primary key column.
//!
//! @param fields
//! The fields to retrieve. All fields are retrieved if it's zero or
//! left out.
//!
//! For columns, the result mappings always have corresponding
//! entries. Other fields, i.e. properties, only occur in the result
//! mappings when they match fields in the @[prop_col] column.
//!
//! A @expr{0@} (zero) entry can be used in @[fields] to return all
//! properties without filtering.
//!
//! @seealso
//! @[select], @[select1], @[get_multi]
{
  return conn_get (get_db(), id, fields);
}

Result get_multi (array(mixed) ids, void|array(string) fields)
//! Retrieves multiple records selected by primary key values.
//!
//! This function currently only works if the primary key is a single
//! column.
//!
//! @param id
//! Array containing primary key values.
//!
//! The number of returned records might be less than the number of
//! entries here if some of them don't match any record. Also, the
//! order of the returned records has no correlation to the order in
//! the @[id] array.
//!
//! @param fields
//! The fields to retrieve. All fields are retrieved if it's zero or
//! left out.
//!
//! For columns, the result mappings always have corresponding
//! entries. Other fields, i.e. properties, only occur in the result
//! mappings when they match fields in the @[prop_col] column.
//!
//! A @expr{0@} (zero) entry can be used in @[fields] to return all
//! properties without filtering.
//!
//! @returns
//! Returns a @[SqlTable.Result] object from which the results can be
//! retrieved. Zero is never returned.
//!
//! @note
//! The result object implements an iterator, so it can be used
//! directly in e.g. a @expr{foreach@}.
//!
//! @seealso
//! @[get], @[select], @[select1]
{
  return conn_get_multi (get_db(), ids, fields);
}

// Explicit connection handling.

int conn_insert (Sql.Sql db_conn, mapping(string:mixed)... records)
//! Like @[insert], but a database connection object is passed
//! explicitly instead of being retrieved via @[get_db].
{
  Sql.mysql conn = db_conn->master_sql;
#ifdef MYSQL_DEBUG
  if (!sizeof (records)) error ("Must give at least one record.\n");
#endif
  UPDATE_MSG ("%O: insert %O\n",
	      this, sizeof (records) == 1 ? records[0] : records);
  mapping(string:mixed) first_rec = records[0];
  conn->big_query ("INSERT `" + table + "` " +
		   make_insert_clause (records),
		   0, query_charset);
  if (!id_col) return 0;
  if (zero_type (first_rec[id_col]))
    return first_rec[id_col] = conn->insert_id();
  else
    return first_rec[id_col];
}

int conn_insert_ignore (Sql.Sql db_conn, mapping(string:mixed)... records)
//! Like @[insert_ignore], but a database connection object is passed
//! explicitly instead of being retrieved via @[get_db].
{
  Sql.mysql conn = db_conn->master_sql;
#ifdef MYSQL_DEBUG
  if (!sizeof (records)) error ("Must give at least one record.\n");
#endif
  UPDATE_MSG ("%O: insert_ignore %O\n",
	      this, sizeof (records) == 1 ? records[0] : records);
  mapping(string:mixed) first_rec = records[0];
  conn->big_query ("INSERT IGNORE `" + table + "` " +
		   make_insert_clause (records),
		   0, query_charset);
  if (!id_col) return 0;
  int last_insert_id = conn->insert_id();
  if (last_insert_id &&
      sizeof (records) == 1 && zero_type (first_rec[id_col]))
    // Only set the field if we got a single record. Otherwise we
    // don't really know which record it applies to.
    first_rec[id_col] = last_insert_id;
  return last_insert_id;
}

int conn_replace (Sql.Sql db_conn, mapping(string:mixed)... records)
//! Like @[replace], but a database connection object is passed
//! explicitly instead of being retrieved via @[get_db].
{
  Sql.mysql conn = db_conn->master_sql;
#ifdef MYSQL_DEBUG
  if (!sizeof (records)) error ("Must give at least one record.\n");
#endif
  UPDATE_MSG ("%O: replace %O\n",
	      this, sizeof (records) == 1 ? records[0] : records);
  mapping(string:mixed) first_rec = records[0];
  conn->big_query ("REPLACE `" + table + "` " +
		   make_insert_clause (records),
		   0, query_charset);
  if (!id_col) return 0;
  if (zero_type (first_rec[id_col]))
    return first_rec[id_col] = conn->insert_id();
  else
    return first_rec[id_col];
}

void conn_update (Sql.Sql db_conn, mapping(string:mixed) record,
		  void|int(0..2) clear_other_fields)
//! Like @[update], but a database connection object is passed
//! explicitly instead of being retrieved via @[get_db].
{
  Sql.mysql conn = db_conn->master_sql;
#ifdef MYSQL_DEBUG
  if (!(<0,1,2>)[clear_other_fields])
    error ("Invalid clear_other_fields flag.\n");
#endif
  UPDATE_MSG ("%O: update%s %O\n", this,
	      clear_other_fields == 2 ? " (clear all other fields)" :
	      clear_other_fields == 1 ? " (clear other props)" : "", record);

  record += ([]);
  string pk_where = make_pk_where (record);
  if (!pk_where)
    error ("The record lacks a value for a primary key column.\n");
  record = update_pack_fields (conn, record, pk_where, clear_other_fields);

  conn->big_query ("UPDATE `" + table + "` "
		   "SET " + make_set_clause (record,
					     clear_other_fields == 2) + " "
		   "WHERE " + pk_where + " "
		   "LIMIT 1",	// The limit is just extra paranoia.
		   0, query_charset);
}

int conn_insert_or_update (Sql.Sql db_conn, mapping(string:mixed) record,
			   void|int(0..2) clear_other_fields)
//! Like @[insert_or_update], but a database connection object is
//! passed explicitly instead of being retrieved via @[get_db].
{
  Sql.mysql conn = db_conn->master_sql;
#ifdef MYSQL_DEBUG
  if (!(<0,1,2>)[clear_other_fields])
    error ("Invalid clear_other_fields flag.\n");
#endif
  UPDATE_MSG ("%O: insert_or_update%s %O\n", this,
	      clear_other_fields == 2 ? " (clear all other fields)" :
	      clear_other_fields == 1 ? " (clear other props)" : "", record);

  // If we have properties to merge then do that with separate queries
  // afterwards. We never do it by first querying the properties from
  // an existing record (if any), even though that could save one
  // query. That's because the race is less bothersome if we do it
  // afterwards based on id_col than before based on pk_cols (or
  // possibly all fields if pk_cols aren't given) - in the latter case
  // we could retrieve the properties from a different record that got
  // removed in the race window. Doing it afterwards might change the
  // wrong record only if id_col isn't used.

  mapping(string:mixed) real_cols = col_types & record;
  mapping(string:mixed) other_fields = record - real_cols;
  string prop_col_value;

  if (sizeof (other_fields)) {
#ifdef MYSQL_DEBUG
    if (!prop_col) error ("Column(s) %s missing in table %O.\n",
			  String.implode_nicely (indices (other_fields)),
			  table);
#endif
    // These encoded properties are normally for the INSERT clause
    // only - not used on update. That way we can avoid the two extra
    // queries in the INSERT case, but otoh that query gets larger.
    // It's used in both cases if clear_other_fields is set, though.
    prop_col_value = get_and_merge_props (0, 0, other_fields);
  }
  else if (clear_other_fields && prop_col)
    prop_col_value = "";

  array(string) update_set = allocate (sizeof (real_cols));
  {
    int i;
    foreach (real_cols; string col;)
      update_set[i++] = "`" + col + "`=VALUES(`" + col + "`)";
  }

  if (prop_col_value && zero_type (real_cols[prop_col])) {
    if (clear_other_fields)
      update_set += ({"`" + prop_col + "`=VALUES(`" + prop_col + "`)"});
    real_cols[prop_col] = prop_col_value;
  }

  if (id_col && zero_type (real_cols[id_col]))
    update_set += ({"`" + id_col + "`=LAST_INSERT_ID(`" + id_col + "`)"});

  if (clear_other_fields == 2) {
    foreach (col_types - real_cols - ({id_col}); string col;)
      update_set += ({"`" + col + "`=DEFAULT(`" + col + "`)"});
  }

  conn->big_query ("INSERT `" + table + "` " +
		   make_insert_clause (({real_cols})) + " " +
		   (sizeof (update_set) ?
		    "ON DUPLICATE KEY UPDATE " + update_set * "," : ""),
		   0, query_charset);

  if (id_col && zero_type (record[id_col]))
    record[id_col] = conn->insert_id();

  if (sizeof (other_fields) && !clear_other_fields &&
      // affected_rows() returns 2 if a record was updated, 1 if a new
      // one was added, and 0 if nothing happened. We should merge in
      // the property changes for both 2 and 0.
      conn->affected_rows() != 1) {
    string where = make_pk_where (record + ([]));
    if (!where) {
      // There's no AUTO_INCREMENT id column and the record doesn't
      // contain values for the primary keys, so some of the columns
      // matched an existing record through another unique index.
      // Since we don't know which they are we have to match all the
      // columns in the WHERE clause. FIXME: Add tracking of unique
      // indices to avoid this.
      String.Buffer buf = String.Buffer();
      int first = 1;
      foreach (real_cols; string col_name; mixed val)
	if (col_name != prop_col) {
	  if (first) first = 0; else buf->add (" AND ");
	  buf->add ("`", col_name, "`=");
	  add_mysql_value (buf, col_name, val);
	}
    }
    update_props (conn, where, other_fields);
  }

  return id_col && record[id_col];
}

void conn_delete (Sql.Sql db_conn, string|array where, void|string|array rest)
//! Like @[delete], but a database connection object is passed
//! explicitly instead of being retrieved via @[get_db].
{
  UPDATE_MSG ("%O: delete WHERE (%O)%s\n", this, where,
	      rest ? sprintf (" %O", rest) : "");

  mapping(string|int:mixed) bindings = ([]);
  if (arrayp (where)) where = handle_argspec (where, bindings);
  if (arrayp (rest)) rest = handle_argspec (rest, bindings);
  if (!sizeof (bindings)) bindings = 0;

  db_conn->master_sql->big_query ("DELETE FROM `" + table + "` "
				  "WHERE (" + where + ") " + (rest || ""),
				  bindings);
}

void conn_remove (Sql.Sql db_conn, mixed id)
//! Like @[remove], but a database connection object is passed
//! explicitly instead of being retrieved via @[get_db].
{
  UPDATE_MSG ("%O: remove %O\n", this, id);
  Sql.mysql conn = db_conn->master_sql;
  conn->big_query ("DELETE FROM `" + table + "` "
		   "WHERE " + simple_make_pk_where (id),
		   0, query_charset);
}

void conn_remove_multi (Sql.Sql db_conn, array(mixed) ids)
//! Like @[remove_multi], but a database connection object is passed
//! explicitly instead of being retrieved via @[get_db].
{
  UPDATE_MSG ("%O: remove_multi %{%O,%}\n", this, ids);
  Sql.mysql conn = db_conn->master_sql;
  // FIXME: Split into several queries if the list is very long.
  conn->big_query ("DELETE FROM `" + table + "` "
		   "WHERE " + make_multi_pk_where (ids),
		   0, query_charset);
}

Result conn_select (Sql.Sql db_conn, string|array where,
		    void|array(string) fields, void|string|array select_exprs,
		    void|string table_refs, void|string|array rest,
		    void|string select_flags)
//! Like @[select], but a database connection object is passed
//! explicitly instead of being retrieved via @[get_db].
{
  mapping(string:mixed) bindings = ([]);
  if (arrayp (where)) where = handle_argspec (where, bindings);
  if (arrayp (select_exprs))
    select_exprs = handle_argspec (select_exprs, bindings);
  if (arrayp (rest)) rest = handle_argspec (rest, bindings);
  if (!sizeof (bindings)) bindings = 0;

  Result res = Result();

  string query = "SELECT ";
  if (select_flags) query += select_flags + " ";
  query += res->prepare_select_expr (fields, select_exprs, !!table_refs) +
    " FROM `" + table + "` " + (table_refs || "");

  res->res = db_conn->master_sql->big_typed_query (
    query + " WHERE (" + where + ") " + (rest || ""), bindings);

  return res;
}

array conn_select1 (Sql.Sql db_conn, string|array select_expr,
		    string|array where, void|string table_refs,
		    void|string|array rest, void|string select_flags)
//! Like @[select1], but a database connection object is passed
//! explicitly instead of being retrieved via @[get_db].
{
  mapping(string:mixed) bindings = ([]);
  if (arrayp (select_expr))
    select_expr = handle_argspec (select_expr, bindings);
  if (arrayp (where)) where = handle_argspec (where, bindings);
  if (arrayp (rest)) rest = handle_argspec (rest, bindings);
  if (!sizeof (bindings)) bindings = 0;

  string property;
  string col_type = col_types[select_expr];
  if (!col_type && prop_col &&
      sscanf (select_expr, "%*[^ .(]%*1[ .(]") != 2) {
    property = select_expr;
    select_expr = prop_col;
    col_type = "string";
  }

  string query = "SELECT ";
  if (select_flags) query += select_flags + " ";
  if (timestamp_cols[select_expr])
    query += "UNIX_TIMESTAMP(`" + table + "`.`" + select_expr + "`)";
  else if (col_type)
    query += "`" + table + "`.`" + select_expr + "`";
  else
    query += select_expr;
  query += " FROM `" + table + "` " + (table_refs || "");

  Sql.mysql_result res = db_conn->master_sql->big_typed_query (
    query + " WHERE (" + where + ") " + (rest || ""), bindings);

#ifdef MYSQL_DEBUG
  if (res->num_fields() != 1)
    error ("Result from %O did not contain a single field (got %d fields).\n",
	   query, res->num_fields());
#endif

  array ret = allocate (res->num_rows());

  if (property) {
    int i = 0;
    while (array(string) ent = res->fetch_row())
      ret[i++] = decode_props (ent[0], where)[property];
  }

  else {
    int i = 0;
    while (array(mixed) ent = res->fetch_row()) {
      ret[i++] = ent[0];
    }
  }

  return ret;
}

mapping(string:mixed) conn_get (Sql.Sql db_conn, mixed id,
				void|array(string) fields)
//! Like @[get], but a database connection object is passed explicitly
//! instead of being retrieved via @[get_db].
{
  Sql.mysql conn = db_conn->master_sql;

#ifdef MYSQL_DEBUG
  if (fields && !sizeof (fields)) error ("No fields selected.\n");
#endif

  int want_all = !fields;
  if (want_all) {
    if (prop_col)
      // Some dwim: Probably don't want the prop_col value verbatim as
      // well in the result.
      fields = indices (col_types - ([prop_col: ""]));
    else
      fields = indices (col_types);
  }

  mapping(string:string) real_cols = col_types & fields;
  mapping(string:string) other_fields;

  if (sizeof (real_cols) < sizeof (fields)) {
    if (!has_value (fields, 0)) {
      mapping(string:string) field_map = mkmapping (fields, fields);
      other_fields = field_map - real_cols;
    }
#ifdef MYSQL_DEBUG
    if (!prop_col)
      error ("Requested nonexisting column(s) %s.\n",
	     String.implode_nicely (indices (other_fields ||
					     ([prop_col: ""]))));
#endif
  }
  else if (!want_all)
    other_fields = ([]);

  int exclude_prop_col;
  if (prop_col && !equal (other_fields, ([]))) {
    if (!real_cols[prop_col]) exclude_prop_col = 1;
    real_cols[prop_col] = "string";
  }

  array(string) real_col_names = indices (real_cols);
  array(string) real_col_types = values (real_cols);

  array(string) select_cols = allocate (sizeof (real_col_types));
  foreach (real_col_names; int i; string name) {
    if (timestamp_cols[name])
      select_cols[i] = "UNIX_TIMESTAMP(`" + name + "`)";
    else
      select_cols[i] = "`" + name + "`";
  }

  string pk_where = simple_make_pk_where (id);
  Mysql.mysql_result res =
    conn->big_typed_query ("SELECT " + (select_cols * ",") + " "
			   "FROM `" + table + "` "
			   "WHERE " + pk_where,
			   0, query_charset);

  array(string) ent = res->fetch_row();
  if (!ent) return 0;

  mapping(string:mixed) rec = ([]);

  foreach (ent; int i; string|int|float|Sql.Null val) {
    rec[real_col_names[i]] = val;
  }

  if (prop_col && !equal (other_fields, ([]))) {
    string prop_val = rec[prop_col];
    if (prop_val != 0 && prop_val != "") {
      mapping(string:mixed) props = decode_props (prop_val, pk_where);
      if (other_fields) props = other_fields & props;
      // Properties take precedence over columns. That allows gradual
      // migration to columns, since the field in prop_col will
      // disappear in the next update.
      rec += props;
    }
    if (exclude_prop_col) m_delete (rec, prop_col);
  }

  return rec;
}

Result conn_get_multi (Sql.Sql db_conn, array(mixed) ids,
		       void|array(string) fields)
//! Like @[get_multi], but a database connection object is passed
//! explicitly instead of being retrieved via @[get_db].
{
  Sql.mysql conn = db_conn->master_sql;
  Result res = Result();
  // FIXME: Split into several queries if the list is very long.
  res->res =
    conn->big_typed_query ("SELECT " +
			   res->prepare_select_expr (fields, 0, 0) +
			   " FROM `" + table + "` "
			   "WHERE " + make_multi_pk_where (ids),
			   0, query_charset);
  return res;
}

// The Result object.

class Result
//! Result object returned by e.g. @[select]. This is similar in
//! function to an @[Sql.sql_result] object. It also implements the
//! iterator interface and can therefore be used directly in e.g.
//! @expr{foreach@}.
{
  Sql.mysql_result res;
  //! The underlying result object from the db connection.

  protected array(string) real_col_names;
  // The names of the sql columns to retrieve. Initially this contains
  // only the column names built from fields, not the names of the
  // user supplied select expressions. The latter are added when the
  // first row is fetched.
  //
  // If properties are being fetched but not the property column
  // itself then real_col_names has a zero in the prop_pos position.

  protected int add_select_expr_names;
  // Nonzero if there are user supplied select expressions and their
  // names have not yet been added to real_col_names.

  protected int prop_pos;
  // Position of the prop_col values in the result arrays, or -1 if we
  // shouldn't handle that.

  protected mapping(string:string) other_fields;
  // Selected fields in the prop_col values, or zero if everything
  // is wanted. The values are insignificant.

  protected int cur_row;
  protected mapping(string:mixed) cur_rec;
  // Need to store the last fetched result for the iterator interface
  // since fetch() advances the cursor, while the iterator's index()
  // and value() must not. These are updated by fetch(), and cur_row
  // is the row index of cur_rec, _not_ the next one.

  protected string _sprintf (int flag)
  {
    return flag == 'O' &&
      sprintf ("SqlTable.Result(%O, %d/%d)",
	       global::this && table, cur_row, res && res->num_rows());
  }

  int num_rows() {return res->num_rows();}
  //! Returns the number of rows in the result.

  int eof() {return res->eof();}
  //! Returns nonzero if there are no more rows.

  array(mapping(string:mixed)) column_info() {return res->fetch_fields();}
  //! Returns information about the columns in the result.

  mapping(string:mixed) fetch()
  //! Fetches the next record from the result and advance the cursor.
  //! Returns zero if there are no more records.
  //!
  //! The record is returned as a mapping. It is similar to the
  //! mappings returned by @[Sql.query], except that native pike types
  //! and @[Val.null] are used. If @[prop_col] is used then properties
  //! from that column can be returned as mapping entries alongside
  //! the columns, and those values can be any pike data type.
  //!
  //! As opposed to the @[Sql.query] mappings, the returned mapping
  //! has a single entry for each field - there are no duplicate
  //! entries prefixed with the table name.
  {
    if (cur_rec) cur_row++;

    array(string) ent = res->fetch_row();
    if (!ent) return cur_rec = 0;

    mapping(string:mixed) rec;

    if (add_select_expr_names) {
      // Fetch the names of the user supplied select expressions from
      // the result.
      array(mapping(string:mixed)) field_info =
	res->fetch_fields()[sizeof (real_col_names)..];
      real_col_names += column (field_info, "name");
      add_select_expr_names = 0;
    }

    rec = mkmapping (real_col_names, ent);

    if (prop_pos != -1) {
      m_delete (rec, 0);
      if (!(<0, "">)[ent[prop_pos]]) {
	mapping(string:mixed) props = decode_props (ent[prop_pos], 0);
	if (other_fields) props = other_fields & props;
	// Properties take precedence over columns. That allows
	// gradual migration to columns, since the field in prop_col
	// will disappear in the next update.
	rec += props;
      }
    }

    return cur_rec = rec;
  }

  array(mapping(string:mixed)) get_array()
  //! Returns all the remaining records as an array of mappings.
  //! @[eof] returns true after this.
  //!
  //! @note
  //! This is not a cast since it destructively modifies this object
  //! by fetching all remaining records.
  {
    array(mapping(string:mixed)) res = ({});
    while (mapping(string:mixed) rec = fetch())
      res += ({rec});
    return res;
  }

  string prepare_select_expr (array(string) fields, string select_exprs,
			      int with_table_qualifiers)
  // Internal function to initialize all the variables from an
  // optional array of requested fields.
  {
#ifdef MYSQL_DEBUG
    if (fields && !sizeof (fields)) error ("No fields selected.\n");
#endif

    string select_clause;
    string tbl_qual = with_table_qualifiers ? table + "`.`" : "";
    int want_all = !fields;

    prop_pos = -1;

    if (want_all) {
      if (prop_col)
	// Some dwim: Probably don't want the prop_col value verbatim
	// as well in the result.
	fields = indices (col_types - ([prop_col: ""]));
      else
	fields = indices (col_types);
    }

    mapping(string:string) real_cols = col_types & fields;
    if (sizeof (real_cols)) {
      mapping(string:int(1..1)) ts_cols = timestamp_cols & fields;
      if (sizeof (ts_cols)) {
	real_col_names = indices (ts_cols);
	select_clause =
	  "UNIX_TIMESTAMP(`" + tbl_qual +
	  (real_col_names * ("`),UNIX_TIMESTAMP(`" + tbl_qual)) + "`)";
	if (sizeof (ts_cols) < sizeof (real_cols)) {
	  array(string) normal_col_names = indices (real_cols - ts_cols);
	  real_col_names += normal_col_names;
	  select_clause +=
	    ",`" + tbl_qual + (normal_col_names * ("`,`" + tbl_qual)) + "`";
	}
      }
      else {
	real_col_names = indices (real_cols);
	select_clause =
	  "`" + tbl_qual + (real_col_names * ("`,`" + tbl_qual)) + "`";
      }
    } else {
      real_col_names = ({});
    }

    if (sizeof (real_cols) < sizeof (fields)) {
      if (!has_value (fields, 0)) {
	mapping(string:string) field_map = mkmapping (fields, fields);
	other_fields = field_map - real_cols;
      }
#ifdef MYSQL_DEBUG
      if (!prop_col)
	error ("Requested nonexisting column(s) %s.\n",
	       String.implode_nicely (indices (other_fields ||
					       ([prop_col: ""]))));
#endif
    }
    else if (!want_all)
      other_fields = ([]);

    if (prop_col && !equal (other_fields, ([]))) {
      if (real_cols[prop_col])
	prop_pos = search (real_col_names, prop_col);
      else {
	prop_pos = sizeof (real_col_names);
	if (select_clause) select_clause += ",`" + tbl_qual + prop_col + "`";
	else select_clause = "`" + tbl_qual + prop_col + "`";
	real_col_names += ({0});
      }
    }

    if (select_exprs) {
      add_select_expr_names = 1;
      if (select_clause) select_clause += ", " + select_exprs;
      else select_clause = select_exprs;
    }

    return select_clause || "";
  }

  // Iterator interface. This is a separate object only to avoid
  // implementing a `! in Result, which would make it behave oddly.

#ifdef MYSQL_DEBUG
  protected int got_iterator;
#endif

  Iterator _get_iterator()
  //! Returns an iterator for the result. Only one iterator may be
  //! created per @[Result] object.
  {
#ifdef MYSQL_DEBUG
    if (got_iterator)
      error ("Cannot create more than one iterator for a Result object.\n");
    got_iterator = 1;
#endif
    return Iterator (num_rows());
  }

  protected class Iterator (protected int cached_num_rows)
  {
    protected int `!() {return cur_row >= cached_num_rows;}
    protected int _sizeof() {return cached_num_rows;}

    protected string _sprintf (int flag)
    {
      return flag == 'O' &&
	sprintf ("SqlTable.Result.Iterator(%O, %d/%d)",
		 Result::this && global::this && table,
		 Result::this && cur_row, cached_num_rows);
    }

    int index()
    {
      return cur_row < cached_num_rows ? cur_row : UNDEFINED;
    }

    mapping(string:mixed) value()
    {
      if (!cur_rec) fetch();
      return cur_row < cached_num_rows ? cur_rec : UNDEFINED;
    }

    int next()
    {
      fetch();
      return cur_row < cached_num_rows;
    }

    protected Iterator `+= (int steps)
    // Old interface for pike 7.4 compat.
    {
      if (steps < 0) error ("Stepping backwards not supported.\n");
      while (steps--) if (!next()) break;
      return this;
    }
  }
}

// Internals.

protected string format_rec_compact (mapping(string:mixed) rec)
{
  array(string) vars = indices (rec);
  array(mixed) vals = values (rec);
  sort (vars, vals);
  array(string) rows = allocate (sizeof (vars));
  foreach (vals; int i; mixed val) {
    if (stringp (val) || arrayp (val) || mappingp (val))
      rows[i] = sprintf ("  %O: %t[%d],\n", vars[i], val, sizeof (val));
    else
      rows[i] = sprintf ("  %O: %O,\n", vars[i], val);
  }
  return "([\n" + rows * "" + "])";
}

protected mapping(string:mixed) decode_props (string prop_val, string where)
{
  if (prop_val != 0 && prop_val != "") {
    mixed decoded;
    mixed err = catch (decoded = decode_value (prop_val));
    if (err || !mappingp (decoded))
      // This is bad, but the best we can do is to report it on stderr
      // and continue so that the bogus value gets flushed out and the
      // app can recover somewhat. FIXME: Maybe add a flag to control
      // what to do here.
      werror ("WARNING! Failed to decode property value "
	      "in `%s` from `%s`%s: %s"
	      "The garbled property value is: %O\n"
	      "It will be ignored and overwritten.\n",
	      prop_col, table, where ? " where " + where : "",
	      (err ? describe_error (err) :
	       sprintf ("Expected mapping, got %O.\n", decoded)),
	      prop_val);
    else
      return [mapping(string:mixed)] decoded;
  }
  return ([]);
}

protected void add_mysql_value (String.Buffer buf, string col_name, mixed val)
// A value with zero_type is formatted as "DEFAULT".
{
  if (stringp (val)) {
#ifdef MYSQL_DEBUG
    if (col_types[col_name] != "string")
      error ("Got string value %q for %s column `%s`.\n",
	     val, col_types[col_name], col_name);
#endif
    if (String.width (val) == 8)
      // _latin1 works fine for binary data since the actual charset
      // isn't significant. The only problem is for 8-bit text where
      // mysql latin1 has chars in the control range 0x80..0x9f. Since
      // those control chars are very uncommon in text we just ignore
      // that problem for now.
      buf->add ("_latin1\"", quote (val), "\"");
    else
      // FIXME: If the column holds binary data we should throw an
      // error here instead of sending what is effectively garbled
      // data.
      buf->add ("_utf8\"", string_to_utf8 (quote (val)), "\"");
  }
  else if (intp (val)) {
    if (zero_type (val))
      buf->add ("DEFAULT");
    else {
#ifdef MYSQL_DEBUG
      if (col_types[col_name] != "int" && !datetime_cols[col_name])
	error ("Got integer value %O for %s column `%s`.\n",
	       val, col_types[col_name] || "string", col_name);
#endif
      if (timestamp_cols[col_name])
	buf->add ("FROM_UNIXTIME(", (string) val, ")");
      else
	buf->add ((string) val);
    }
  }
  else if (val == Val.null)
    buf->add ("NULL");
  else {
#ifdef MYSQL_DEBUG
    if (objectp (val) && functionp (val->den)) {
      // Allow Gmp.mpq for float fields, and for int fields if they
      // have no fractional part.
      if (col_types[col_name] != "float" &&
	  !(val->den() == 1 && col_types[col_name] == "int"))
	error ("Got %O for %s column `%s`.\n",
	       val, col_types[col_name] || "string", col_name);
    }
    else {
      if (!floatp (val))
	error ("Cannot use %t value for `%s`: %O\n", val, col_name, val);
      if (col_types[col_name] != "float")
	error ("Got float value %O for %s column `%s`.\n",
	       val, col_types[col_name] || "string", col_name);
    }
#endif
    buf->add ((string) val);
  }
}

protected string make_insert_clause (array(mapping(string:mixed)) records)
// Returns an "(a,b,c) VALUES (1,2,3),(4,5,6)" clause as used in
// INSERT statements. Fields that don't have columns are packed into
// prop_col updates. Destructive on the records array, but not on the
// mapping elements.
{
  mapping(string:mixed) query_cols = ([]); // Only indices are relevant.

  // FIXME: Ought to use bindings, but Mysql.mysql doesn't support it
  // yet (as of pike 7.8.191).

#ifdef MYSQL_DEBUG
  if (!sizeof (records)) error ("Must give at least one record.\n");
#endif

  foreach (records; int i; mapping(string:mixed) rec) {
    mapping(string:mixed) real_cols = col_types & rec;
    mapping(string:mixed) other_fields = rec - real_cols;

    if (sizeof (other_fields)) {
#ifdef MYSQL_DEBUG
      if (!prop_col) error ("Column(s) %s missing.\n",
			    String.implode_nicely (indices (other_fields)));
#endif

      string encoded_props;
      if (mixed err = catch {
	  encoded_props = encode_value_canonic (other_fields);
	})
	error ("Failed to encode properties: %sThe properties are: %O\n",
	       describe_error (err), other_fields);

      if (sizeof (encoded_props) > prop_col_max_length)
	error ("Property value too long (%d b - max is %d b) "
	       "for `%s` in `%s` - failed to insert record: %s\n",
	       sizeof (encoded_props), prop_col_max_length, prop_col, table,
	       format_rec_compact (rec));
      real_cols[prop_col] = encoded_props;
    }

    query_cols |= real_cols;
    records[i] = real_cols;
  }

  array(string) col_list = indices (query_cols);
  String.Buffer buf = String.Buffer();
  if (sizeof (col_list))
    buf->add ("(`", (col_list * "`,`"), "`) VALUES ");
  else
    buf->add ("() VALUES ");

  int first = 1;
  foreach (records, mapping(string:mixed) rec) {
    if (first) first = 0; else buf->add (",");
    buf->add ("(");

    int col_first = 1;
    foreach (map (col_list, rec); int i; mixed val) {
      if (col_first) col_first = 0; else buf->add (",");
      add_mysql_value (buf, col_list[i], val);
    }

    buf->add (")");
  }

  return buf->get();
}

protected string make_pk_where (mapping(string:mixed) rec)
// Returns a WHERE expression like "a=1 AND b=2" for matching the
// primary key, or zero if the record doesn't have values for all pk
// columns. The pk fields are also removed from the rec mapping.
{
#ifdef MYSQL_DEBUG
  if (!sizeof (pk_cols)) error ("There is no primary key in this table.\n");
#endif
  String.Buffer buf = String.Buffer();
  int first = 1;
  foreach (pk_cols, string pk_col) {
    if (first) first = 0; else buf->add (" AND ");
    mixed val = m_delete (rec, pk_col);
    if (zero_type (val) || val == Val.null)
      return 0;
    buf->add ("`", pk_col, "`=");
    add_mysql_value (buf, pk_col, val);
  }
  return buf->get();
}

protected string simple_make_pk_where (mixed id)
// Returns a WHERE expression like "a=1 AND b=2" for matching the
// primary key. id is like the argument to get().
{
#ifdef MYSQL_DEBUG
  if (!sizeof (pk_cols)) error ("There is no primary key in this table.\n");
#endif

  String.Buffer buf = String.Buffer();
  if (sizeof (pk_cols) == 1) {
    buf->add ("`", pk_cols[0], "`=");
#ifdef MYSQL_DEBUG
    if (id == Val.null)
      error ("Cannot use Val.null for primary key column %O.\n",
	     pk_cols[0]);
#endif
    add_mysql_value (buf, pk_cols[0],
		     id || 0); // Clear any zero_type in id.
  }

  else {
#ifdef MYSQL_DEBUG
    if (!arrayp (id) || sizeof (id) != sizeof (pk_cols))
      error ("The id must be an array with %d elements.\n", sizeof (pk_cols));
#endif
    int first = 1;
    foreach (pk_cols; int i; string pk_col) {
      if (first) first = 0; else buf->add (" AND ");
      buf->add ("`", pk_cols[0], "`=");
#ifdef MYSQL_DEBUG
      if (id[i] == Val.null)
	error ("Cannot use Val.null for primary key column %O.\n", pk_col);
#endif
      add_mysql_value (buf, pk_col,
		       id[i] || 0); // Clear any zero_type in id[i].
    }
  }

  return buf->get();
}

protected string make_multi_pk_where (array(mixed) ids, int|void negated)
// Returns a WHERE expression like "foo IN (2,3,17,4711)" for matching
// a bunch of records by primary key.
{
#ifdef MYSQL_DEBUG
  if (sizeof (pk_cols) != 1)
    error ("The table must have a single column primary key.\n");
#endif

  if (!sizeof (ids)) return negated?"TRUE":"FALSE";

  string pk_col = pk_cols[0];
  string pk_type = col_types[pk_col];

  string optional_not = negated?" NOT ":"";

  if ((<"float", "int">)[pk_type]) {
#ifdef MYSQL_DEBUG
    foreach (ids; int i; mixed id)
      if (!intp (id) && !floatp (id))
	error ("Expected numeric value for primary key column %O, "
	       "got %t at position %d.\n",
	       pk_col, id, i);
#endif
    return (timestamp_cols[pk_col] ?
	    "UNIX_TIMESTAMP(`" + pk_col + "`)" : "`" + pk_col + "`") +
      optional_not + " IN (" + (((array(string)) ids) * ",") + ")";
  }

  else {
#ifdef MYSQL_DEBUG
    foreach (ids; int i; mixed id)
      if (!stringp (id))
	error ("Expected string value for primary key column %O, "
	       "got %t at position %d.\n",
	       pk_col, id, i);
#endif
    String.Buffer buf = String.Buffer();
    buf->add ("`", pk_col, "` ", optional_not, " IN (");
    int first = 1;
    foreach (ids, string id) {
      if (first) first = 0; else buf->add (",");
      add_mysql_value (buf, pk_col, id);
    }
    buf->add (")");
    return buf->get();
  }
}

protected string get_and_merge_props (Sql.mysql conn, string pk_where,
				      mapping(string:mixed) prop_changes)
// Retrieves the current properties for a record (if pk_where is set),
// merges prop_changes into them, and returns the new value to assign
// to the properties column.
{
  mapping(string:mixed) old_props;
  if (array(string) ent = pk_where &&
      conn->big_query ("SELECT `" + prop_col + "` "
		       "FROM `" + table + "` "
		       "WHERE " + pk_where,
		       0, query_charset)->fetch_row())
    old_props = decode_props (ent[0], pk_where);
  else
    old_props = ([]);

  mapping(string:mixed) rem_props = filter (prop_changes, `==, Val.null);
  mapping(string:mixed) new_props = old_props + prop_changes - rem_props;

  string encoded_props;
  if (mixed err = catch {
      encoded_props = encode_value_canonic (new_props);
    })
    error ("Failed to encode properties: %sThe properties are: %O\n",
	   describe_error (err), new_props);

  if (sizeof (encoded_props) > prop_col_max_length)
    error ("Property value too long (%d b - max is %d b) "
	   "for `%s` in `%s`%s: %s",
	   sizeof (encoded_props), prop_col_max_length,
	   prop_col, table, pk_where ? " where " + pk_where : "",
	   format_rec_compact (new_props));

  return encoded_props;
}

protected void update_props (Sql.mysql conn, string pk_where,
			     mapping(string:mixed) prop_changes)
// Updates the properties column by retrieving the current properties
// and merging prop_changes into them. prop_changes is assumed to only
// contain properties.
{
  string encoded_props = get_and_merge_props (conn, pk_where, prop_changes);
  conn->big_query ("UPDATE `" + table + "` "
		   "SET `" + prop_col + "`="
		   "_binary\"" + quote (encoded_props) + "\" "
		   "WHERE " + pk_where + " "
		   "LIMIT 1",	// In case the WHERE condition is bad.
		   0, query_charset);
}

protected mapping(string:mixed) update_pack_fields (
  Sql.mysql conn, mapping(string:mixed) rec, string pk_where,
  int replace_properties)
// Returns a record mapping where all fields that don't have columns
// have been packed into a prop_col entry. The table is queried if
// necessary to obtain the old prop_col fields for merging.
{
  mapping(string:mixed) real_cols = col_types & rec;
  mapping(string:mixed) other_fields = rec - real_cols;

  if (sizeof (other_fields)) {
#ifdef MYSQL_DEBUG
    if (!prop_col) error ("Column(s) %s missing in table %O.\n",
			  String.implode_nicely (indices (other_fields)),
			  table);
#endif
    if (replace_properties) pk_where = 0;
    else if (!pk_where) pk_where = make_pk_where (rec + ([]));
    real_cols[prop_col] = get_and_merge_props (conn, pk_where, other_fields);
  }

  else if (replace_properties && prop_col)
    real_cols[prop_col] = "";

  return real_cols;
}

protected string make_set_clause (mapping(string:mixed) rec,
				  int set_all_columns)
// Returns a SET clause like "a=1,b=2" as is used in an UPDATE
// statement. All fields are assumed to have columns. If
// set_all_columns is given then all unmentioned columns, except the
// primary key columns, are set to their default values.
{
  String.Buffer buf = String.Buffer();

  int first = 1;
  foreach (rec; string col; mixed val) {
    if (first) first = 0; else buf->add (",");
    buf->add ("`", col, "`=");
    add_mysql_value (buf, col, val);
  }

  if (set_all_columns)
    foreach (col_types - rec - pk_cols; string col;) {
      if (first) first = 0; else buf->add (",");
      buf->add ("`", col, "`=DEFAULT(`", col, "`)");
    }

  return buf->get();
}
