#pike __REAL_VERSION__

//#define SP_DEBUG	1

#ifdef SP_DEBUG
#define PD(X ...)            werror(X)
                             // PT() puts this in the backtrace
#define PT(X ...)            (lambda(object _this){(X);}(this))
#else
#undef SP_DEBUGMORE
#define PD(X ...)            0
#define PT(X ...)            (X)
#endif

//! Sql.FutureResult promise result class.

//! The returned raw unadorned records, all typed data.
//! Once @[data] has been accessed, @[raw_data]
//! will point to the same adorned records.
//!
//! @seealso
//!   @[Sql.Connection->big_typed_query()]
final array(mixed) raw_data;

//! The returned labeled records, all typed data.
//!
//! @seealso
//!   @[Sql.Connection->query()], @[raw_data]
final array(mapping(string:mixed)) `data() {
  if (dblink) {
    raw_data = dblink->res_obj_to_array(raw_data, fields);
    dblink = 0;
  }
  return raw_data;
}

//! The SQL query.
final string query;

//! The parameter bindings belonging to the query.
final mapping(string:mixed) bindings;

//! The status of the completed command
final string status_command_complete;

//! The number of affected rows.
final int affected_rows;

//! The description of the fields in a record.
final array(mapping(string:mixed)) fields;

//! The backtrace of the error that occurred.
final mixed exception;

private .Connection dblink;
private int minresults;
private int|Concurrent.Promise promise;

protected string _sprintf(int type) {
  string res;
  switch(type) {
    case 'O':
      res = sprintf("FutureResult from query: %O, bindings: %O\n"
                    "recordcount: %d, truth: %O",
       query, bindings, sizeof(raw_data), promise);
      break;
  }
  return res;
}

private void failed() {
  dblink = 0;				// Release reference
  if (!exception)
    exception = ({0, backtrace()});
  PD("Future failed %O %s\n", query, describe_backtrace(exception));
  if (intp(promise))
    error("Future already determined\n");
  else {
    Concurrent.Promise p = promise;
    promise = -1;
    p->failure(this);
  }
}

private void succeeded(.Result result) {
  PD("Future succeeded %O\n", query);
  exception =
    catch {
      fields = result->fetch_fields();
      affected_rows = result->affected_rows();
      status_command_complete = result->status_command_complete();
    };
  if (exception)
    failed();
  else if (intp(promise))
    error("Future already determined\n");
  else {
    Concurrent.Promise p = promise;
    promise = 1;
    p->success(this);
  }
}

//!  @tt{True@} on success, @tt{false@} otherwise.
public bool `ok() {
  PD("Future evaluated %O\n", promise);
  if (objectp(promise))
    error("Future undecided\n");
  return promise > 0;
}

private void result_cb(.Result result, array(array(mixed)) rows) {
  PD("Callback got %O\n", rows);
  if (rows)
    switch (minresults) {
      case 1:
        raw_data = ({rows[1]});
      case 0:
        succeeded(result);
        break;
      default:
        raw_data += rows;
    }
  else
    switch (minresults) {
      case 2:
        if (sizeof(raw_data))
          succeeded(result);
      case 1:
        failed();
        break;
      case 3:
        succeeded(result);
    }
}

protected void create(.Connection db, Concurrent.Promise p,
 string q, mapping(string:mixed) bindings,
 int results) {
  PD("Create future %O %O %O\n", db, q, bindings);
  query = q;
  this::bindings = bindings;
  raw_data = ({});
  minresults = results;
  promise = p;
  dblink = db;
  if (exception = catch(db->big_typed_query(q, bindings)
                      ->set_result_array_callback(result_cb)))
    failed();
}

protected void _destruct() {
  PD("Destroy future %O %O\n", query, bindings);
  if (objectp(promise))
    failed();
}
