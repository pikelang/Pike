#pike __REAL_VERSION__

//
// Basic filesystem monitor.
//
//
// 2009-07-09 Henrik Grubbström
//
//! Basic filesystem monitor.
//!
//! This module is intended to be used for incremental scanning of
//! a filesystem.
//!
//! Supports FSEvents on MacOS X and Inotify on Linux to provide low
//! overhead monitoring; other systems use a less efficient polling approach.
//!
//! @seealso
//!  @[Filesystem.Monitor.symlinks], @[System.FSEvents], @[System.Inotify]

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

//! The default minimum number of seconds without changes for a change
//! to be regarded as stable (see @[stable_data_change()].
protected constant default_stable_time = 5;

protected int max_dir_check_interval = default_max_dir_check_interval;
protected int file_interval_factor = default_file_interval_factor;
protected int stable_time = default_stable_time;

protected inline constant SeverityLevel = DefaultCompilerEnvironment.SeverityLevel;
protected inline constant NOTICE = DefaultCompilerEnvironment.NOTICE;
protected inline constant WARNING = DefaultCompilerEnvironment.WARNING;
protected inline constant ERROR = DefaultCompilerEnvironment.ERROR;
protected inline constant FATAL = DefaultCompilerEnvironment.FATAL;

// Callbacks

//! Event tracing callback.
//!
//! @param level
//!   Severity level of the event.
//!
//! @param fun
//!   Name of the function that called @[report()].
//!
//! @param format
//!   @[sprintf()] formatting string describing the event.
//!
//! @param args
//!   Optional extra arguments for the @[format] string.
//!
//! This function is called in various places to provide
//! granular tracing of the monitor state.
//!
//! The default implementation calls @[werror()] with
//! @[format] and @[args] if @[level] is @[ERROR] or higher,
//! or if @tt{FILESYSTEM_MONITOR_DEBUG@} has been defined.
protected void report(SeverityLevel level, string(7bit) fun,
		      sprintf_format format, sprintf_args ... args)
{
#ifndef FILESYSTEM_MONITOR_DEBUG
  if (level < ERROR) return;
#endif
  werror(format, @args);
}

#define MON_WERR(X...)	report(NOTICE,	__func__, X)
#define MON_WARN(X...)	report(WARNING,	__func__, X)
#define MON_ERROR(X...)	report(ERROR,	__func__, X)

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

//! File existance callback.
//!
//! @param path
//!   Path of the file or directory.
//!
//! @param st
//!   Status information for @[path] as obtained by @expr{file_stat(path, 1)@}.
//!
//! This function is called during initialization for all monitored paths,
//! and subpaths for monitored directories. It represents the initial state
//! for the monitor.
//!
//! @note
//!   For directories, @[file_exists()] will be called for the subpaths
//!   before the call for the directory itself. This can be used to detect
//!   when the initialization for a directory is finished.
//!
//! Called by @[check()] and @[check_monitor()] the first time a monitored
//! path is checked (and only if it exists).
//!
//! Overload this to do something useful.
//!
//! @note
//!   This callback has similar semantics to @[file_created()], but
//!   is called at initialization time.
//!
//! @seealso
//!   @[file_created()], @[file_deleted()], @[stable_data_change()]
void file_exists(string path, Stdio.Stat st);

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
//!
//! @note
//!   This callback has similar semantics to @[file_exists()], but
//!   is called at run time.
//!
//! @seealso
//!   @[file_exists()], @[file_deleted()], @[stable_data_change()]
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
//!
//! @seealso
//!   @[file_created()], @[file_exists()], @[stable_data_change()]
void file_deleted(string path);

//! Stable change callback.
//!
//! @param path
//!   Path of the file or directory that has stopped changing.
//!
//! @param st
//!   Status information for @[path] as obtained by @expr{file_stat(path, 1)@}.
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
//!
//! @note
//!   This callback being called does not mean that the contents
//!   or inode has changed, just that they don't seem to change any
//!   more. In particular it is called for paths found during
//!   initialization after @[stable_time] seconds have passed.
//!
//! @seealso
//!   @[file_created()], @[file_exists()], @[file_deleted()]
void stable_data_change(string path, Stdio.Stat st);

//! Flags for @[Monitor]s.
enum MonitorFlags {
  MF_RECURSE = 1,
  MF_AUTO = 2,
  MF_INITED = 4,
  MF_HARD = 8,
};

protected constant S_IFMT = 0x7ffff000;

