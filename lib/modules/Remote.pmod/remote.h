// -*- Pike -*-

#define PROTO_VERSION "0001"

#define error(E) throw( ({ E+"\n" , backtrace() }) )

// #define REMOTE_DEBUG

#ifdef REMOTE_DEBUG
# define DEBUGMSG(X) werror(X)
#else
# define DEBUGMSG(X)
#endif

program Context = ((program)"context");
program Connection = ((program)"connection");
program Call = ((program)"call");
program Obj = ((program)"obj");
