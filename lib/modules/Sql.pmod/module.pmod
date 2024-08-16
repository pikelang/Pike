#pike __REAL_VERSION__

//! The SQL module is a unified interface between pike and all
//! its supported databases. The parts of this module that is
//! usuable for all normal uses is the @[Sql] class and the
//! @[sql_result] class.
//!
//! @example
//! string people_in_group(string group) {
//!    Sql.Sql db = Sql.Sql("mysql://localhost/testdb");
//!    return db->query("SELECT name FROM users WHERE "
//!                     "group=%s", group)->name * ",";
//! }

constant QUERY_OPTION_CHARSET = "*charset*";

//! @ignore
// Use getters and Val-> to ensure dynamic resolving in case the
// values in Val.pmod are replaced.
program `->Null() {return Val->Null;}
Val.Null `->NULL() {return Val->null;}
//! @endignore

//! @class Null
//!   Class used to implement the SQL NULL value.
//!
//! @deprecated Val.Null
//!
//! @seealso
//!   @[Val.Null], @[Val.null]

//! @endclass

//! @decl Val.Null NULL;
//!
//!   The SQL NULL value.
//!
//! @deprecated Val.null
//!
//! @seealso
//!   @[Val.null]

//! Redact the password (if any) from an Sql-url.
//!
//! @param sql_url
//!   Sql-url possibly containing an unredacted password.
//!
//! @returns
//!   Returns the same Sql-url but with the password (if any)
//!   replaced by the string @expr{"CENSORED"@}.
string censor_sql_url(string sql_url)
{
  array(string) a = sql_url/"://";
  string prot = a[0];
  string host = a[1..] * "://";
  a = host/"@";
  if (sizeof(a) > 1) {
    host = a[-1];
    a = (a[..<1] * "@")/":";
    string user = a[0];
    if (sizeof(a) > 1) {
      sql_url = prot + "://" + user + ":CENSORED@" + host;
    }
  }
  return sql_url;
}
