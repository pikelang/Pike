#pike __REAL_VERSION__

//#define SP_DEBUG	1
//#define SP_DEBUGMORE	1

#ifdef SP_DEBUG
#define PD(X ...)            werror(X)
                             // PT() puts this in the backtrace
#define PT(X ...)            (lambda(object _this){(X);}(this))
#else
#undef SP_DEBUGMORE
#define PD(X ...)            0
#define PT(X ...)            (X)
#endif

//! This class is the base class for promise based SQL queries.
//! @[future()] will return a future which carries @[FutureResult]
//! objects to contain the result of the query.
//!
//! @seealso
//!  @[FutureResult], @[Connection.promise_query()]

inherit Concurrent.Promise;

private .FutureResult res;
private int minresults, maxresults, discardover;
private function(array, .Result, array :array) map_cb;

private void failed(mixed msg) {
  res->_dblink = 0;				// Release reference
  res->status_command_complete = arrayp(msg) ? msg : ({msg, backtrace()});
  PD("Future failed %O %s\n",
   res->query, describe_backtrace(res->status_command_complete));
  failure(res);
}

private void succeeded(.Result result) {
  PD("Future succeeded %O\n", res->query);
  if (discardover >= 0)
    res->data = res->data[.. discardover];
  mixed err =
    catch {
      result->eof();				// Collect any SQL errors
      res->fields = result->fetch_fields();
      res->affected_rows = result->affected_rows();
      res->status_command_complete = result->status_command_complete();
    };
  if (err)
    failed(err);
  else
    success(res);
}

private void result_cb(.Result result, array(array(mixed)) rows) {
  PD("Callback got %O\n", rows);
  if (rows) {
    if (map_cb) {
      mixed err = catch(rows
       = predef::filter(predef::map(rows, map_cb, result, res->data),
          arrayp));
      if (err)
        failed(err);
    }
    if (maxresults >= 0 && result->num_rows() > maxresults)
      failed(sprintf("Too many records returned: %d > %d\n%O",
                          result->num_rows(), maxresults, result));
    if (discardover >= 0) {
      int room = discardover - sizeof(res->data);
      if (room >= 0)
        res->data += rows[.. room - 1];
    } else
      res->data += rows;
  } else if (sizeof(res->data) >= minresults)
    succeeded(result);
  else					// Collect any SQL errors first
    failed(catch(result->eof()) ||
      sprintf("Insufficient number of records returned\n%O", result));
}

//! @param min
//!   If the query returns less than this number of records, fail the
//!   future.  Defaults to @expr{0@}.
final this_program min_records(int(0..) min) {
  minresults = min;
  return this;
}

//! @param max
//!   If the query returns more than this number of records, fail the
//!   future. @expr{-1@} means no maximum (default).
final this_program max_records(int(-1..) max) {
  maxresults = max;
  return this;
}

//! @param over
//!   Discard any records over this number.  @expr{-1@} means do not discard
//!   any records (default).
final this_program discard_records(int(-1..) over) {
  discardover = over;
  return this;
}

protected
 void create(.Connection db, string q, mapping(string:mixed) bindings,
             function(array, .Result, array :array) map_cb) {
  PD("Create future %O %O %O\n", db, q, bindings);
  this::map_cb = map_cb;
  res = .FutureResult(db, q, bindings);
  discardover = maxresults = -1;
  if (res->status_command_complete = catch(db->streaming_typed_query(q, bindings)
                                    ->set_result_array_callback(result_cb)))
    failed(res->status_command_complete);
  ::create();
}

#ifdef SP_DEBUG
protected void _destruct() {
  PD("Destroy promise %O %O\n", query, bindings);
  ::_destruct();
}
#endif
