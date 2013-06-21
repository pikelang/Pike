#pike __REAL_VERSION__

//! The @[System.FSEvents] module provides an interface to FSEvents.
//! FSEvents is an API in Mac OS X which allows an application to register for 
//! notifications of changes to a given directory tree without forcing the 
//! application to continously poll the directory tree.
//! 
//! This module is designed for use in asynchronous, or backend mode. That is, 
//! rather than polling for changes, a function you specify will be called when 
//! events of interest occur. This module assumes the presence and use of
//! a CFRunLoop based Backend object, otherwise this module will not receive
//! events from the OS. The CFRunLoop Backend is a new feature that is available 
//! in development versions of Pike 7.9.6 and greater running on Mac OS X 10.5 and 
//! higher.

// if necessary, inherit the C language module here.
#if constant(System._FSEvents)
inherit System._FSEvents;
#else
constant this_program_does_not_exist = 1;
#endif

//!  describe the event flags associated with an event.
//!
//! @returns
//!   a string describing the flags set.
string describe_event_flag(int mask) {
        array(string) list = ({});
        
        foreach (indices(this_program);; string name) {
                if (has_prefix(name, "kFSEventStreamEventFlag")) {
                        int value = `[](this_program, name);

                        if (value & mask) list += ({ name });
                }
        }

        return list * "|";
}
