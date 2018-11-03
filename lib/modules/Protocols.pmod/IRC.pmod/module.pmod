#pike __REAL_VERSION__

//! IRC client and connection handling.
//! 
//! Start with @ref{Client@} and @ref{Channel@}.
//!
//! @example
//! Protocols.IRC.client irc;
//! class channel_notif
//! {
//!     inherit Protocols.IRC.Channel;
//!     void not_message(object person,string msg)
//!     {
//!         if (msg == "!hello") irc->send_message(name, "Hello, "+person->nick+"!");
//!     }
//! }
//! int main()
//! {
//!     irc = Protocols.IRC.Client("irc.freenode.net", ([
//!         "nick": "DemoBot12345",
//!         "realname": "Demo IRC bot",
//!         "channel_program": channel_notif,
//!     ]));
//!     irc->join_channel("#bot-test");
//!     return -1;
//! }


//! Abstract class for a person.
class Person
{
  //! User nickname, e.g. @expr{"Mirar"@}.
  string nick;

  //! User name, e.g. @expr{"mirar"@}.
  string user;

  //! User domain, e.g. @expr{"mistel.idonex.se"@}.
  string ip;

  //! Time of last action, represented as posix time.
  int last_action;

   multiset aka=(<>);
   string realname="?";
   string server=0;

   mapping misc=([]);
   multiset channels=(<>);

   void say(string message);
   void notice(string message);
   void me(string what);
}

//! Abstract class for an IRC channel.
class Channel
{
   //! The name of the channel.
   string name;

   //! Called whenever a message arrives on this channel.
   void not_message(Person who,string message) { }

   //! Called whenever someone joins this channel.
   void not_join(Person who) { }

   void not_part(Person who,string message,Person executor) { }
   void not_mode(Person who,string mode) { }
   void not_failed_to_join() { }
   void not_invite(Person who) { }
}
