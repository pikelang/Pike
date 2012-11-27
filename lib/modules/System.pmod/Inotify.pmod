#pike __REAL_VERSION__

/* vim:syntax=lpc
 */

//! @ignore
#if !constant (System._Inotify.parse_event)
constant this_program_does_not_exist = 1;
#else
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

//! More convenient interface to inotify(7). Automatically reads events
//! from the inotify file descriptor and parses them.
//!
class Instance {
    protected object instance;
    protected Stdio.File file;

    class Watch {
	string name;
	function(int, int, string, mixed ...:void) callback;
	int mask;
	array extra;

	void create(string name, int mask,
		    function(int, int, string, mixed ...:void) callback,
		    array extra) {
	    this_program::name = name;
	    this_program::mask = mask;
	    this_program::callback = callback;
	    this_program::extra = extra;
	}
    }

    class DirWatch {
	inherit Watch;

	void handle_event(int wd, int mask, int cookie,
			  int|string name) {
	    if (name) {
		name = (has_suffix(this_program::name, "/")
			? this_program::name
			: (this_program::name+"/")) + name;
	    } else {
		name = this_program::name;
	    }

	    callback(mask, cookie, name, @extra);
	}
    }

    class FileWatch {
	inherit Watch;

	void handle_event(int wd, int mask, int cookie,
			  int|string name) {
	    callback(mask, cookie, this_program::name, @extra);
	}
    }

    mapping(int:object) watches = ([]);

    void parse(mixed id, string data) {
	while (sizeof(data)) {
	    array a = parse_event(data);
	    object watch;

	    if (watch = watches[a[0]]) {
		watch->handle_event(a[0], a[1], a[2], a[3]);
	    }

	    data = data[a[4]..];
	}
    }

    void create() {
	instance = _Instance();
	file = Stdio.File(instance->get_fd(), "r");
	file->set_nonblocking();
	file->set_read_callback(parse);
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
		  mixed ... extra) {
	int wd = instance->add_watch(filename, mask);

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

    //! Remove the watch associated with the watch descriptor @expr{wd@}.
    void rm_watch(int wd) {
	if (!has_index(watches, wd)) {
	    error("Watch %d does not exist.\n", wd);
	}

	m_delete(watches, wd);
	instance->rm_watch(wd);
    }
}
#endif
