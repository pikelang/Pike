
class Connection {
  inherit .SMTP.Connection;
  array(string) commands = ({ "lhlo", "mail", "rcpt", "data",
			      "rset", "vrfy", "quit", "noop" });
}

//! A LMTP server. It has been fairly well tested against Postfix client.
//! For documentation, see the @[SMTP] server one since it works exactly
//! the same way. Actually this module is only an extention to the SMTP server.
class Server {
   private object fdport;
   private array(string) domains;
   private function cb_mailfrom;
   private function cb_rcptto;
   private function cb_data;
   
   private void accept_callback()
   {
     object fd = fdport->accept();
     if(!fd)
       error("Can't accept connections from socket\n");
     Connection(fd, domains, cb_mailfrom, cb_rcptto, cb_data);
     destruct(fd);
   }

   void create(array(string) _domains, void|int port, void|string ip,
	       function _cb_mailfrom, function _cb_rcptto, function _cb_data)
   {
     domains = _domains;
     cb_mailfrom = _cb_mailfrom;
     cb_rcptto = _cb_rcptto;
     cb_data = _cb_data;
     if(!port)
       port = 26;
     fdport = Stdio.Port(port, accept_callback, ip);
     if(!fdport)
     {
       error("Cannot bind to socket, already bound ?\n");
     }
   }
}
