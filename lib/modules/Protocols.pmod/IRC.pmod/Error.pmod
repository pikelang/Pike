#pike __REAL_VERSION__

class StdErr
{
   constant is_generic_error=1;
   constant is_irc_error=1;

   string name;
   array(mixed) __backtrace;

   void create(string _name)
   {
      name=_name;
      __backtrace=backtrace();
      while (!__backtrace[-1][0]||
	     __backtrace[-1][0][sizeof(__backtrace[-1][0])-11..]
	     == "/Error.pmod")
	 __backtrace=__backtrace[0..sizeof(__backtrace)-2];
   }

   mixed `[](mixed z)
   {
      switch (z)
      {
	 case 0: return "Protocols.IRC:  "+name+"\n";
	 case 1: return __backtrace;
      }
      return ::`[](z);
   }
}

class Connection
{
   inherit StdErr;

   constant is_irc_connection_error=1;

   int errno;

   void create(string desc,int _errno)
   {
      ::create(desc+" ("+strerror(errno=_errno)+")");
   }
}

void connection(mixed ...args) { throw(Connection(@args)); }
void internal(string format,mixed ...args) 
   { throw(StdErr(sprintf(format+" (internal error)",@args))); }