//! Monitoring information for a single filesystem path.
//!
//! @seealso
//!   @[monitor()]
protected class Monitor(string path,
			MonitorFlags flags,
			int max_dir_check_interval,
			int file_interval_factor,
			int stable_time)
{
  inherit ADT.Heap.Element;

  int next_poll;
  Stdio.Stat st;
  int last_change = 0x7fffffff;	// Future... Can be set to -0x7fffffff
				// to indicate immediate stabilization
				// (avoid an extra check() round to
				// let the stat stabilize).
  private bool initialized = false;
  array(string) files;

  //! Event tracing callback.
  //!
  //! @param level
  //!   Severity level of the event.
  //!
  //! @param fun
  //!   Name of the function that called @[report()].
  //!
  //! @param format
  //!   @[sprintf()] formatting string describing the event.
  //!
  //! @param args
  //!   Optional extra arguments for the @[format] string.
  //!
  //! This function is called in various places to provide
  //! granular tracing of the monitor state.
  //!
  //! The default implementation just calls @[global::report()]
  //! with the same arguments.
  protected void report(SeverityLevel level, string(7bit) fun,
			sprintf_format format, sprintf_args ... args)
  {
    global::report(level, fun, format, @args);
  }

  //! Register the @[Monitor] with the monitoring system.
  //!
  //! @param initial
  //!   Indicates that the @[Monitor] is newly created.
  protected void register_path(int|void initial)
  {
    if (initial) {
      // We need to be polled...
      MON_WERR("Registering %O for polling.\n", path);
      mixed key = monitor_mutex->lock();
      monitor_queue->push(this);
      if (monitor_queue->peek() == this) {
	if (co_id) {
	  reschedule_backend_check();
	}
      }
    }
  }

  //! Unregister the @[Monitor] from the monitoring system.
  //!
  //! @param dying
  //!   Indicates that the @[Monitor] is being destructed.
  //!   It is the destruction cause value offset by one.
  protected void unregister_path(int|void dying)
  {
    if (dying == 1) {
      // We are going away permanently due to explicit destruct(),
      // so remove ourselves from from the monitor_queue.
      mixed key = monitor_mutex->lock();
      MON_WERR("Unregistering %O from polling.\n", path);
      monitor_queue->remove(this);
    }
  }

  protected int `<(mixed m) { return next_poll < m; }
  protected int `>(mixed m) { return next_poll > m; }

  protected void create()
  {
    MON_WERR("Creating monitor for %O.\n", path);
    Element::create(this);
    register_path(1);
  }

  //! Returns the parent monitor, or UNDEFINED if no such monitor exists.
  this_program parent()
  {
    string parent_path = canonic_path (dirname (path));
    return monitors[parent_path];
  }

  //! To be called when a (direct) submonitor is released.
  void submonitor_released (this_program submon)
  {
    if (files) {
      string filename = basename (submon->path);
      MON_WERR("%O->submonitor_released(%O): Removing list state for %O.\n",
	       this, submon, filename);
      files -= ({ filename });
    }
  }

  //! Call a notification callback.
  //!
  //! @param cb
  //!   Callback to call or @[UNDEFINED] for no operation.
  //!
  //! @param path
  //!   Path to notify on.
  //!
  //! @param st
  //!   Stat for the @[path].
  protected void call_callback(function(string, Stdio.Stat|void:void) cb,
			       string path, Stdio.Stat|void st)
  {
    if (!cb) return;
    cb(path, st);
  }

  //! File attribute or content changed callback.
  //!
  //! @param st
  //!   Status information for @[path] as obtained by
  //!   @expr{file_stat(path, 1)@}.
  //!
  //! This function is called when a change has been detected for an
  //! attribute for a monitored file or directory.
  //!
  //! Called by @[check()] and @[check_monitor()].
  //!
  //! The default implementation calls @[global::attr_changed()] or
  //! @[global::data_changed()] depending on the state.
  //!
  //! @note
  //!   If there is a @[data_changed()] callback, it may supersede this
  //!   callback if the file content also has changed.
  protected void attr_changed(string path, Stdio.Stat st)
  {
    MON_WERR("attr_changed(%O, %O)\n", path, st);
    if (global::data_changed) {
      call_callback(global::data_changed, path);
    } else {
      call_callback(global::attr_changed, path, st);
    }
  }

  //! File existance callback.
  //!
  //! @param st
  //!   Status information for @[path] as obtained by
  //!   @expr{file_stat(path, 1)@}.
  //!
  //! This function is called during initialization for all monitored paths,
  //! and subpaths for monitored directories. It represents the initial state
  //! for the monitor.
  //!
  //! @note
  //!   For directories, @[file_created()] will be called for the subpaths
  //!   before the call for the directory itself. This can be used to detect
  //!   when the initialization for a directory is finished.
  //!
  //! Called by @[check()] and @[check_monitor()] the first time a monitored
  //! path is checked (and only if it exists).
  //!
  //! The default implementation registers the path, and
  //! calls @[global::file_exists()].
  //!
  //! @note
  //!   This callback has similar semantics to @[file_created()], but
  //!   is called at initialization time.
  //!
  //! @seealso
  //!   @[file_created()], @[file_deleted()], @[stable_data_change()]
  protected void file_exists(string path, Stdio.Stat st)
  {
    MON_WERR("file_exists(%O, %O)\n", path, st);
    register_path();
    int t = time(1);
    call_callback(global::file_exists, path, st);
    if (st->mtime + (stable_time || global::stable_time) >= t) {
      // Not stable yet! We guess that the mtime is a
      // fair indication of when the file last changed.
      last_change = st->mtime;
    }
  }

  //! File creation callback.
  //!
  //! @param st
  //!   Status information for @[path] as obtained by
  //!   @expr{file_stat(path, 1)@}.
  //!
  //! This function is called when either a monitored path has started
  //! existing, or when a new file or directory has been added to a
  //! monitored directory.
  //!
  //! Called by @[check()] and @[check_monitor()].
  //!
  //! The default implementation registers the path, and
  //! calls @[global::file_deleted()].
  //!
  //! @note
  //!   This callback has similar semantics to @[file_exists()], but
  //!   is called at run time.
  //!
  //! @seealso
  //!   @[file_exists()], @[file_deleted()], @[stable_data_change()]
  protected void file_created(string path, Stdio.Stat st)
  {
    MON_WERR("file_created(%O, %O)\n", path, st);
    register_path();
    call_callback(global::file_created, path, st);
  }

  //! File deletion callback.
  //!
  //! @param path
  //!   Path of the new file or directory that has been deleted.
  //!
  //! @param old_st
  //!   Stat for the file prior to deletion (if known). Note that
  //!   this argument is not passed along to top level function.
  //!
  //! This function is called when either a monitored path has stopped
  //! to exist, or when a file or directory has been deleted from a
  //! monitored directory.
  //!
  //! Called by @[check()] and @[check_monitor()].
  //!
  //! The default implementation unregisters the path, and
  //! calls @[global::file_deleted()].
  //!
  //! @seealso
  //!   @[file_created()], @[file_exists()], @[stable_data_change()]
  protected void file_deleted(string path, Stdio.Stat|void old_st)
  {
    MON_WERR("file_deleted(%O, %O)\n", path, st);
    unregister_path();
    call_callback(global::file_deleted, path);
  }

  //! Stable change callback.
  //!
  //! @param st
  //!   Status information for @[path] as obtained by
  //!   @expr{file_stat(path, 1)@}.
  //!
  //! This function is called when previous changes to @[path] are
  //! considered "stable".
  //!
  //! "Stable" in this case means that there have been no detected
  //! changes for at lease @[stable_time] seconds.
  //!
  //! Called by @[check()] and @[check_monitor()].
  //!
  //! The default implementation calls @[global::stable_data_change()].
  //!
  //! @note
  //!   This callback being called does not mean that the contents
  //!   or inode has changed, just that they don't seem to change any
  //!   more. In particular it is called for paths found during
  //!   initialization after @[stable_time] seconds have passed.
  //!
  //! @seealso
  //!   @[file_created()], @[file_exists()], @[file_deleted()]
  protected void stable_data_change(string path, Stdio.Stat st)
  {
    MON_WERR("stable_data_change(%O, %O)\n", path, st);
    call_callback(global::stable_data_change, path, st);
  }

  protected string _sprintf(int c)
  {
    return sprintf("Monitor(%O, %O, next: %s, st: %O)",
		   path, flags, ctime(next_poll) - "\n", st);
  }

  //! Bump the monitor to an earlier scan time.
  //!
  //! @param flags
  //!   @int
  //!     @value 0
  //!       Don't recurse.
  //!     @value 1
  //!       Check all monitors for the entire subtree.
  //!   @endint
  //!
  //! @param seconds
  //!   Number of seconds from now to run next scan. Defaults to
  //!   half of the remaining interval.
  void bump(MonitorFlags|void flags, int|void seconds)
  {
    int now = time(1);
    if (seconds)
      next_poll = now + seconds;
    else if (next_poll > now)
      next_poll -= (next_poll - now) / 2;
    adjust_monitor(this);

    if ((flags & MF_RECURSE) && st && st->isdir && files) {
      // Bump the files in the directory as well.
      foreach(files, string file) {
	file = canonic_path(Stdio.append_path(path, file));
	Monitor m2 = monitors[file];
	if (m2) {
	  m2->bump(flags, seconds);
	}
      }
    }
  }

  //! Calculate and set a suitable time for the next poll of this monitor.
  //!
  //! @param st
  //!   New stat for the monitor.
  //!
  //! This function is called by @[check()] to schedule the
  //! next check.
  protected void update(Stdio.Stat st)
  {
    int delta = max_dir_check_interval || global::max_dir_check_interval;
    this::st = st;

    if (st && !st->isdir) {
      delta *= file_interval_factor || global::file_interval_factor;
    }

    if (st) {
      //  Start with a delta proportional to the time since mtime/ctime,
      //  but bound this to the max setting. A stat in the future will be
      //  adjusted to the max interval.
      int mtime = max(st->mtime, st->ctime);
      int d = ((time(1) - mtime) >> 2);
      if (!initialized && (d >= 0)) {
	// Assume that mtime is reasonable at startup.
	last_change = mtime;
      }
      if ((d >= 0) && (d < delta)) delta = d;
    }
    if (last_change <= time(1)) {
      // Time until stable.
      int d = last_change + (stable_time || global::stable_time) - time(1);
      d >>= 1;
      if (d < 0) d = 1;
      if (d < delta) delta = d;
    }
    if (!initialized) {
      // Attempt to distribute polls evenly at startup, and to
      // make sure that the full set of directory contents is
      // found reasonably fast.
      delta = 1 + random(delta >> 2);
      initialized = true;
    }

    MON_WERR("Next poll in %d seconds.\n", (delta || 1));
    next_poll = time(1) + (delta || 1);
    adjust_monitor(this);
  }

  //! Check if this monitor should be removed automatically.
  void check_for_release(int mask, int flags)
  {
    if ((this::flags & mask) == flags) {
      MON_WERR("Releasing in %O->check_for_release(%O, %O)\n",
	       this, mask, flags);
      m_delete(monitors, path);
      release_monitor(this);
      if (this_program par = parent()) {
	par->submonitor_released (this);
      } else {
	MON_WERR("%O has no parent monitor.\n", this);
      }
    }
  }

  //! Called to create a sub monitor.
  protected void monitor(string path, int flags, int max_dir_interval,
			 int file_interval_factor, int stable_time)
  {
    global::monitor(path, flags, max_dir_check_interval,
		    file_interval_factor, stable_time);
  }

  //! Called when the status has changed for an existing file.
  protected int(0..1) status_change(Stdio.Stat old_st, Stdio.Stat st,
				    int orig_flags, int flags)
  {
    MON_WERR("status_change(%O, %O, 0x%04x, 0x%04x)\n",
	     old_st, st, orig_flags, flags);
    if (st->isdir) {
      int res = 0;
      array(string) files = get_dir(path) || ({});
      array(string) new_files = files;
      array(string) deleted_files = ({});
      if (this::files) {
	new_files -= this::files;
	deleted_files = this::files - files;
      }
      MON_WERR("%d files created, %d files deleted.\n",
	       sizeof(new_files), sizeof(deleted_files));
      this::files = files;
      foreach(new_files, string file) {
	res = 1;
	file = canonic_path(Stdio.append_path(path, file));
	if(filter_file(file)) continue;
	Monitor m2 = monitors[file];
	mixed err = catch {
	    if (m2) {
	      // We have a separate monitor on the created file.
	      // Let it handle the notification.
	      m2->check(flags);
	    }
	  };
	if (this::flags & MF_RECURSE) {
	  monitor(file, orig_flags | MF_AUTO | MF_HARD,
		  max_dir_check_interval,
		  file_interval_factor,
		  stable_time);
	  monitors[file]->check();
	} else if (!m2) {
	  file_created(file, file_stat(file, 1));
	}
	if (err) {
	  master()->handle_error(err);
	}
      }
      foreach(deleted_files, string file) {
	res = 1;
	file = canonic_path(Stdio.append_path(path, file));
	if(filter_file(file)) continue;
	Monitor m2 = monitors[file];
	mixed err = catch {
	    if (m2) {
	      // We have a separate monitor on the deleted file.
	      // Let it handle the notification.
	      m2->check(flags);
	    }
	  };
	if (this::flags & MF_RECURSE) {
	  // The monitor for the file has probably removed itself,
	  // or the user has done it by hand, in either case we
	  // don't need to do anything more here.
	} else if (!m2) {
	  file_deleted(file);
	}
	if (err) {
	  master()->handle_error(err);
	}
      }
      if (flags & MF_RECURSE) {
	// Check the remaining files in the directory soon.
	foreach(((files - new_files) - deleted_files), string file) {
	  file = canonic_path(Stdio.append_path(path, file));
	  if(filter_file(file)) continue;
	  Monitor m2 = monitors[file];
	  if (m2) {
	    m2->bump(flags);
	  } else {
	    // Lost update due to race-condition:
	    //
	    //   Exist ==> Deleted ==> Exists
	    //
	    // with no update of directory inbetween.
	    //
	    // Create the lost submonitor again.
	    res = 1;
	    monitor(file, orig_flags | MF_AUTO | MF_HARD,
		    max_dir_check_interval,
		    file_interval_factor,
		    stable_time);
	    monitors[file]->check();
	  }
	}
      }
      return res;
    } else {
      attr_changed(path, st);
      return 1;
    }
    return 0;
  }

  //! Check for changes.
  //!
  //! @param flags
  //!   @int
  //!     @value 0
  //!       Don't recurse.
  //!     @value 1
  //!       Check all monitors for the entire subtree rooted in @[m].
  //!   @endint
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
  //! @note
  //!   The return value can not be trusted to return @expr{1@} for all
  //!   detected changes in recursive mode.
  //!
  //! @seealso
  //!   @[check()], @[data_changed()], @[attr_changed()], @[file_created()],
  //!   @[file_deleted()], @[stable_data_change()]
  int(0..1) check(MonitorFlags|void flags)
  {
    MON_WERR("Checking monitor %O...\n", this);
    Stdio.Stat st = file_stat(path, 1);
    Stdio.Stat old_st = this::st;
    int orig_flags = this::flags;
    this::flags |= MF_INITED;
    update(st);
    if (!(orig_flags & MF_INITED)) {
      // Initialize.
      if (st) {
	if (st->isdir) {
	  array(string) files = get_dir(path) || ({});
	  this::files = files;
	  foreach(files, string file) {
	    file = canonic_path(Stdio.append_path(path, file));
	    if(filter_file(file)) continue;
	    if (monitors[file]) {
	      // There's already a monitor for the file.
	      // Assume it has already notified about existance.
	      continue;
	    }
	    if (this::flags & MF_RECURSE) {
	      monitor(file, orig_flags | MF_AUTO | MF_HARD,
		      max_dir_check_interval,
		      file_interval_factor,
		      stable_time);
	      check_monitor(monitors[file]);
	    } else {
	      file_exists(file, file_stat(file, 1));
	    }
	  }
	}
	// Signal file_exists for path as an end marker.
	file_exists(path, st);
      } else  {
	// The path we're supposed to monitor is already gone.
	check_for_release(MF_AUTO, MF_AUTO);
      }
      return 1;
    }
    if (old_st) {
      if (!st || ((old_st->mode & S_IFMT) != (st->mode & S_IFMT))) {
	// File deleted or changed type.

	int delay;
	// Propagate deletions to any submonitors.
	if (files) {
	  foreach(files, string file) {
	    file = canonic_path(Stdio.append_path(path, file));
	    if (Monitor submon = monitors[file]) {
	      // Adjust next_poll, so that the monitor will be checked soon.
	      submon->next_poll = time(1)-1;
	      adjust_monitor(submon);
	      delay = 1;
	    }
	  }
	}
	if (delay) {
	  // Delay the notification until the submonitors have notified.
	  this::st = old_st;
	  next_poll = time(1);
	  adjust_monitor(this);
	} else {
	  if (st) {
	    // Avoid race when a file has been replaced with a directory
	    // or vice versa or similar.
	    st = UNDEFINED;

	    // We will catch the new file at the next poll.
	    next_poll = time(1);
	    adjust_monitor(this);
	  } else {
	    // The monitor no longer has a link from its parent directory.
	    this::flags &= ~MF_HARD;

	    // Check if we should remove the monitor.
	    check_for_release(MF_AUTO, MF_AUTO);
	  }

	  file_deleted(path, old_st);
	  return 1;
	}
	return 0;
      }
    } else if (st) {
      // File created.

      last_change = time(1);
      file_created(path, st);
      if (st->isdir) {
	array(string) files = get_dir(path) || ({});
	this::files = files;
	foreach(files, string file) {
	  file = canonic_path(Stdio.append_path(path, file));
	  if (filter_file(file)) continue;
	  if (monitors[file]) {
	    // There's already a monitor for the file.
	    // Assume it has already notified about existance.
	    continue;
	  }
	  if (this::flags & MF_RECURSE) {
	    monitor(file, orig_flags | MF_AUTO | MF_HARD,
		    max_dir_check_interval,
		    file_interval_factor,
		    stable_time);
	    check_monitor(monitors[file]);
	  } else {
	    file_created(file, file_stat(file, 1));
	  }
	}
      }
      update(st);
      return 1;
    } else {
      return 0;
    }

    //  Note: ctime seems to change unexpectedly when running ImageMagick
    //        on NFS disk so we disable it for the moment [bug 5587].
    if (last_change != -0x7fffffff &&
	((st->mtime != old_st->mtime) ||
	 /* (st->ctime != old_st->ctime) || */
	 (st->size != old_st->size))) {
      MON_WERR("Inode changed. New st: %O.\n", st);
      last_change = time(1);
      update(st);
      if (status_change(old_st, st, orig_flags, flags)) return 1;
    } else if (last_change < time(1) - (stable_time || global::stable_time)) {
      MON_WERR("Inode stable now.\n");
      last_change = 0x7fffffff;
      stable_data_change(path, st);
      return 1;
    } else if (last_change != 0x7fffffff &&
	       st->isdir && status_change(old_st, st, orig_flags, flags)) {
      MON_WERR("Directory not stable yet.\n");
      // Directory not stable yet.
      last_change = time(1);
      update(st);
      return 1;
    } else if (last_change != 0x7fffffff) {
      MON_WERR("Not stable yet. Age: %d seconds. Path: %O\n",
	       time(1) - last_change, path);
    } else {
      MON_WERR("Inode stable still.\n");
    }
    return 0;
  }

  protected void _destruct(int cause)
  {
    // NB: Cause #0 == DESTRUCT_EXPLICIT.
    //     Any other cause and unregistering is irrelevant.
    unregister_path(1 + cause);
  }
}

