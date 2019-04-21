//! Allows for interactive debugging and live data structure inspection
//! in both single- and multi-threaded programs.
//! Creates an independent background thread that every @[pollinterval]
//! will show a list of running threads.
//! Optionally, a @[triggersignal] can be specified which allows the dump to
//! be triggered by a signal.
//!
//! Example:
//!  In the program you'd like to inspect, insert the following one-liner:
//! @code
//!      Debug.Inspect("/tmp/test.pike");
//! @endcode
//!  Then start the program and keep it running.
//!  Next you create a /tmp/test.pike with the following content:
//!@code
//!void create() {
//!  werror("Only once per modification of test.pike\n");
//!}
//!
//!int main() {
//!  werror("This will run every iteration\n");
//!  werror("By returning 1 here, we disable the stacktrace dumps\n");
//!  return 0;
//!}
//!
//!void _destruct() {
//!  werror("_destruct() runs just as often as create()\n");
//!}
//!@endcode
//!  Whenever you edit /tmp/test.pike, it will automatically reload
//!  the file.

#pike __REAL_VERSION__

#pragma dynamic_dot

#define POLLINTERVAL 4

//! Starts up the background thread.
//!
//! @param cb
//! Specifies either the callback function which is invoked on each iteration,
//! or the
//! name of a file which contains a class which is (re)compiled automatically
//! with an optional @expr{main()@} method, which will be called on each
//! iteration.
//! If the @expr{main()@} method returns 0, new stacktraces will be dumped
//! every iteration; if it returns 1, stacktrace dumping will be suppressed.
//!
//! @note
//!   The compilation and the running of the callback is guarded by
//!   a catch(), so that failures (to compile) in that section will not
//!   interfere with the running program.
//!
//! @note
//!   If the list of running threads did not change, displaying the list again
//!   will be suppressed.
//!
//! @seealso
//!  @[triggersignal], @[pollinterval], @[_loopthread], @[_callback],
//!  @[Debug.globals]
protected void create(string|function(void:void)|void cb) {
  _callback = cb;
  Thread.Thread(loop);
}

private Thread.Mutex running = Thread.Mutex();
private string lasttrace = "";
private object runit;
private int lastmtime;
private int skipnext;
private int oldsignal;

//! If assigned to, it will allow the diagnostics inspection to be triggered
//! by this signal.
int triggersignal;

//! The polling interval in seconds, defaults to 4.
int pollinterval = POLLINTERVAL;

//! The inspect-thread.  It will not appear in the displayed thread-list.
Thread.Thread _loopthread;

//! Either the callback function which is invoked on each iteration, or the
//! name of a file which contains a class which is (re)compiled automatically
//! and called on each iteration.
//!
//! @seealso
//!  @[create]
string|function(void:void) _callback;

private void loop(int sig) {
  _loopthread = Thread.this_thread();
  for(;; inspect()) {
    sleep(pollinterval, 1);
    if (triggersignal != oldsignal) {
      werror("\nTo inspect use: kill -%d %d\n\n",
       triggersignal, getpid());
      signal(triggersignal, inspect);
    }
  }
}

//! The internal function which does all the work each pollinterval.
//! Run it directly to force a thread-dump.
void inspect() {
  Thread.MutexKey lock;
  if (lock = running->trylock(1)) {
    Thread.Thread thisthread = Thread.this_thread();
    if (!skipnext) {
      int i = 0;
      String.Buffer buf = String.Buffer();
      buf.add("\n{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{\n");
      foreach(Thread.all_threads(); ; Thread.Thread ct)
        if (ct != thisthread && ct != _loopthread)
          buf.sprintf("====================================== Thread: %d\n%s\n",
           i++, describe_backtrace(ct.backtrace(), -1));
      buf.add("}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}\n");
      string s = buf.get();
      if (s != lasttrace)		    // I shall say this only wence!
        werror(lasttrace = s);
    }
    if (_callback) {
      mixed err = catch {
        if (stringp(_callback)) {
          Stdio.Stat st = file_stat(_callback);
          if (st) {
            if (st->mtime > lastmtime) {
              lastmtime = st->mtime;
              runit = 0;	// Explicitly, so that the destructor is called
              runit = compile_file(_callback)();
              skipnext = runit->main && runit->main();
            } else if (runit && runit->main)
              skipnext = runit->main();
            else
              skipnext = 1;
          } else
            runit = 0;
        } else
          _callback();
      };
      if (err)
        werror("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n"
              +describe_backtrace(err)
              +">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
    };
  }
  lock = 0;
}
