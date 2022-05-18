#pike __REAL_VERSION__

//
// Filesystem monitor with support for symbolic links.
//
//
// 2010-01-25 Henrik Grubbström
//
//! Filesystem monitor with support for symbolic links.
//!
//! This module extends @[Filesystem.Monitor.basic] with
//! support for symbolic links.
//!
//! @note
//!   For operating systems where symbolic links aren't supported,
//!   this module will behave exactly like @[Filesystem.Monitor.basic].
//!
//! @seealso
//!   @[Filesystem.Monitor.basic]

//! @decl inherit Filesystem.Monitor.basic
inherit "basic.pike" : basic;

#define MON_WERR(X...)	report(NOTICE,	__func__, X)
#define MON_WARN(X...)	report(WARNING,	__func__, X)
#define MON_ERROR(X...)	report(ERROR,	__func__, X)

#if constant(readlink)

//! @decl void attr_changed(string path, Stdio.Stat st)
//!
//! File attribute changed callback.
//!
//! @param path
//!   Path of the file or directory which has changed attributes.
//!
//! @param st
//!   Status information for @[path] as obtained by @expr{file_stat(path)@}.
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
//! @note
//!   It differs from the @[Filesystem.Monitor.basic] version in that
//!   symbolic links have the @[st] of their targets.
//!
//! Overload this to do something useful.

//! @decl void file_exists(string path, Stdio.Stat st)
//!
//! File existance callback.
//!
//! @param path
//!   Path of the file or directory.
//!
//! @param st
//!   Status information for @[path] as obtained by @expr{file_stat(path)@}.
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
//! @note
//!   It differs from the @[Filesystem.Monitor.basic] version in that
//!   symbolic links have the @[st] of their targets.
//!
//! Called by @[check()] and @[check_monitor()] the first time a monitored
//! path is checked (and only if it exists).
//!
//! Overload this to do something useful.

//! @decl void file_created(string path, Stdio.Stat st)
//!
//! File creation callback.
//!
//! @param path
//!   Path of the new file or directory.
//!
//! @param st
//!   Status information for @[path] as obtained by @expr{file_stat(path)@}.
//!
//! This function is called when either a monitored path has started
//! existing, or when a new file or directory has been added to a
//! monitored directory.
//!
//! @note
//!   It differs from the @[Filesystem.Monitor.basic] version in that
//!   symbolic links have the @[st] of their targets.
//!
//! Called by @[check()] and @[check_monitor()].
//!
//! Overload this to do something useful.

//! @decl void stable_data_change(string path, Stdio.Stat st)
//!
//! Stable change callback.
//!
//! @param path
//!   Path of the file or directory that has stopped changing.
//!
//! @param st
//!   Status information for @[path] as obtained by @expr{file_stat(path)@}.
//!
//! This function is called when previous changes to @[path] are
//! considered "stable".
//!
//! "Stable" in this case means that there have been no detected
//! changes for at lease @[stable_time] seconds.
//!
//! @note
//!   It differs from the @[Filesystem.Monitor.basic] version in that
//!   symbolic links have the @[st] of their targets.
//!
//! Called by @[check()] and @[check_monitor()].
//!
//! Overload this to do something useful.

//! Monitoring information for a single filesystem path.
//!
//! With support for expansion of symbolic links.
//!
//! @seealso
//!   @[monitor()]
protected class DefaultMonitor
{
  //! Based on @[Filesystem.Monitor.basic.DefaultMonitor].
  inherit basic::DefaultMonitor;

  //! Mask of symlink ids that can reach this monitor.
  int symlinks;