//! @class DefaultMonitor
//! This symbol evaluates to the @[Monitor] class used by
//! the default implementation of @[monitor_factory()].
//!
//! It is currently one of the values @[Monitor], @[EventStreamMonitor]
//! or @[InotifyMonitor].
//!
//! @seealso
//!   @[monitor_factory()]

// NB: See further below for the actual definitions.

//! @decl inherit Monitor

//! @endclass

//
// Some necessary setup activities for systems that provide
// filesystem event monitoring
//
#if constant(System.FSEvents.EventStream)
#define HAVE_EVENTSTREAM 1
#endif

#if constant(System.Inotify)
#define HAVE_INOTIFY 1
#endif

#if HAVE_EVENTSTREAM
protected System.FSEvents.EventStream eventstream;
protected array(string) eventstream_paths = ({});

//! This function is called when the FSEvents EventStream detects a change
//! in one of the monitored directories.
protected void low_eventstream_callback(string path, int flags, int event_id)
{
  MON_WERR("eventstream_callback(%O, 0x%08x, %O)\n", path, flags, event_id);
  if(path[-1] == '/') path = path[0..<1];
  MON_WERR("Normalized path: %O\n", path);

  int monitor_flags;
  if (flags & System.FSEvents.kFSEventStreamEventFlagMustScanSubDirs)
    monitor_flags |= MF_RECURSE;

  int found;
  string checkpath = path;
  for (int i = 0; i <= 1; i++) {
    MON_WERR("Looking up monitor for path %O.\n", checkpath);
    if(Monitor m = monitors[checkpath]) {
      MON_WERR("Found monitor %O for path %O.\n", m, checkpath);
      m->check(monitor_flags);
      found = 1;
      break;
    }
    checkpath = dirname (checkpath);
  }

  if (!found)
    MON_WARN("No monitor found for path %O.\n", path);
}

