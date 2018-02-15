// -*- Pike -*-

#define PROTO_VERSION "0001"

// #define REMOTE_DEBUG

#ifdef REMOTE_DEBUG
# define DEBUGMSG(X) werror(X)
#else
# define DEBUGMSG(X)
#endif

#define CTX_ERROR      0
#define CTX_OTHER      1
#define CTX_OBJECT     2
#define CTX_FUNCTION   3
#define CTX_PROGRAM    3
#define CTX_CALL_SYNC  4
#define CTX_ARRAY      5
#define CTX_RETURN     6
#define CTX_MAPPING    7
#define CTX_CALL_ASYNC 8
#define CTX_EXISTS     9
#define CTX_EXIST_RES  10
