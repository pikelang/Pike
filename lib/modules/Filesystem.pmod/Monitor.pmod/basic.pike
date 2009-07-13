//
// Basic filesystem monitor.
//
// $Id: basic.pike,v 1.2 2009/07/13 09:08:19 grubba Exp $
//
// 2009-07-09 Henrik Grubbström
//

//! Basic filesystem monitor.
//!
//! This module is intended to be used for incremental scanning of
//! a filesystem.

//! The default maximum number of seconds between checks of directories
//! in seconds.
//!
//! This value is multiplied with @[default_file_interval_factor] to
//! get the corresponding default maximum number of seconds for files.
//!
//! The value can be changed by calling @[create()].
//!
//! The value can be overridden for individual files or directories
//! by calling @[monitor()].
//!
//! Overload this constant to change the default.
protected constant default_max_dir_check_interval = 60;

//! The default factor to multiply @[default_max_dir_check_interval]
//! with to get the maximum number of seconds between checks of files.
//!
//! The value can be changed by calling @[create()].
//!
//! The value can be overridden for individual files or directories
//! by calling @[monitor()].
//!
//! Overload this constant to change the default.
protected constant default_file_interval_factor = 5;

//! The minimum number of seconds without changes for a change to be
//! regarded as stable (see @[stable_data_change()].
protected constant stable_time = 5;

protected int max_dir_check_interval = default_max_dir_check_interval;
protected int file_interval_factor = default_file_interval_factor;

// Callbacks

//! File content changed callback.
//!
//! @param path
//!   Path of the file which has had content changed.
//!
//! This function is called when a change has been detected for a
//! monitored file.
//!
//! Called by @[check()] and @[check_monitor()].
//!
//! Overload this to do something useful.
void data_changed(string path);

//! File attribute changed callback.
//!
//! @param path
//!   Path of the file or directory which has changed attributes.
//!
//! @param st
//!   Status information for @[path] as obtained by @expr{file_stat(path, 1)@}.
//!
//! This function is called when a change has been detected for an
//! attribute for a monitored file or directory.
//!
//! Called by @[check()] and @[check_monitor()].
//!
//! @note
//!   If there is a @[data_changed()] callback, it may supersede this
//!   callback if the file content also has changed.
//!
//! Overload this to do something useful.
void attr_changed(string path, Stdio.Stat st);

//! File creation callback.
//!
//! @param path
//!   Path of the new file or directory.
//!
//! @param st
//!   Status information for @[path] as obtained by @expr{file_stat(path, 1)@}.
//!
//! This function is called when either a monitored path has started
//! existing, or when a new file or directory has been added to a
//! monitored directory.
//!
//! Called by @[check()] and @[check_monitor()].
//!
//! Overload this to do something useful.
void file_created(string path, Stdio.Stat st);

//! File deletion callback.
//!
//! @param path
//!   Path of the new file or directory that has been deleted.
//!
//! This function is called when either a monitored path has stopped
//! to exist, or when a file or directory has been deleted from a
//! monitored directory.
//!
//! Called by @[check()] and @[check_monitor()].
//!
//! Overload this to do something useful.
void file_deleted(string path);

//! Stable change callback.
//!
//! @param path
//!   Path of the file or directory that has stopped changing.
//!
//! This function is called when previous changes to @[path] are
//! considered "stable".
//!
//! "Stable" in this case means that there have been no detected
//! changes for at lease @[stable_time] seconds.
//!
//! Called by @[check()] and @[check_monitor()].
//!
//! Overload this to do something useful.
void stable_data_change(string path);