protected void eventstream_callback(string path, int flags, int event_id)
{
  if (backend)
    backend->call_out(low_eventstream_callback, 0, path, flags, event_id);
  else
    low_eventstream_callback(path, flags, event_id);
}

protected void start_accelerator()
{
  // Make sure that the main backend is in CF-mode.
  Pike.DefaultBackend.enable_core_foundation(1);

  MON_WERR("Creating event stream.\n");
#if constant (System.FSEvents.kFSEventStreamCreateFlagFileEvents)
  int flags = System.FSEvents.kFSEventStreamCreateFlagFileEvents;
#else
  int flags = System.FSEvents.kFSEventStreamCreateFlagNone;
#endif

  // Note: We let the polling system find the initial contents of
  //       any pre-existing directories.
  eventstream =
    System.FSEvents.EventStream(({}), 0.1,
				System.FSEvents.kFSEventStreamEventIdSinceNow,
				flags);
  eventstream->callback_func = eventstream_callback;
}

//! FSEvents EventStream-accelerated @[Monitor].
protected class EventStreamMonitor
{
  inherit Monitor;

  protected void register_path(int|void initial)
  {
#ifndef INHIBIT_EVENTSTREAM_MONITOR
    if (initial) {
      if (Pike.DefaultBackend.executing_thread() != Thread.this_thread()) {
	// eventstream stuff (especially start()) must be called from
	// the backend thread, otherwise events will be fired in
	// CFRunLoop contexts where noone listens.
	MON_WERR("%O: Switching to backend thread.\n", this_function);
	call_out(register_path, 0, initial);
	return;
      }

      // We're now in the main backend.

      if (!eventstream) {
	start_accelerator();
      }

      string found;
      foreach(eventstream_paths;;string p) {
	if((path == p) || has_prefix(path, p + "/")) {
	  MON_WERR("Path %O already monitored via path %O.\n", path, p);
	  found = p;
	  break;
	}
      }
      if (found) {
	MON_WERR("Path %O is accelerated via %O.\n", path, found);
      } else {
	// NB: Eventstream doesn't notify on the monitored path;
	//     only on its contents.
	mixed err = catch {
	    MON_WERR("Adding %O to the set of monitored paths.\n", path);
	    eventstream_paths += ({path});
	    if(eventstream->is_started())
	      eventstream->stop();
	    eventstream->add_path(path);
	    eventstream->start();
	  };

	if (err) {
	  MON_ERROR("%O: Failed to register path %O.\n", this_function, path);
	  master()->handle_error(err);
	}
      }
      // Note: Falling through to ::register_path() below.
      //       This is needed to handle paths mounted on eg network
      //       filesystems that are modified on other machines.
      // It is also used to find the initial contents of a monitored directory.
    }
#endif /* !INHIBIT_EVENTSTREAM_MONITOR */
    ::register_path(initial);
  }
}

