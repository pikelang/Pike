/*
 * This is the pgsql config header
 *
 * WARNING: changing this file will not automatically result in recompiling
 * any precompiled pgsql.o or pgsql_utils.o
 */

//#define PG_DEBUG  1
//#define PG_DEBUGMORE  1

//#define USEPGsql	1	    // Doesn't use Stdio.FILE, but _PGsql
//#define PG_STATS	1	    // Collect extra usage statistics

#define FETCHLIMIT	     1024   // Initial upper limit on the
				    // number of rows to fetch across the
				    // network at a time
				    // 0 for no chunking
				    // Needs to be >0 for interleaved
				    // portals
#define FETCHLIMITLONGRUN    1	    // for long running background queries
#define STREAMEXECUTES	     1	    // streams executes if defined
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

#ifdef PG_DEBUG
#define PD(X ...)            werror(X)
#else
#undef PG_DEBUGMORE
#define PD(X ...)
#endif

protected enum portalstate {
  portalinit=0,bound,copyinprogress,closed
};

protected enum sctype {
  keep=0,flushsend,sendout
};
