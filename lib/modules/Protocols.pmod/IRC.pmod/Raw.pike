#pike __REAL_VERSION__

import ".";

object con;

// #define IRC_DEBUG

function(string,string ...:void) command_callback;
function(string,string ...:void) notify_callback;
function(:void) close_callback;

// command_callback(string cmd,string ... parameters)
// notify_callback(string from,string type,string from,string message)

void create(string server,int port,
	    function(string,string ...:void) _command_callback,
	    void|function(string,string...:void) _notify_callback,
	    void|function(:void) _close_callback)
{
   array aserver=gethostbyname(server);
   if (!aserver || !sizeof(aserver[1]))
      Error.connection("Failed to lookup host %O",server);
   server=aserver[1][random(sizeof(aserver[1]))];

   con=Stdio.File();
   if (!con->connect(server,port))
      Error.connection("Failed to connect",con->errno());

   con->set_nonblocking(con_read,0,con_close);

   command_callback=_command_callback;
   notify_callback=_notify_callback;
   close_callback=_close_callback;
}

void con_close()
{
#ifdef IRC_DEBUG
   werror("-> (connection closed)\n");
#endif
   con->close();
   con=0;
   if (close_callback) close_callback();
}

string buf="";

void con_read(mixed dummy,string what)
{
#ifdef IRC_DEBUG
   werror("-> %O\n",what);
#endif
   buf+=what;
   array lines=(buf-"\r")/"\n";
   buf=lines[-1];
   foreach (lines[0..sizeof(lines)-2],string row)
   {
      mixed err=catch 
      {
	 handle_command(row);
      };
      if (err)
	 werror(master()->describe_backtrace(err));
   }
}

array write_buf=({});

void con_write_callback()
{
   while (sizeof(write_buf))
   {
      int j=con->write(write_buf[0]);
      if (j!=sizeof(write_buf[0]))
      {
	 if (j==-1) 
	 {
#ifdef IRC_DEBUG
	    werror("<- (write error)\n");
#endif
	    break; // connection error?
	 }
#ifdef IRC_DEBUG
	 werror("<- %O (buffer full)\n",write_buf[0][0..j-1]);
#endif
	 write_buf[0]=write_buf[0][j..];
      }
#ifdef IRC_DEBUG
      werror("<- %O (buffer finished)\n",write_buf[0]);
#endif
      write_buf=write_buf[1..];
   }
   con->set_nonblocking(con_read,0,con_close);
}

void con_write(string s)
{
   if (sizeof(write_buf))
   {
      write_buf+=({s});
      return;
   }
   if (!con)
   {
#ifdef IRC_DEBUG
	 werror("<- (write error; con lost)\n");
#endif
      return;
   }
   int j=con->write(s);
   if (j!=sizeof(s))
   {
      if (j==-1) 
      {
#ifdef IRC_DEBUG
	 werror("<- (write error; %O)\n",strerror(con->errno()));
#endif
	 return; // connection broken?
      }
#ifdef IRC_DEBUG
      werror("<- %O [%d] (buffer full)\n",s[0..j-1],j);
#endif
      write_buf=({s[j..]});
      con->set_nonblocking(con_read,con_write_callback,con_close);
   }
#ifdef IRC_DEBUG
   else
      werror("<- %O [%d]\n",s,j);
#endif
}

void transmit_noreply(string cmd,string args)
{
   con_write(cmd+" "+args+"\r\n");
}

void handle_command(string cmd)
{
   string a,b;
   if (cmd=="") return;
   if (cmd[0]==':') 
   {
      if (!notify_callback) return;
      if (sscanf(cmd,":%s :%s",a,b)==2)
	 notify_callback(@(a/" "),b);
      else
	 notify_callback(@(cmd[1..]/" "));
      return;
   }
   if (sscanf(cmd,"%s :%s",a,b)==2)
      command_callback(@(a/" "),b);
   else
      command_callback(@(cmd/" "));
}