//! Monitoring information.
protected class Monitor(string path, int(0..1) recurse,
			int max_dir_check_interval,
			int file_interval_factor)
{
  int next_poll;
  Stdio.Stat st;
  int last_change = 0x7fffffff;	// Future...
  array(string) files;

  int `<(mixed m) { return next_poll < m; }
  int `>(mixed m) { return next_poll > m; }
  int `==(mixed m) { return next_poll == m; }

  protected string _sprintf(int c)
  {
    return sprintf("Monitor(%O, %O, next: %s, st: %O)",
		   path, recurse, ctime(next_poll) - "\n", st);
  }
}

//! Mapping from monitored path to corresponding @[Monitor].
protected mapping(string:Monitor) monitors = ([]);

//! Heap containing all active @[Monitor]s.
//!
//! The heap is sorted on @[Monitor()->next_poll].
protected ADT.Heap monitor_queue = ADT.Heap();

//! Create a new monitor.
//!
//! @param max_dir_check_interval
//!   Override of @[default_max_dir_check_interval].
//!
//! @param file_interval_factor
//!   Override of @[default_file_interval_factor].
protected void create(int|void max_dir_check_interval,
		      int|void file_interval_factor)
{
  if (max_dir_check_interval > 0) {
    this_program::max_dir_check_interval = max_dir_check_interval;
  }
  if (file_interval_factor > 0) {
    this_program::file_interval_factor = file_interval_factor;
  }
  clear();
}

//! Clear the set of monitored files and directories.
void clear()
{
  monitors = ([]);
  monitor_queue = ADT.Heap();
}

//! Calculate a suitable time for the next poll of this monitor.
protected void update_monitor(Monitor m, Stdio.Stat st)
{
  int delta = m->max_dir_check_interval;
  m->st = st;
  if (!st || !st->isdir) {
    delta *= m->file_interval_factor;
  }
  if (st) {
    int d = ((time(1) - st->mtime)>>4 || 1);
    if (d < 0) d = m->max_dir_check_interval;
    if (d < delta) delta = d;
    d = ((time(1) - st->ctime)>>4 || 1);
    if (d < 0) d = m->max_dir_check_interval;
    if (d < delta) delta = d;
  }
  m->next_poll = time(1) + delta;
  monitor_queue->adjust(m);
}

//! Release a single @[Monitor] from monitoring.
//!
//! @seealso
//!   @[release()]
protected void release_monitor(Monitor m)
{
  m->next_poll = -1000;
  monitor_queue->adjust(m);
  while (monitor_queue->peek() < 0) {
    monitor_queue->pop();
  }
}

//! Register a @[path] for monitoring.
//!
//! @param path
//!   Path to monitor.
//!
//! @param recurse
//!   @int
//!     @value 0
//!       Don't recurse.
//!     @value 1
//!       Monitor the entire subtree, and any directories
//!       or files that may appear later.
//!   @endint
//!
//! @param max_dir_check_interval
//!   Override of @[default_max_dir_check_interval] for this path
//!   or subtree.
//!
//! @param file_interval_factor
//!   Override of @[default_file_interval_factor] for this path
//!   or subtree.
//!
//! @seealso
//!   @[release()]
void monitor(string path, int(0..1)|void recurse,
	     int(0..)|void max_dir_check_interval,
	     int(0..)|void file_interval_factor)
{
  Monitor m = monitors[path] ||
    Monitor(path, recurse, max_dir_check_interval, file_interval_factor);
  monitors[path] = m;
  monitor_queue->push(m);
  Stdio.Stat st = file_stat(path, 1);
  update_monitor(m, st);
  // FIXME: What about symlinks?
  if (st && st->isdir) {
    array(string) files = get_dir(path) || ({});
    m->files = files;
    if (recurse) {
      foreach(files, string file) {
	monitor(Stdio.append_path(path, file), recurse,
		max_dir_check_interval, file_interval_factor);
      }
    }
  }
}

//! Release a @[path] from monitoring.
//!
//! @param path
//!   Path to stop monitoring.
//!
//! @param recurse
//!   @int
//!     @value 0
//!       Don't recurse.
//!     @value 1
//!       Release the entire subtree.
//!   @endint
//!
//! @seealso
//!   @[monitor()]
void release(string path, int(0..1)|void recurse)
{
  Monitor m = m_delete(monitors, path);
  if (m) {
    release_monitor(m);
  }
  if (recurse && m->st && m->st->isdir) {
    path = combine_path(path, "");
    foreach(monitors; string mpath; m) {
      if (has_prefix(mpath, path)) {
	m_delete(monitors, mpath);
	release_monitor(m);
      }
    }
  }
}

//! Check a single @[Monitor] for changes.
//!
//! @param m
//!   @[Monitor] to check.
//!
//! This function is called by @[check()] for the @[Monitor]s
//! it considers need checking. If it detects any changes an
//! appropriate callback will be called.
//!
//! @returns
//!   Returns @expr{1@} if a change was detected and @expr{0@} (zero)
//!   otherwise.
//!
//! @note
//!   Any callbacks will be called from the same thread as the one
//!   calling @[check_monitor()].
//!
//! @seealso
//!   @[check()], @[data_changed()], @[attr_changed()], @[file_created()],
//!   @[file_deleted()], @[stable_data_change()]
protected int(0..1) check_monitor(Monitor m)
{
  // werror("Checking monitor %O...\n", m);
  Stdio.Stat st = file_stat(m->path, 1);
  Stdio.Stat old_st = m->st;
  update_monitor(m, st);
  if (!st) {
    if (old_st) {
      if (file_deleted) {
	file_deleted(m->path);
      }
      return 1;
    }
    return 0;
  }
  if (!old_st) {
    m->last_change = time(1);
    if (file_created) {
      file_created(m->path, st);
    }
    return 1;
  }
  if ((st->mtime != old_st->mtime) || (st->ctime != old_st->ctime) ||
      (st->size != old_st->size)) {
    m->last_change = time(1);
    if (st->isdir) {
      array(string) files = get_dir(m->path);
      array(string) new_files = files - m->files;
      array(string) deleted_files = m->files - files;
      m->files = files;
      foreach(new_files, string file) {
	file = Stdio.append_path(m->path, file);
	Monitor m2 = monitors[file];
	mixed err = catch {
	    if (m2) {
	      // We have a separate monitor on the created file.
	      // Let it handle the notification.
	      check_monitor(m2);
	    }
	  };
	if (m->recurse) {
	  monitor(file, m->recurse,
		  m->max_dir_check_interval, m->file_interval_factor);
	  monitors[file]->last_change = time(1);
	}
	if (!m2 && file_created) {
	  file_created(file, file_stat(file, 1));
	}
      }
      foreach(deleted_files, string file) {
	file = Stdio.append_path(m->path, file);
	Monitor m2 = monitors[file];
	mixed err = catch {
	    if (m2) {
	      // We have a separate monitor on the deleted file.
	      // Let it handle the notification.
	      check_monitor(m2);
	    }
	  };
	if (m->recurse) {
	  // Release the file after the recursive call above,
	  // to make sure all notifications propagate properly.
	  release(file, m->recurse);
	}
	if (!m2 && file_deleted) {
	  file_deleted(file);
	}
	if (err) throw(err);
      }
      if (sizeof(new_files) || sizeof(deleted_files)) return 1;
    } else {
      if (data_changed) {
	data_changed(m->path);
      }
      return 1;
    }
    if (attr_changed) {
      attr_changed(m->path, st);
    }
    return 1;
  }
  if (m->last_change < time(1) - stable_time) {
    m->last_change = 0x7fffffff;
    if (stable_data_change) {
      stable_data_change(m->path);
    }
    return 1;
  }
  return 0;
}

//! Check for changes.
//!
//! @param max_wait
//!   Maximum time in seconds to wait for changes. @expr{-1}
//!   for infinite wait.
//!
//! A suitable subset of the monitored files will be checked
//! for changes.
//!
//! @returns
//!   The function returns when either a change has been detected
//!   or when @[max_wait] has expired.
//!
//! @note
//!   Any callbacks will be called from the same thread as the one
//!   calling @[check()].
//!
//! @seealso
//!   @[monitor()]
void check(int|void max_wait)
{
  if (!sizeof(monitors)) {
    if (max_wait > 0) sleep(max_wait);
    return;
  }
  while(1) {
    int cnt;
    int t = time();
    Monitor m;
    while ((m = monitor_queue->peek()) &&
	   m <= t) {
      cnt += check_monitor(m);
    }
    if (cnt || !max_wait) return;
    if (max_wait > 0) max_wait--;
    sleep(1);
  }
}

//! Backend to use.
//!
//! If @expr{0@} (zero) - use the default backend.
protected Pike.Backend backend;

//! Call-out identifier for @[backend_check()] if in
//! nonblocking mode.
//!
//! @seealso
//!   @[set_nonblocking()], @[set_blocking()]
protected mixed co_id;

//! Change backend.
//!
//! @param backend
//!   Backend to use. @expr{0@} (zero) for the default backend.
void set_backend(Pike.Backend|void backend)
{
  int was_nonblocking = !!co_id;
  set_blocking();
  this_program::backend = backend;
  if (was_nonblocking) {
    set_nonblocking();
  }
}

//! Turn off nonblocking mode.
//!
//! @seealso
//!   @[set_nonblocking()]
void set_blocking()
{
  if (co_id) {
    if (backend) backend->remove_call_out(co_id);
    else remove_call_out(co_id);
    co_id = 0;
  }
}

//! Backend check callback function.
//!
//! This function is intended to be called from a backend,
//! and performs a @[check()] followed by rescheduling
//! itself via a call to @[set_nonblocking()].
//!
//! @seealso
//!   @[check()], @[set_nonblocking()]
protected void backend_check()
{
  co_id = 0;
  mixed err = catch {
      check(0);
    };
  set_nonblocking();
  if (err) throw(err);
}

//! Turn on nonblocking mode.
//!
//! Register suitable callbacks with the backend to automatically
//! call @[check()].
//!
//! @[check()] and thus all the callbacks will be called from the
//! backend thread.
//!
//! @seealso
//!   @[set_blocking()], @[check()].
void set_nonblocking()
{
  if (co_id) return;
  Monitor m = monitor_queue->peek();
  int t = (m && m->next_poll - time(1)) || max_dir_check_interval;
  if (t > max_dir_check_interval) t = max_dir_check_interval;
  if (backend) co_id = backend->call_out(backend_check, t);
  else co_id = call_out(backend_check, t);
}
