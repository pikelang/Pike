#pike __REAL_VERSION__

//#define SP_DEBUG      1
//#define SP_DEBUGMORE  1

#ifdef SP_DEBUG
#define PD(X ...)            werror(X)
                             // PT() puts this in the backtrace
#define PT(X ...)            (lambda(object _this){(X);}(this))
#else
#undef SP_DEBUGMORE
#define PD(X ...)            0
#define PT(X ...)            (X)
#endif

//! The promise result class; it will contain the results of a query.
//!
//! @seealso
//!   @[Promise], @[Connection.promise_query()]

//! The returned raw unadorned records, all typed data.
//! Once @[get()] has been accessed, @[data]
//! will point to the same adorned records.
//!
//! @seealso
//!   @[Sql.Connection->big_typed_query()]
final array(mixed) data;

//! @returns
//! The returned labeled records, all typed data.
//!
//! @seealso
//!   @[Sql.Connection->query()], @[data]
final array(mapping(string:mixed)) get() {
  if (_dblink) {
    data = _dblink->res_obj_to_array(data, fields);
    _dblink = 0;
  }
  return data;
}

//! The SQL query.
final string query;

//! The parameter bindings belonging to the query.
final mapping(string:mixed) bindings;

//! The status of the completed command.
//! If the command is still in progress, the value is @expr{null@}.
//! If an error has occurred, it contains the backtrace of that error.
final string|mixed status_command_complete;

//! The number of affected rows.
final int affected_rows;

//! The description of the fields in a record.
final array(mapping(string:mixed)) fields;

final .Connection _dblink;

protected string _sprintf(int type) {
  string res;
  switch(type) {
    case 'O':
      res = status_command_complete && arrayp(status_command_complete)
        ? status_command_complete[0]
        : sprintf("FutureResult from query: %O, bindings: %O\n"
                  "recordcount: %d\nSQL status: %s",
                      query, bindings, sizeof(data),
                      status_command_complete || "still running...");
      break;
  }
  return res;
}

protected
 void create(.Connection db, string q, mapping(string:mixed) bindings) {
  PD("Create future result %O %O %O\n", db, q, bindings);
  query = q;
  this::bindings = bindings;
  data = ({});
  _dblink = db;
}

#ifdef SP_DEBUG
protected void _destruct() {
  PD("Destroy result %O %O\n", query, bindings);
}
#endif