constant DefaultMonitor = EventStreamMonitor;

#elseif HAVE_INOTIFY

protected System.Inotify._Instance instance;

protected string(8bit) inotify_cookie(int wd)
{
  // NB: Prefix with a NUL to make sure not to conflict with real paths.
  return sprintf("\0%8c", wd);
}

//! Event callback for Inotify.
protected void inotify_event(int wd, int event, int cookie, string(8bit) path)
{
  MON_WERR("inotify_event(%O, %s, %O, %O)...\n",
	   wd, System.Inotify.describe_mask(event), cookie, path);
  string(8bit) icookie = inotify_cookie(wd);
  Monitor m;
  if((m = monitors[icookie])) {
    if (sizeof (path)) {
      string full_path = canonic_path(Stdio.append_path(m->path, path));
      // We're interested in the sub monitor, if it exists.
      if (Monitor submon = monitors[full_path]) {
	MON_WERR ("inotify_event: Got submonitor %O.\n", submon);

	m = submon;
      } else {
	MON_WERR ("inotify_event: Forcing check of %O.\n", m);
	// No monitor exists for the path yet (typically happens for
	// IN_CREATE events). Force a check on the directory monitor.
	m->check(m->flags);

	// Try again after directory check.
	if (Monitor submon2 = monitors[full_path]) {
	  MON_WERR ("inotify_event: Got submonitor %O on retry.\n", submon2);
	  m = submon2;
	} else {
	  MON_WERR ("inotify_event: Failed to get monitor for file %s "
		    "in %O.\n", path, m);
	}
      }
    }
  }

  if (m) {
    if (event == System.Inotify.IN_IGNORED) {
      // This Inotify watch has been removed
      // (either by us or automatically).
      MON_WERR("### Monitor watch descriptor %d is no more.\n", wd);
      m_delete(monitors, icookie);
    }
    mixed err = catch {
	if (event & System.Inotify.IN_CLOSE_WRITE)
	  // File marked as stable immediately.
	  m->last_change = -0x7fffffff;
	m->check(0);
      };
    if (err) {
      master()->handle_error(err);
    }
  } else {
    // Most likely not reached.
    MON_WARN("Monitor not found for cookie %O, path %O.\n", icookie, path);
  }
}

