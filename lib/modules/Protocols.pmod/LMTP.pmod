//
// $Id: LMTP.pmod,v 1.9 2004/05/17 08:38:54 vida Exp $
//

#pike __REAL_VERSION__
class Configuration {
  inherit .SMTP.Configuration;
}

class Connection {
  inherit .SMTP.Connection;
  // The commands this module supports
  mapping(string:function) commands = ([
         "lhlo": ehlo,
	 "mail": mail, 
	 "rcpt": rcpt,
	 "data": data,
	 "rset": rset,
	 "vrfy": vrfy,
	 "quit": quit,
	 "noop": noop 
  ]);
  constant protocol = "LMTP";

  // if we are in LMTP mode we call cb_data for each recipient
  // and with one recipient. This way we have one mime message per
  // recipient and one outcode to display to the client per recipient
  // (that is LMTP specific)
  void message(string content) {
    if(sizeof(content) > cfg->maxsize)
    {
      outcode(552);
      return 0;
    }
    // LMTP as well as SMTP encode '.' by another '.' when it is the first
    // character of a line so we have to decode it 
    // We don't decode for SMTP since it can be usefull not to encode it for
    // sending again the mail 
    content = replace(replace(content, "\r\n", "\n"), "\n..", "\n.");
    MIME.Message message = low_message(content);
    if(!message) return;

    mixed err;
    foreach(mailto, string recipient)
    {
      int|array check;
      if(cfg->givedata)
	err = catch(check = cfg->cb_data(copy_value(message), mailfrom,
				    recipient, content));
      else
	err = catch(check = cfg->cb_data(copy_value(message), mailfrom, recipient));
      if(err)
      {
	outcode(554, err[0]);
	log(describe_backtrace(err));
	continue;
      }
      outcode(getretcode(check), geterrorstring(check));
    }
  }
}

//! A LMTP server. It has been fairly well tested against Postfix client.
//! Actually this module is only an extention to the @[SMTP] server.
class Server {
   static object fdport;
   Configuration config;

   static void accept_callback()
   {
     object fd = fdport->accept();
     if(!fd)
       error("Can't accept connections from socket\n");
     Connection(fd, config);
     destruct(fd);
   }

   //! @decl void create(array(string) _domains, void|int port,@
   //!        void|string ip, function _cb_mailfrom,@
   //!        function _cb_rcptto, function _cb_data)
   //!  Create a receiving LMTP server. It implements RFC 2821, 2822, 2033 and 
   //!  1854.
   //! 
   //! @param domain
   //!   Domains name this server relay, you need to provide at least one
   //!   domain (the first one will be used for MAILER-DAEMON address).
   //!   if you want to relay everything you can put a '*' after this
   //!   first domain.
   //! @param port
   //!   Port this server listen on
   //! @param listenip
   //!   IP on which server listen
   //! @param cb_mailfrom
   //!   Mailfrom callback function, this function will be called
   //!   when a client send a mail from command. This function must take a
   //!   string as argument (corresponding to the sender's email) and return
   //!   int corresponding to the SMTP code to output to the client. If you
   //!   return an array the first element is the SMTP code and the second
   //!   is the error string to display.
   //! @param cb_rcptto
   //!   Same as cb_mailfrom but called when a client sends a rcpt to.
   //! @param cb_data
   //!  This function is called for each recipient in the "rcpt to" command
   //!  after the client sends the "data" command
   //!  It must have the following synopsis:
   //!  int|array cb_data(object mime, string sender, string recipients,@
   //!  void|string rawdata)
   //!  object mime : the mime data object
   //!  string sender : sender of the mail (from the mailfrom command)
   //!  string recipient : one recipient given by one rcpt 
   //!     command.
   //! return : SMTP code to output to the client. If you return an array 
   //!   the first element is the SMTP code and the second is the error string
   //!   to display. Note that to comply with LMTP protocol you must output a
   //!   code each time this function is called.
   //! @example
   //!  Here is an example of silly program that does nothing except outputting
   //!  informations to stdout.
   //! int cb_mailfrom(string mail)
   //! {
   //!   return 250;
   //! }
   //!
   //! int cb_rcptto(string email)
   //! {
   //!   // check the user's mailbox here
   //!   return 250;
   //! }
   //! 
   //! int cb_data(object mime, string sender, string recipient)
   //! {
   //!   write(sprintf("smtpd: mailfrom=%s, to=%s, headers=%O\ndata=%s\n", 
   //!   sender, recipient, mime->headers, mime->getdata()));
   //!   // check the data and deliver the mail here
   //!   if(mime->body_parts)
   //!   {
   //!   {
   //!     foreach(mime->body_parts, object mpart)
   //!       write(sprintf("smtpd: mpart data = %O\n", mpart->getdata()));
   //!   }
   //!   return 250;
   //! }
   //! 
   //! int main(int argc, array(string) argv)
   //! {
   //!   Protocols.LMTP.Server(({ "ece.fr" }), 2500, "127.0.0.1", @
   //!      cb_mailfrom, cb_rcptto, cb_data);
   //!   return -1;
   //! }
   void create(array(string) _domains, void|int port, void|string ip, function _cb_mailfrom, function _cb_rcptto, function _cb_data)
   {
     config = Configuration(_domains, _cb_mailfrom, _cb_rcptto, _cb_data);
     random_seed(getpid() + time());
     if(!port)
       port = 25;
     fdport = Stdio.Port(port, accept_callback, ip);
     if(!fdport)
     {
       error("Cannot bind to socket, already bound ?\n");
     }
   }

}
