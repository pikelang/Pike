//
// $Id$
//

#pike 7.7

//! Pike 7.6 compatibility.
//!
//! Some symbols in Protocols.TELNET that now are protected
//! used to be visible in Pike 7.6 and earlier.

//! @decl inherit predef::Protocols.TELNET;
//! Based on the current Protocols.TELNET.

inherit Protocols.TELNET;

// create() and setup() used to be visible in Pike 7.6 and earlier.

//! Pike 7.6 compatibility version of Protocols.TELNET.protocol.
class protocol
{
  //! Based on the current Protocols.TELNET.protocol.
  inherit TELNET::protocol;

  //! create() used to be visible.
  //!
  //! @seealso
  //!   @[::create()]
  void create(object f,
	      function(mixed,string:void) r_cb,
	      function(mixed|void:string) w_cb,
	      function(mixed|void:void) c_cb,
	      mapping callbacks, mixed|void new_id)
  {
    ::create(f, r_cb, w_cb, c_cb, callbacks, new_id);
  }

  //! setup() used to be visible.
  //!
  //! @seealso
  //!   @[::setup()]
  void setup()
  {
    ::setup();
  }
}

//! Pike 7.6 compatibility version of Protocols.TELNET.LineMode.
class LineMode
{
  //! Based on the current Protocols.TELNET.LineMode.
  inherit TELNET::LineMode;

  //! create() used to be visible.
  //!
  //! @seealso
  //!   @[::create()]
  void create(object f,
	      function(mixed,string:void) r_cb,
	      function(mixed|void:string) w_cb,
	      function(mixed|void:void) c_cb,
	      mapping callbacks, mixed|void new_id)
  {
    ::create(f, r_cb, w_cb, c_cb, callbacks, new_id);
  }

  //! setup() used to be visible.
  //!
  //! @seealso
  //!   @[::setup()]
  void setup()
  {
    ::setup();
  }
}

//! Pike 7.6 compatibility version of Protocols.TELNET.Readline.
class Readline
{
  //! Based on the current Protocols.TELNET.Readline.
  inherit TELNET::Readline;

  //! create() used to be visible.
  //!
  //! @seealso
  //!   @[::create()]
  void create(object f,
	      function(mixed,string:void) r_cb,
	      function(mixed|void:string) w_cb,
	      function(mixed|void:void) c_cb,
	      mapping callbacks, mixed|void new_id)
  {
    ::create(f, r_cb, w_cb, c_cb, callbacks, new_id);
  }

  //! setup() used to be visible.
  //!
  //! @seealso
  //!   @[::setup()]
  void setup()
  {
    ::setup();
  }
}