protected void start_accelerator()
{
  MON_WERR("Creating Inotify monitor instance.\n");
  instance = System.Inotify._Instance();
  if (backend) instance->set_backend(backend);
  instance->set_event_callback(inotify_event);
  if (co_id) {
    MON_WERR("Turning on nonblocking mode for Inotify.\n");
    instance->set_nonblocking();
  }
}

//! Inotify-accelerated @[Monitor].
protected class InotifyMonitor
{
  inherit Monitor;

  protected int wd = -1;
  protected int(0..) out_of_inotify_space;

  protected void register_path(int|void initial)
  {
#ifndef INHIBIT_INOTIFY_MONITOR
    if (wd == -1) {
      if (!instance) {
	start_accelerator();
      }

      // NB: We need to follow symlinks here.
      // Currently we only support changing symlinks and symlinks to directories.
      // FIXME: Handle broken symlinks where the target later shows up and
      //        symlinks to changing files.
      Stdio.Stat st = file_stat(path, 1);
      mixed err;
      if (st && (!(flags & MF_AUTO) || st->isdir)) {
	// Note: We only want to add watchers on directories. File
	// notifications will take place on the directory watch
	// descriptors. Expansion of the path to cover notifications
	// on individual files is handled in the inotify_event
	// callback.

	if (err = catch {
	    int new_wd = instance->add_watch(path,
					     System.Inotify.IN_MOVED_FROM |
					     System.Inotify.IN_UNMOUNT |
					     System.Inotify.IN_MOVED_TO |
					     System.Inotify.IN_MASK_ADD |
					     System.Inotify.IN_MOVE_SELF |
					     System.Inotify.IN_DELETE |
					     System.Inotify.IN_MOVE |
					     System.Inotify.IN_MODIFY |
					     System.Inotify.IN_ATTRIB |
					     System.Inotify.IN_DELETE_SELF |
					     System.Inotify.IN_CREATE |
					     System.Inotify.IN_CLOSE_WRITE);

	    if (new_wd != -1) {
	      MON_WERR("Registered %O with %O ==> %d.\n", path, instance, new_wd);
	      out_of_inotify_space = 0;
	      wd = new_wd;
	      monitors[inotify_cookie(wd)] = this;
	    }
	  }) {
	  if (!has_value(lower_case(describe_error(err)), "no space left")) {
	    master()->handle_error(err);
	  } else if (!(out_of_inotify_space++ % 100)) {
	    werror("%O: Out of inotify space (%d attempts):\n",
		   this_function, out_of_inotify_space);
	    master()->handle_error(err);
	    werror("Consider increasing '/proc/sys/fs/inotify/max_user_watches'.\n");
	  }
	}
      }
    }

#endif /* !INHIBIT_INOTIFY_MONITOR */
    ::register_path(initial);
  }

  protected void unregister_path(int|void dying)
  {
    if (wd != -1) {
      // NB: instance may be null if the main object has been destructed
      //     and we've been called via a _destruct().
      if (instance && dying) {
	MON_WERR("### Unregistering from inotify.\n");
	// NB: Inotify automatically removes watches for deleted files,
	//     and will complain if we attempt to remove them too.
	//
	//     Since we have no idea if there's already a queued ID_IGNORED
	//     pending we just throw away any error here.
	mixed err = catch {
	    instance->rm_watch(wd);
	  };
	if (err) {
	  MON_WARN("### Failed to unregister %O: %s\n",
		   path, describe_backtrace(err));
	}
      }
      wd = -1;
    }
    ::unregister_path(dying);
  }
}

constant DefaultMonitor = InotifyMonitor;

#else

constant DefaultMonitor = Monitor;

#endif /* HAVE_EVENTSTREAM || HAVE_INOTIFY */

//! Canonicalize a path.
//!
//! @param path
//!   Path to canonicalize.
//!
//! @returns
//!   The default implementation returns @expr{combine_path(path, ".")@},
//!   i.e. no trailing slashes.
protected string canonic_path(string path)
{
#if HAVE_EVENTSTREAM
  if (!backend) {
    catch {
      path = System.resolvepath(path);
    };
  }
#endif
  return combine_path(path, ".");
}

//! Mapping from monitored path to corresponding @[Monitor].
//!
//! The paths are normalized to @expr{canonic_path(path)@},
//!
//! @note
//!   All filesystems are handled as if case-sensitive. This should
//!   not be a problem for case-insensitive filesystems as long as
//!   case is maintained.
protected mapping(string:Monitor) monitors = ([]);

//! Heap containing active @[Monitor]s that need polling.
//!
//! The heap is sorted on @[Monitor()->next_poll].
protected ADT.Heap monitor_queue = ADT.Heap();

//! Mutex controlling access to @[monitor_queue].
protected Thread.Mutex monitor_mutex = Thread.Mutex();

//! Create a new monitor.
//!
//! @param max_dir_check_interval
//!   Override of @[default_max_dir_check_interval].
//!
//! @param file_interval_factor
//!   Override of @[default_file_interval_factor].
//!
//! @param stable_time
//!   Override of @[default_stable_time].
protected void create(int|void max_dir_check_interval,
		      int|void file_interval_factor,
		      int|void stable_time)
{
  if (max_dir_check_interval > 0) {
    this::max_dir_check_interval = max_dir_check_interval;
  }
  if (file_interval_factor > 0) {
    this::file_interval_factor = file_interval_factor;
  }
  if (stable_time > 0) {
    this::stable_time = stable_time;
  }
  clear();
}

protected void _destruct()
{
  // Destruct monitors before we're destructed ourselves, since they
  // will attempt to unregister with us.
  foreach (monitors;; Monitor m)
    destruct (m);
}

//! Clear the set of monitored files and directories.
//!
//! @note
//!   Due to circular datastructures, it's recomended
//!   to call this function prior to discarding the object.
void clear()
{
  mixed key = monitor_mutex->lock();
  monitors = ([]);
  monitor_queue = ADT.Heap();
#if HAVE_EVENTSTREAM
  eventstream = 0;
#elseif HAVE_INOTIFY
  instance = 0;
#endif
}

