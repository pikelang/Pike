#pike __REAL_VERSION__
#require constant(System._Inotify.parse_event)

/* vim:syntax=lpc
 */

//! @ignore
inherit System._Inotify;
//! @endignore


//! Turn an event mask into a human readable representation. This is used
//! for debugging purpose.
//!
string describe_mask(int mask) {
    array(string) list = ({});

    foreach (sort(indices(this_program));; string name) {
	if (has_prefix(name, "IN_")) {
	    int value = `[](this_program, name);

	    if (value == (value & mask)) list += ({ name });
	}
    }

    return list * "|";
}

class Watch {
    string name;
    function(int, int, string, mixed ...:void) callback;
    int mask;
    array extra;

    protected void create(string name, int mask,
			  function(int, int, string, mixed ...:void) callback,
			  array extra) {
	this::name = name;
	this::mask = mask;
	this::callback = callback;
	this::extra = extra;
    }
}

class DirWatch {
    inherit Watch;

    void handle_event(int wd, int mask, int cookie,
		      int|string name) {
	if (name) {
	    name = (has_suffix(this::name, "/")
		    ? this::name : (this::name+"/")) + name;
	} else {
	    name = this::name;
	}

	callback(mask, cookie, name, @extra);
    }
}

class FileWatch {
    inherit Watch;

    void handle_event(int wd, int mask, int cookie,
		      int|string name) {
	callback(mask, cookie, this::name, @extra);
    }
}

//! More convenient interface to inotify(7). Automatically reads events
//! from the inotify file descriptor and parses them.
//!
//! @note
//!	Objects of this class will be destructed when they go out of external
//!	references. As such they behave differently from other classes which use
//!	callbacks, e.g. @[Stdio.File].
//! @note
//!	The number of inotify instances is limited by ulimits.
class Instance {
    inherit _Instance;
    protected mapping(int:object) watches = ([]);

    void event_callback(int wd, int event, int cookie, string path)
    {
	Watch watch = watches[wd];
	if (watch) {
	    if (event == IN_IGNORED) {
		m_delete(watches, wd);
		werror("Watch %O (wd: %d) deleted.\n", watch, wd);
	    }
	    watch->handle_event(wd, event, cookie, path);
	}
    }

    protected void create() {
	set_event_callback(event_callback);
	set_nonblocking();
    }

     //! Add a watch for a certain file and a set of events specified by
     //! @expr{mask@}. The function @expr{callback@} will be called when
     //! such an event occurs. The arguments to the callback will be the
     //! events mask, the cookie, the @expr{filename@} and @expr{extra@}.
     //! @returns
     //!	    Returns a watch descriptor which can be used to remove the
     //!	    watch.
     //! @note
     //!	    When adding a second watch for the same file the old one
     //!	    will be removed unless @[System.Inotify.IN_MASK_ADD] is
     //!	    contained in @expr{mask@}.
     //! @note
     //!	    The mask of an event may be a subset of the mask given when
     //!	    adding the watch.
     //! @note
     //!	    In case the watch is added for a regular file, the filename
     //!	    will be passed to the callback. In case the watch is added
     //!	    for a directory, the name of the file to which the event
     //!	    happened inside the directory will be concatenated.
     //!
    int add_watch(string filename, int mask,
		  function(int, int, string, mixed ...:void) callback,
		  mixed ... extra)
    {
	int wd = ::add_watch(filename, mask);

//! @ignore
# if constant(@module@.IN_MASK_ADD)
	/*
	 * just in case we want to use the mask in the watches later,
	 * they better be correct
	 */
	if ((mask & IN_MASK_ADD) && has_index(watches, wd)) {
		mask |= watches[wd]->mask;
	}
# endif
//! @endignore
	if (Stdio.is_dir(filename)) {
	    watches[wd] = DirWatch(filename, mask, callback, extra);
	} else {
	    watches[wd] = FileWatch(filename, mask, callback, extra);
	}

	return wd;
    }
}
