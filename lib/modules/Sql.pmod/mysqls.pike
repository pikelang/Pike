/*
 * $Id: mysqls.pike,v 1.5 2004/04/16 12:12:46 grubba Exp $
 *
 * Glue for the Mysql-module using SSL
 */

//! Implements SQL-urls for
//!   @tt{mysqls://[user[:password]@@][hostname][:port][/database]@}
//!
//! Sets the connection to SSL-mode, and sets the default configuration
//! file to @expr{"/etc/my.cnf"@}.
//!
//! @fixme
//!   Ought to load a suitable default configuration file for Win32 too.
//!
//! @note
//!   This connection method only exists if the Mysql-module has been
//!   compiled with SSL-support.

#pike __REAL_VERSION__

#if constant(Mysql.mysql.CLIENT_SSL)

inherit Sql.mysql;

void create(string host,
	    string db,
	    string user,
	    string password,
	    mapping(string:mixed)|void options)
{
  if (!mappingp(options))
    options = ([ ]);

  options->connect_options |= CLIENT_SSL;

  if (!options->mysql_config_file)
    options->mysql_config_file = "/etc/my.cnf";

  ::create(host||"", db||"", user||"", password||"", options);
}
#endif