//! Release a single @[Monitor] from monitoring.
//!
//! @seealso
//!   @[release()]
protected void release_monitor(Monitor m)
{
  mixed key = monitor_mutex->lock();
  monitor_queue->remove(m);
}

//! Update the position in the @[monitor_queue] for the monitor @[m]
//! to account for an updated next_poll value.
protected void adjust_monitor(Monitor m)
{
  // NB: May be called with monitors not on the monitor_queue due
  //     to double checks when using acceleration.
  if (m->pos < 0) return;	// Not on the monitor_queue.

  mixed key = monitor_mutex->lock();
  if (m->pos < 0) return;	// Not on the monitor_queue any more (race).

  monitor_queue->adjust(m);
  if (monitor_queue->peek() != m) {
    return;
  }

  if (co_id) {
    // Nonblocking mode and we need to poll earlier,
    // so reschedule the call_out.
    reschedule_backend_check();
  }
}

//! Create a new @[Monitor] for a @[path].
//!
//! This function is called by @[monitor()] to create a new @[Monitor]
//! object.
//!
//! The default implementation just calls @[DefaultMonitor] with the
//! same arguments.
//!
//! @seealso
//!   @[monitor()], @[DefaultMonitor]
protected DefaultMonitor monitor_factory(string path, MonitorFlags|void flags,
					 int(0..)|void max_dir_check_interval,
					 int(0..)|void file_interval_factor,
					 int(0..)|void stable_time)
{
  return DefaultMonitor(path, flags, max_dir_check_interval,
			file_interval_factor, stable_time);
}


//! Register a @[path] for monitoring.
//!
//! @param path
//!   Path to monitor.
//!
//! @param flags
//!   @int
//!     @value 0
//!       Don't recurse.
//!     @value 1
//!       Monitor the entire subtree, and any directories
//!       or files that may appear later.
//!     @value 3
//!       Monitor the entire subtree, and any directories
//!       or files that may appear later. Remove the monitor
//!       automatically when @[path] is deleted.
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
//! @param stable_time
//!   Override of @[default_stable_time] for this path
//!   or subtree.
//!
//! @seealso
//!   @[release()]
Monitor|void monitor(string path, MonitorFlags|void flags,
	     int(0..)|void max_dir_check_interval,
	     int(0..)|void file_interval_factor,
	     int(0..)|void stable_time)
{
  path = canonic_path(path);
  if(filter_file(path)) return;
  Monitor m = monitors[path];
  if (m) {
    if (!(flags & MF_AUTO)) {
      // The new monitor is added by hand.
      // Adjust the monitor.
      m->flags = flags;
      m->max_dir_check_interval = max_dir_check_interval;
      m->file_interval_factor = file_interval_factor;
      m->stable_time = stable_time;
      m->next_poll = 0;
      adjust_monitor(m);
    }
    if (flags & MF_HARD) {
      m->flags |= MF_HARD;
    }
    // For the other cases there's no need to do anything,
    // since we can keep the monitor as-is.
  } else {
    m = monitor_factory(path, flags, max_dir_check_interval,
			file_interval_factor, stable_time);
    monitors[path] = m;
    // NB: Registering with the monitor_queue is done as
    //     needed by register_path() as called by create().
  }
  return m;
}

int filter_file(string path)
{
  array x = path/"/";
  foreach(x;; string pc)
    if(pc && strlen(pc) && pc[0]=='.' && pc != "..") {
      MON_WERR("skipping %O\n", path);
      return 1;
    }

  return 0;
}

//! Release a @[path] from monitoring.
//!
//! @param path
//!   Path to stop monitoring.
//!
//! @param flags
//!   @int
//!     @value 0
//!       Don't recurse.
//!     @value 1
//!       Release the entire subtree.
//!     @value 3
//!       Release the entire subtree, but only those paths that were
//!       added automatically by a recursive monitor.
//!   @endint
//!
//! @seealso
//!   @[monitor()]
void release(string path, MonitorFlags|void flags)
{
  path = canonic_path(path);
  Monitor m = m_delete(monitors, path);
  if (!m) return;

  release_monitor(m);
  if (flags && m->st && m->st->isdir) {
    if (!sizeof(path) || path[-1] != '/') {
      path += "/";
    }
    foreach(monitors; string mpath; m) {
      if (has_prefix(mpath, path)) {
	m->check_for_release(flags, flags);
      }
    }
  }
}

//! Check whether a path is monitored or not.
//!
//! @param path
//!   Path to check.
//!
//! @returns
//!   Returns @expr{1@} if there is a monitor on @[path],
//!   and @expr{0@} (zero) otherwise.
//!
//! @seealso
//!   @[monitor()], @[release()]
int(0..1) is_monitored(string path)
{
  return !!monitors[canonic_path(path)];
}

//! Check a single @[Monitor] for changes.
//!
//! @param m
//!   @[Monitor] to check.
//!
//! @param flags
//!   @int
//!     @value 0
//!       Don't recurse.
//!     @value 1
//!       Check all monitors for the entire subtree rooted in @[m].
//!   @endint
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
//! @note
//!   The return value can not be trusted to return @expr{1@} for all
//!   detected changes in recursive mode.
//!
//! @seealso
//!   @[check()], @[data_changed()], @[attr_changed()], @[file_created()],
//!   @[file_deleted()], @[stable_data_change()]
protected int(0..1) check_monitor(Monitor m, MonitorFlags|void flags)
{
  return m && m->check(flags);
}

//! Check all monitors for changes.
//!
//! @param ret_stats
//!   Optional mapping that will be filled with statistics (see below).
//!
//! All monitored paths will be checked for changes.
//!
//! @note
//!   You typically don't want to call this function, but instead
//!   @[check()].
//!
//! @note
//!   Any callbacks will be called from the same thread as the one
//!   calling @[check()].
//!
//! @seealso
//!   @[check()], @[monitor()]
void check_all(mapping(string:int)|void ret_stats)
{
  int cnt;
  int scanned_cnt;
  foreach(monitors; string path; Monitor m) {
    scanned_cnt++;
    mixed err = catch {
	cnt += check_monitor(m);
      };
    if (err) {
      master()->handle_error(err);
    }
  }
  if (ret_stats) {
    ret_stats->num_monitors = sizeof(monitors);
    ret_stats->scanned_monitors = scanned_cnt;
    ret_stats->updated_monitors = cnt;
    ret_stats->idle_time = 0;
  }
}

