#pike __REAL_VERSION__

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

//! Abstract class for a IRC channel.
class Channel
{
  //! The name of the channel.
  string name;

   void	not_message(Person who,string message);
   void	not_join(Person who);
   void	not_part(Person who,string message,Person executor);
   void	not_mode(Person who,string mode);
   void	not_failed_to_join();
   void not_invite(Person who);
}

