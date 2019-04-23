#pike 7.9

//! Pike 7.8 compatibility.
//!
//! Some symbols in @[predef::Protocols.DNS] that are now protected
//! used to be visible in Pike 7.8 and earlier.

//! @decl inherit predef::Protocols.DNS;
//! Based on the current Protocols.DNS.

inherit Protocols.DNS : DNS;

#pragma no_deprecation_warnings

//! Pike 7.8 compatibility version of @[predef::Protocols.DNS.server].
class server
{
  //! Based on the current Protocols.DNS.server.
  inherit DNS::server;

  //! create() used to be visible.
  //!
  //! @seealso
  //!   @[::create()]
  void create(int|string|void arg1, string|int ... args)
  {
    ::create(arg1, @args);
  }
}

//! Pike 7.8 compatibility version of @[predef::Protocols.DNS.client].
class client
{
  //! Based on the current Protocols.DNS.client.
  inherit DNS::client;

  //! create() used to be visible.
  //!
  //! @seealso
  //!   @[::create()]
  void create(void|string|array(string) server,
	      void|int|array(string) domain)
  {
    ::create(server, domain);
  }
}

//! Pike 7.8 compatibility version of @[predef::Protocols.DNS.async_client].
class async_client
{
  //! Based on the current Protocols.DNS.server.
  inherit DNS::async_client;

  //! create() used to be visible.
  //!
  //! @seealso
  //!   @[::create()]
  void create(void|string|array(string) server,
	      void|string|array(string) domain)
  {
    ::create(server, domain);
  }
}
