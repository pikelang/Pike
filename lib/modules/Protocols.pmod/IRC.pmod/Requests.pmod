#pike __REAL_VERSION__

/*
PASS gurka
NICK Mirar^
USER mirar mistel.idonex.se irc.starchat.net :Mirar is testing
*/

string __cvs_id="$Id$";

import ".";

class Request
{
   function callback;
   string cmd=0;
   
   void async(object con,mixed ...args)
   {
      con->transmit_noreply(cmd,encode(args));
      if (callback) call_out(callback,0,1);
   }

   int(1..1) sync(object con,mixed ...args)
   { 
      con->transmit_noreply(cmd,encode(args));
      return 1;
   }

   void got_answer(mixed s)
   {
      if (callback) callback(decode_answer(s));
   }

   void got_error(object error)
   {
      if (callback) callback(error);
   }

   string encode(mixed ...);
   mixed decode_answer(string s);
}

class NoReply
{
   string source=
"#"+__LINE__+" \""+__FILE__+#" (NoReply.%cmd%)\"
inherit Protocols.IRC.Requests.Request;

string cmd=\"%cmd%\";

string encode(array args)
{
    %encode%
}

mixed decode_answer(string s)
{
    return 1;
}
";

   string cmd;
   program p;
   
   void create(string _cmd,string ...args)
   {
      source=replace(source,"%cmd%",cmd=_cmd);
      array format=({});
      int i=0;
      foreach (args,string type)
      {
	 switch (type)
	 {
	    case "string": format+=({"%s"}); break;
	    case "text":   format+=({":%s"}); break;
	    default: Error.internal("didn't expect type %O",type);
	 }
      }
      source=replace(source,"%encode%",
		     "return sprintf(\""
		     +format*" "+"\",@args);");
   }

   object `()(mixed ... args)
   {
      if (!p) 
      { 
	 p=compile_string(source,"NoReply."+cmd); 
	 source=0; 
      }
      return p(@args);
   }
}


object pass=NoReply("PASS","string");
object nick=NoReply("NICK","string");
object user=NoReply("USER","string","string","string","text");
object pong=NoReply("PONG","text");
object ping=NoReply("PING","text");
object privmsg=NoReply("PRIVMSG","string","text");
object notice=NoReply("NOTICE","string","text");
object join=NoReply("JOIN","string");
object join2=NoReply("JOIN","string","text");
object part=NoReply("PART","string");
object names=NoReply("NAMES","string");
object who=NoReply("WHO","string");
object kick=NoReply("KICK","string","string","text");

class mode
{
   inherit Request;

   string cmd="MODE";

   string encode(array args)
   {
      switch (sizeof(args))
      {
         case 2:  
            return sprintf("%s %s",@args);
         case 3:  
            return sprintf("%s %s :%s",@args);
         default:
            error("illegal number of args to MODE");
      }
   }

   mixed decode_answer(string s)
   {
      return 1;
   }
};

