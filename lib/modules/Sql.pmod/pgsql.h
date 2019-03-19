/*
 * This is the pgsql config header
 *
 * WARNING: changing this file will not automatically result in recompiling
 * any precompiled pgsql.o or pgsql_utils.o
 */

//#define PG_DEBUG  1
//#define PG_DEBUGMORE  1

//#define PG_STATS	1	    // Collect extra usage statistics

#define FETCHLIMIT	     1024   // Initial upper limit on the
				    // number of rows to fetch across the
				    // network at a time
				    // 0 for no chunking
				    // Needs to be >0 for interleaved
				    // portals
#define MINPREPARELENGTH     16	    // statements shorter than this will not
				    // be cached
#define STATEMENTCACHEDEPTH  1024   // Initial maximum cachecountsum for
				    // prepared statements,
#define QUERYTIMEOUT	     4095   // Queries running longer than this number
				    // of seconds are canceled automatically
#define PORTALBUFFERSIZE     (32*1024) // Approximate buffer per portal

#define PGSQL_DEFAULT_PORT   5432
#define PGSQL_DEFAULT_HOST   "localhost"
#define PREPSTMTPREFIX	     "pike_prep_"
#define PTSTMTPREFIX	     "pike_tprep_"
#define PORTALPREFIX	     "pike_portal_"
#define RECONNECTDELAY	     1	    // Initial delay for reconnects
#define RECONNECTBACKOFF     4	    // Secondary delay for reconnect
#define FACTORPLAN	     8	    // Determines criterium when caching plan
				    // -> if parsingtime*FACTORPLAN >= runtime
				    // cache the statement
#define DRIVERNAME	     "pgsql"
#define MARKSTART	     "{""{\n"	      // split string to avoid
#define MARKERROR	     ">"">"">"">"     // foldeditors from recognising
#define MARKEND		     "\n}""}"	      // it as a fold
#define MAGICTERMINATE       42
#define PG_PROTOCOL(m,n)     (((m)<<16)|(n))
#define PGFLUSH		     "H\0\0\0\4"
#define PGSYNC		     "S\0\0\0\4"

#define BOOLOID		16
#define BYTEAOID	17
#define CHAROID		18
#define INT8OID		20
#define INT2OID		21
#define INT4OID		23
#define TEXTOID		25
#define OIDOID		26
#define XMLOID		142
#define FLOAT4OID	700
#define FLOAT8OID	701
#define MACADDROID	829
#define INETOID		869	    /* Force textmode */
#define BPCHAROID	1042
#define VARCHAROID	1043
#define CTIDOID		1247
#define UUIDOID		2950

#define UTF8CHARSET	"UTF8"
#define CLIENT_ENCODING	"client_encoding"

#define DERROR(msg ...)		({sprintf(msg),backtrace()})
#define SERROR(msg ...)		(sprintf(msg))
#define USERERROR(msg)		throw(msg)
#define SUSERERROR(msg ...)	USERERROR(SERROR(msg))

#ifdef PG_DEBUG
#define PD(X ...)            werror(X)
			     // PT() puts this in the backtrace
#define PT(X ...)	     (lambda(object _this){return (X);}(this))
#else
#undef PG_DEBUGMORE
#define PD(X ...)	     0
#define PT(X ...)	     (X)
#endif

#define PORTALINIT	0		// Portal states
#define BOUND		1
#define COPYINPROGRESS	2
#define CLOSING		3
#define CLOSED		4

#define KEEP		0		// Sendcmd subcommands
#define SENDOUT		1
#define FLUSHSEND	2
#define FLUSHLOGSEND	3
#define SYNCSEND	4

#define NOERROR			0	// Error states networkparser
#define PROTOCOLERROR		1
#define PROTOCOLUNSUPPORTED	2