//! Check for changes.
//!
//! @param max_wait
//!   Maximum time in seconds to wait for changes. @expr{-1@}
//!   for infinite wait.
//!
//! @param max_cnt
//!   Maximum number of paths to check in this call. @expr{0@}
//!   (zero) for unlimited.
//!
//! @param ret_stats
//!   Optional mapping that will be filled with statistics (see below).
//!
//! A suitable subset of the monitored files will be checked
//! for changes.
//!
//! @returns
//!   The function returns when either a change has been detected
//!   or when @[max_wait] has expired. The returned value indicates
//!   the number of seconds until the next call of @[check()].
//!
//!   If @[ret_stats] has been provided, it will be filled with
//!   the following entries:
//!   @mapping
//!     @member int "num_monitors"
//!       The total number of active monitors when the scan completed.
//!     @member int "scanned_monitors"
//!       The number of monitors that were scanned for updates during the call.
//!     @member int "updated_monitors"
//!       The number of monitors that were updated during the call.
//!     @member int "idle_time"
//!       The number of seconds that the call slept.
//!   @endmapping
//!
//! @note
//!   Any callbacks will be called from the same thread as the one
//!   calling @[check()].
//!
//! @seealso
//!   @[check_all()], @[monitor()]
int check(int|void max_wait, int|void max_cnt,
	  mapping(string:int)|void ret_stats)
{
#if HAVE_INOTIFY
  if (instance) {
    /* FIXME: No statistics currently available. */
    instance->poll();
  }
#endif
  int scan_cnt = max_cnt;
  int scan_wait = max_wait;
  while(1) {
    int ret = max_dir_check_interval;
    int cnt;
    int t = time();
    if (sizeof(monitor_queue)) {
      // NB: peek() can apparently in some circumstances throw errors.
      //     cf [bug 7644]. The likely cause being that a different
      //     thread removed the last element during the call. Make
      //     sure not to propagate the error to the caller.
      mixed err = catch {
	  Monitor m;
	  while ((m = monitor_queue->peek()) && (m->next_poll <= t)) {
	    cnt += check_monitor(m);
	    if (!(--scan_cnt)) {
	      m = monitor_queue->peek();
	      break;
	    }
	  }
	  if (m) {
	    ret = m->next_poll - t;
	    if (ret <= 0) ret = 1;
	  } else {
	    scan_cnt--;
	  }
	};
      if (err) {
	master()->handle_error(err);
      }
    }
    if (cnt || !scan_wait || !scan_cnt) {
      if (ret_stats) {
	ret_stats->num_monitors = sizeof(monitors);
	ret_stats->scanned_monitors = max_cnt - scan_cnt;
	ret_stats->updated_monitors = cnt;
	ret_stats->idle_time = max_wait - scan_wait;
      }
      return ret;
    }
    if (ret < scan_wait) {
      scan_wait -= ret;
      sleep(ret);
    } else {
      if (scan_wait > 0) scan_wait--;
      sleep(1);
    }
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
  this::backend = backend;
#if HAVE_EVENTSTREAM
#elif HAVE_INOTIFY
  if (instance) {
    instance->set_backend(backend || Pike.DefaultBackend);
  }
#endif
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
#if HAVE_INOTIFY
  if (instance) {
    instance->set_blocking();
  }
#endif
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

  int t;
  mixed err = catch {
      t = check(0);
    };
  set_nonblocking(t);
  if (err) throw(err);
}

//! Reschedule beckend check.
//!
//! @param suggested_t
//!   Suggested time in seconds until next call of @[check()].
//!
//! Register suitable callbacks with the backend to automatically
//! call @[check()].
//!
//! @[check()] and thus all the callbacks will be called from the
//! backend thread.
protected void reschedule_backend_check(int|void suggested_t)
{
  // NB: Other stuff than plain files may be used with the monitoring
  //     system, so the call_out may be needed even with accelerators.
  //
  // NB: Also note that Inotify doesn't support monitoring of non-existing
  //     paths, so it still needs the call_out-loop.
  MON_WERR("Rescheduling call_out.\n");

  int t = max_dir_check_interval;
  if (sizeof(monitor_queue)) {
    Monitor m = monitor_queue->peek();
    if (m) {
      t = m->next_poll - time(1);
    }
    if (t > max_dir_check_interval) t = max_dir_check_interval;
  }
  if (!undefinedp(suggested_t) && (suggested_t < t)) {
    t = suggested_t;
  }
  if (t < 0) t = 0;

  if (co_id) {
    if (backend) backend->remove_call_out(co_id);
    else remove_call_out(co_id);
  }
  if (backend) co_id = backend->call_out(backend_check, t);
  else co_id = call_out(backend_check, t);
}

//! Turn on nonblocking mode.
//!
//! @param suggested_t
//!   Suggested time in seconds until next call of @[check()].
//!
//! Register suitable callbacks with the backend to automatically
//! call @[check()].
//!
//! @[check()] and thus all the callbacks will be called from the
//! backend thread.
//!
//! @note
//!   If nonblocking mode is already active, this function will
//!   be a noop.
//!
//! @seealso
//!   @[set_blocking()], @[check()].
void set_nonblocking(int|void suggested_t)
{
#if HAVE_INOTIFY
  if (instance) {
    instance->set_nonblocking();
  }
#endif
  if (co_id) return;
  reschedule_backend_check(suggested_t);
}

//! Set the @[max_dir_check_interval].
void set_max_dir_check_interval(int max_dir_check_interval)
{
  if (max_dir_check_interval > 0) {
    this::max_dir_check_interval = max_dir_check_interval;
  } else {
    this::max_dir_check_interval = default_max_dir_check_interval;
  }
}

//! Set the @[file_interval_factor].
void set_file_interval_factor(int file_interval_factor)
{
  if (file_interval_factor > 0) {
    this::file_interval_factor = file_interval_factor;
  } else {
    this::file_interval_factor = default_file_interval_factor;
  }
}

//! Set the @[stable_time].
void set_stable_time (int stable_time)
{
  if (stable_time > 0) {
    this::stable_time = stable_time;
  } else {
    this::stable_time = default_stable_time;
  }
}
