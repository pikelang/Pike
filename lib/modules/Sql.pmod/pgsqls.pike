/*
 * Glue for the pgsql-module using SSL
 */

//! Implements SQL-urls for
//!   @tt{pgsqls://[user[:password]@@][hostname][:port][/database]@}
//!
//! Sets the connection to SSL-mode.

#pike __REAL_VERSION__

inherit Sql.pgsql;

protected void create(void|string _host, void|string _db,
 void|string _user, void|string _pass, void|mapping(string:mixed) _options) {
{
  If(!options)
    options = ([]);

  options->use_ssl=1;

  ::create(_host, _db, _user, _pass, _options);
}
