#pike __REAL_VERSION__

//! Debugging filesystem monitor.
//!
//! This module behaves as @[symlinks], but has default implementations
//! of all callbacks that call @[report()], as well as an implementation
//! of [report()] that logs everything to @[Stdio.stderr].
//!
//! @seealso
//!   @[Filesystem.Monitor.basic], @[Filesystem.Monitor.symlinks]

inherit "symlinks.pike";

#define MON_WERR(X...)	report(NOTICE,	__func__, X)
#define MON_WARN(X...)	report(WARNING,	__func__, X)
#define MON_ERROR(X...)	report(ERROR,	__func__, X)

protected void report(SeverityLevel level, string(7bit) fun,
		      sprintf_format format, sprintf_args ... args)
{
  werror(format, @args);
}

void data_changed(string path) {
  MON_WERR("data_changed(%O)\r\n", path);
}
void attr_changed(string path, Stdio.Stat st) {
  MON_WERR("attr_changed(%O, %O)\r\n", path, st);
}
void file_exists(string path, Stdio.Stat st) {
  MON_WERR("file_exits(%O, %O)\r\n", path, st);
}
void file_created(string path, Stdio.Stat st) {
  MON_WERR("file_created(%O, %O)\r\n", path, st);
}
void file_deleted(string path) {
  MON_WERR("file_deleted(%O)\r\n", path);
}
void stable_data_change(string path, Stdio.Stat st) {
  MON_WERR("stable_data_change(%O, %O)\r\n", path, st);
}

//! Return the set of active monitors.
mapping(string:Monitor) get_monitors()
{
  return monitors + ([]);
}

int check(int|void max_wait, int|void max_cnt,
	  mapping(string:int)|void ret_stats)
{
  int ret;
  MON_WERR("check(%O, %O, %O) ==> %O\r\n",
	   max_wait, max_cnt, ret_stats,
	   ret = ::check(max_wait, max_cnt, ret_stats));
  return ret;
}
