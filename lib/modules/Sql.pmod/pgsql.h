/*
 * This is the pgsql config header
 *
 * WARNING: changing this file will not automatically result in recompiling
 * any precompiled pgsql.o or pgsql_utils.o
 */

//#define PG_DEBUG	1
//#define PG_DEBUGMORE	1
//#define PG_DEBUGRACE	1

//#define PG_STATS	1	    // Collect extra usage statistics
#define PG_DEBUGHISTORY	     0	    // If >0, it is the number of records
				    // we keep history on the connection
				    // with the database
#define PG_DEADLOCK_SENTINEL 0	    // If >0, defines the number seconds
				    // a lock can be held before the deadlock
				    // report is being dumped to stderr

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
#define MINPINGINTERVAL	     4	    // minimum seconds between ping()s
#define BACKOFFDELAY	     1

#define PGSQL_DEFAULT_PORT   5432
#define PGSQL_DEFAULT_HOST   "localhost"
#define PREPSTMTPREFIX	     "pike_prep_"
#define PTSTMTPREFIX	     "pike_tprep_"
#define PORTALPREFIX	     "pike_portal_"
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
#define CIDROID		650
#define FLOAT4OID	700
#define FLOAT8OID	701
#define MACADDROID	829
#define INETOID		869
#define BPCHAROID	1042
#define VARCHAROID	1043
#define DATEOID		1082
#define TIMEOID		1083
#define TIMESTAMPOID	1114
#define TIMESTAMPTZOID	1184
#define INTERVALOID	1186
#define TIMETZOID	1266
#define CTIDOID		1247
#define NUMERICOID	1700
#define UUIDOID		2950
#define INT4RANGEOID	3904
#define TSRANGEOID	3908
#define TSTZRANGEOID	3910
#define DATERANGEOID	3912
#define INT8RANGEOID	3926

#define UTF8CHARSET	"UTF8"
#define CLIENT_ENCODING	"client_encoding"
#define NUMERIC_MAGSTEP 10000

#define DERROR(msg ...)		({sprintf(msg),backtrace()})
#define SERROR(msg ...)		(sprintf(msg))
#define USERERROR(msg)		throw(msg)
#define SUSERERROR(msg ...)	USERERROR(SERROR(msg))

#ifdef PG_DEBUG
#define PD(X ...)            werror(X)
			     // PT() puts this in the backtrace
#define PT(X ...)	     (lambda(object _this){(X);}(this))
#else
#undef PG_DEBUGMORE
#define PD(X ...)	     0
#define PT(X ...)	     (X)
#endif

#ifdef PG_DEBUGRACE
#define CHAIN(x)	((x)->chain)
#else
#define CHAIN(x)	(x)
#define conxsess	conxion
#endif

#define KEEP		0		// Sendcmd subcommands
#define SENDOUT		1
#define FLUSHSEND	2
#define FLUSHLOGSEND	3
#define SYNCSEND	4
// If this is extended, change the type of stashflushmode

#define NOTRANS         0
#define TRANSBEGIN      1
#define TRANSEND        2
