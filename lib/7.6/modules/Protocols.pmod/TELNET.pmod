//
// $Id: TELNET.pmod,v 1.1 2008/12/11 18:16:17 grubba Exp $
//

#pike 7.7

//! Pike 7.6 compatibility.
//!
//! Some symbols in Protocols.TELNET that now are protected
//! used to be visible in Pike 7.6 and earlier.

//! Based on the current Protocols.TELNET.
inherit predef::Protocols.TELNET;

// create() and setup() used to be visible in Pike 7.6 and earlier.

//! Pike 7.6 compatibility version of Protocols.TELNET.protocol.
class protocol
{
  //! Based on the current Protocols.TELNET.protocol.
  inherit ::protocol;

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
  inherit ::LineMode;

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
  inherit ::Readline;

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