  //! Call a notification callback and handle symlink expansion.
  //!
  //! @param cb
  //!   Callback to call or @[UNDEFINED] for no operation.
  //!
  //! @param state
  //!   State mapping to avoid multiple notification and infinite loops.
  //!   Call with an empty mapping.
  //!
  //! @param symlinks
  //!   Symlinks that have not been expanded yet.
  //!
  //! @param path
  //!   Path to notify on.
  //!
  //! @param extras
  //!   Extra arguments to @[cb].
  //!
  //! @param symlink
  //!   Symbolic link that must have been followed for the
  //!   callback to be called.
  void low_call_callback(function(string, mixed ...:void) cb,
			 mapping(string:int) state,
			 mapping(string:string) symlink_targets,
			 string path, Stdio.Stat|void st,
			 string|void symlink)
  {
    if (!cb || state[path] || (st && st->islnk)) return;
    state[path] = 1;

    MON_WERR("%O: path: %O, cb: %O, symlink_targets: %O\n",
	     this_function, path, cb, symlink_targets);

    if (!symlink || !symlink_targets[symlink]) {
      cb(path, st);
    }
    if (sizeof(symlink_targets)) {
      // Check the list of symlink targets.
      foreach(reverse(sort(indices(symlink_targets))), string src) {
	string dest = symlink_targets[src];
	if (has_prefix(path, dest)) {
	  low_call_callback(cb, state, symlink_targets - ([ src : dest ]),
			    src + path[sizeof(dest)..], st, symlink);
	}
      }
    }
  }

  //! Call a notification callback and handle symlink expansion.
  //!
  //! @param cb
  //!   Callback to call or @[UNDEFINED] for no operation.
  //!
  //! @param extras
  //!   Extra arguments after the @[path] argument to @[cb].
  protected void call_callback(function(string, mixed ...:void) cb,
			       string path, Stdio.Stat|void st)
  {
    if (!cb) return;
    low_call_callback(cb, ([]), global::symlink_targets, path, st);
  }

  protected void notify_symlink(function(string, mixed ...:void) cb,
				string sym)
  {
    int sym_id = global::symlink_ids[sym];
    if (sym_id) {
      // Depth-first.
      foreach(reverse(sort(filter(values(monitors),
				  lambda(Monitor m, int sym_id) {
				    return m->symlinks & sym_id;
				  }, sym_id)->path)),
	      string m_path) {
	Monitor m = monitors[m_path];
	m->low_call_callback(cb, ([]), global::symlink_targets,
			     m_path, m->st, sym);
      }
    }
  }

  //! Called when the symlink @[path] is no more.
  protected void zap_symlink(string path)
  {
    MON_WERR("%O(%O)\n", this_function, path);

    string old_dest = global::symlink_targets[path];

    if (old_dest) {
      int sym_id = global::symlink_ids[path];
      foreach(monitors; string m_path; Monitor m) {
	if (m->symlinks & sym_id) {
	  MON_WERR("Found monitor %O reached through %O.\n", m, path);
	  m->low_call_callback(global::file_deleted, ([]),
			       global::symlink_targets,
			       m_path, UNDEFINED, path);
	  m->symlinks -= sym_id;
	  // Unregister the monitor if it is the last ref,
	  // and there are no hard links to the file.
	  m->check_for_release(MF_AUTO|MF_HARD, MF_AUTO);
	}
      }
      global::available_ids |= m_delete(symlink_ids, path);
      m_delete(global::symlink_targets, path);
    }
  }

