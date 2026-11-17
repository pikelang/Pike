/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/*
 * Pike interface to ODBC compliant databases.
 *
 */

#ifndef PIKE_PRECOMPILED_ODBC_H
#define PIKE_PRECOMPILED_ODBC_H

/*
 * Includes
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_ODBC
#ifdef HAVE_ISQL_H
#include <isql.h>
#else /* !HAVE_ISQL_H */
#ifdef HAVE_QEODBC_H
#include <qeodbc.h>
#else
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#else /* !HAVE_WINDOWS_H */
#ifndef HAVE_SQL_H
#undef HAVE_ODBC
#endif /* !HAVE_SQL_H */
#endif /* HAVE_WINDOWS_H */
#endif /* HAVE_QEODBC_H */
#endif /* HAVE_ISQL_H */
#endif /* HAVE_ODBC */

#ifdef HAVE_ODBC

#ifndef HAVE_ISQL_H
#ifdef HAVE_SQL_H
#include <sql.h>
#endif /* HAVE_SQL_H */
#endif /* !HAVE_ISQL_H */

#ifdef HAVE_ISQLEXT_H
#include <isqlext.h>
#else /* !HAVE_ISQLEXT_H */
#ifdef HAVE_SQLEXT_H
#include <sqlext.h>
#endif /* HAVE_SQLEXT_H */
#endif /* HAVE_ISQLEXT_H */

#if SIZEOF_SQLWCHAR == 2
#define PRINTSQLWCHAR	"w"
#elif SIZEOF_SQLWCHAR == 4
#define PRINTSQLWCHAR	"ll"
#endif

/*
 * Globals
 */

extern struct program *Odbc_odbc_program;
extern struct program *Odbc_Result_program;
extern struct program *Odbc_TypedResult_program;

extern int Odbc_Result_program_fun_num;
extern int Odbc_TypedResult_program_fun_num;

/*
 * Typedefs
 */

#ifndef HAVE_SQLSMALLINT
typedef SWORD	SQLSMALLINT;
#endif /* HAVE_SQLSMALLINT */

#ifndef HAVE_SQLUSMALLINT
typedef UWORD	SQLUSMALLINT;
#endif /* HAVE_SQLUSMALLINT */

#ifndef HAVE_SQLINTEGER
typedef SDWORD	SQLINTEGER;
#endif /* !HAVE_SQLINTEGER */

#ifndef HAVE_SQLUINTEGER
typedef UDWORD	SQLUINTEGER;
#endif /* !HAVE_SQLUINTEGER */

#ifndef HAVE_SQLLEN
typedef SQLINTEGER	SQLLEN;
#endif /* !HAVE_SQLLEN */

#ifndef HAVE_SQLULEN
typedef SQLUINTEGER	SQLULEN;
#endif /* !HAVE_SQLLEN */

#ifndef HAVE_SQLHENV
typedef HENV	SQLHENV;
#endif /* !HAVE_SQLENV */

#ifndef HAVE_SQLHDBC
typedef HDBC	SQLHDBC;
#endif /* !HAVE_SQLHDBC */

#ifndef HAVE_SQLHSTMT
typedef HSTMT	SQLHSTMT;
#endif /* !HAVE_SQLSTMT */

/*
 * Structures
 */

typedef void (*field_factory_func)(int);

struct field_info {
  SWORD type;
  SWORD bin_type;
  SWORD scale;
  SQLULEN size;
  SQLULEN bin_size;
  field_factory_func factory;
};

struct Odbc_odbc_struct;

/* References are held by the connection, its results and native calls.
 * They are changed with the interpreter lock held. */
struct pike_odbc_lock {
  PIKE_MUTEX_T mutex;
  void *owner;
  int depth;
  unsigned int refs;
};

/*
 * Defines
 */

#define PIKE_ODBC	THIS_ODBC_ODBC
#define PIKE_ODBC_RES	THIS_ODBC_RESULT

/* Flags */
#define PIKE_ODBC_CONNECTED      	1
#define PIKE_ODBC_OLD_TDS_KLUDGE	2
/* String shift used for wide data. */
#define PIKE_ODBC_WDATA_SHIFT_SHIFT	2
#define PIKE_ODBC_WDATA_SHIFT_MASK	12
/* String shift used for wide labels. */
#define PIKE_ODBC_WLABEL_SHIFT_SHIFT	4
#define PIKE_ODBC_WLABEL_SHIFT_MASK	48
/* String shift used for wide queries. */
#define PIKE_ODBC_WQUERY_SHIFT_SHIFT	6
#define PIKE_ODBC_WQUERY_SHIFT_MASK	192

/* http://msdn2.microsoft.com/en-us/library/ms715361.aspx says:
 *
 *   On multithread operating systems, drivers must be thread-safe.
 *   That is, it must be possible for applications to use the same
 *   handle on more than one thread.
 *
 * This means we don't need a lock at all on the connection when we
 * release the interpreter lock. If this really is true remains to be
 * seen..
 */
#define ODBC_ALLOW() THREADS_ALLOW()
#define ODBC_DISALLOW() THREADS_DISALLOW()

/* The same for calls on the connection ODBC or on its results statements, which the connection's lock serializes. */
#ifdef PIKE_THREADS
#define ODBC_SELF() ((void *)Pike_interpreter.thread_state)
#else
#define ODBC_SELF() ((void *)1)
#endif
#define ODBC_ALLOW_ON(ODBC) do {                                      \
    struct Odbc_odbc_struct *odbc_conn_ = (ODBC);                     \
    struct pike_odbc_lock *odbc_lock_ =                               \
      (odbc_conn_ ? pike_odbc_ref_lock(odbc_conn_) : NULL);           \
    void *odbc_self_ = ODBC_SELF();                                   \
    THREADS_ALLOW();                                                  \
    if (odbc_lock_) low_odbc_lock(odbc_lock_, odbc_self_)
#define ODBC_DISALLOW_ON()                                            \
    if (odbc_lock_) low_odbc_unlock(odbc_lock_);                      \
    THREADS_DISALLOW();                                               \
    pike_odbc_free_lock(odbc_lock_);                                  \
  } while (0)

/*
 * Prototypes
 */
SQLHDBC pike_odbc_get_hdbc(struct Odbc_odbc_struct *odbc);
struct pike_odbc_lock *pike_odbc_ref_lock(struct Odbc_odbc_struct *odbc);
void pike_odbc_free_lock(void *lock);
void low_odbc_lock(struct pike_odbc_lock *lock, void *self);
void low_odbc_unlock(struct pike_odbc_lock *lock);
void pike_odbc_lock(struct pike_odbc_lock *lock);
void pike_odbc_unlock(void *lock);
void pike_odbc_set_affected_rows(struct Odbc_odbc_struct *odbc, SQLLEN rows);
unsigned int pike_odbc_get_flags(struct Odbc_odbc_struct *odbc);
void push_sqlwchar(SQLWCHAR *str, size_t len);
void odbc_error(const char *fun, const char *msg,
		struct Odbc_odbc_struct *odbc, SQLHSTMT hstmt,
		RETCODE code, void (*clean)(void *), void *clean_arg);

void init_odbc_res_programs(void);
void exit_odbc_res(void);

#endif /* HAVE_ODBC */

#endif /* PIKE_PRECOMPILED_ODBC_H */
