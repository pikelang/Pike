#pike __REAL_VERSION__

// base classes

class Person
{
   string nick; // Mirar
   string user; // mirar
   string ip;   // mistel.idonex.se
   int last_action; // time_t
   multiset aka=(<>);
   string realname="?";
   string server=0;

   mapping misc=([]);
   multiset channels=(<>);

   void say(string message);
   void notice(string message);
   void me(string what);
}

class Channel
{
   string name;

   void	not_message(Person who,string message);
   void	not_join(Person who);
   void	not_part(Person who,string message,Person executor);
   void	not_mode(Person who,string mode);
   void	not_failed_to_join();
   void not_invite(Person who);
}