  //! Check whether a symlink has changed.
  protected void check_symlink(string path, Stdio.Stat|zero st,
			       int|void inhibit_notify)
  {
    MON_WERR("%O(%O, %O, %O)...\n", this_function, path, st, inhibit_notify);

    string dest;
    if (st && st->islnk) {
      dest = readlink(path);
      if (dest) {
	dest = canonic_path(combine_path(path, "..", dest));
	if (symlink_targets[path] == dest) return;
      }
    }
    if (symlink_targets[path]) {
      zap_symlink(path);
    }
    if (dest) {
      // We have a new symbolic link.
      MON_WERR("Found new symlink %O ==> %O.\n", path, dest);
      symlink_targets[path] = dest;
      int sym_id = allocate_symlink(path);
      int sym_mask = sym_id | symlink_ids[dest];
      int sym_done = sym_id;
      Monitor m;
      if (!(m = monitors[dest])) {
	MonitorFlags m_flags = (flags & ~MF_HARD) | MF_AUTO;
	if (inhibit_notify) {
	  m_flags &= ~MF_INITED;
	}
	monitor(dest, m_flags,
		max_dir_check_interval,
		file_interval_factor,
		stable_time);
	m = monitors[dest];
      }
      m->symlinks |= sym_id;
      if (!has_suffix(dest, "/")) {
	dest += "/";
      }
      foreach(monitors; string mm_path; Monitor mm) {
	if (has_prefix(mm_path, dest)) {
	  mm->symlinks |= sym_id;
	  sym_mask |= symlink_ids[mm_path];
	}
      }
      // Follow any found symlinks.
      while (sym_mask != sym_done) {
	int mask = sym_mask - sym_done;
	foreach(monitors; string mm_path; Monitor mm) {
	  if ((mm->symlinks & mask) && !(mm->symlinks & sym_id)) {
	    mm->symlinks |= sym_id;
	    sym_mask |= symlink_ids[mm_path];
	  }
	}
	sym_done |= mask;
      }
      if (!inhibit_notify) {
	notify_symlink(global::file_created, path);
      }
    }
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
  //! @note
  //!   If there is a @[data_changed()] callback, it may supersede this
  //!   callback if the file content also has changed.
  protected void attr_changed(string path, Stdio.Stat st)
  {
    check_symlink(path, st);
    if (st && st->islnk) {
      return;
    }
    ::attr_changed(path, st);
  }

  protected void low_file_exists(string path, Stdio.Stat st)
  {
    // Note: May be called for symlink targets before they have
    //       initialized properly, in which case st will be 0.
    if (!st || !global::file_exists) return;
    global::file_exists(path, st);
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
  protected void file_exists(string path, Stdio.Stat st)
  {
    check_symlink(path, st, 1);
    if (st && st->islnk) {
      notify_symlink(low_file_exists, path);
      return;
    }
    ::file_exists(path, st);
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
  protected void file_created(string path, Stdio.Stat st)
  {
    check_symlink(path, st);
    if (st && st->islnk) {
      // Note: check_symlink() above has already called the file_created()
      // callback for us.
      return;
    }
    ::file_created(path, st);
  }

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
  protected void file_deleted(string path, Stdio.Stat old_st)
  {
    check_symlink(path, UNDEFINED);
    if (old_st && old_st->islnk) {
      return;
    }
    ::file_deleted(path, old_st);
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
  protected void stable_data_change(string path, Stdio.Stat st)
  {
    if (st && st->islnk) {
      int sym_id = global::symlink_ids[path];
      if (sym_id) {
	foreach(monitors; string m_path; Monitor m) {
	  if ((m->symlinks & sym_id) && (m->last_change == 0x7fffffff)) {
	    m->low_call_callback(global::stable_data_change, ([]),
				 global::symlink_targets, m_path, m->st, path);
	  }
	}
      }
      return;
    }
    ::stable_data_change(path, st);
  }

  //! Check if this monitor should be removed automatically.
  void check_for_release(int mask, int flags)
  {
    if (symlinks) {
      // We need to check if this is the direct target of a symlink.
      foreach(symlink_targets;; string dest) {
	if (path == dest) {
	  // The monitor still has a symlink pointing to it.
	  return;
	}
      }
    }
    ::check_for_release(mask, flags);
  }

  //! Called to create a sub monitor.
  protected void monitor(string path, int flags, int max_dir_interval,
			 int file_interval_factor, int stable_time)
  {
    object m;
    ::monitor(path, flags, max_dir_check_interval,
	      file_interval_factor, stable_time);
    if((m = monitors[path])) m->symlinks |= symlinks;
  }

  //! Called when the status has changed for an existing file.
  protected int(0..1) status_change(Stdio.Stat old_st, Stdio.Stat st,
				    int orig_flags, int flags)
  {
    check_symlink(path, st);
    if (st && st->islnk) {
      return 1;
    }
    return ::status_change(old_st, st, orig_flags, flags);
  }

}

#endif /* constant(readlink) */

//! Mapping from symlink name to symlink target.
protected mapping(string:string) symlink_targets = ([]);

//! Mapping from symlink name to symlink id.
protected mapping(string:int) symlink_ids = ([]);

//! Bitmask of all unallocated symlink ids.
protected int available_ids = -1;

//! Allocates a symlink id for the link @[sym].
protected int allocate_symlink(string sym)
{
  int res = symlink_ids[sym];
  if (res) return res;
  res = available_ids & ~(available_ids - 1);
  available_ids -= res;
  return symlink_ids[sym] = res;
}
