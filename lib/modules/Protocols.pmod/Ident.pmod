// An implementation of the IDENT protocol, specified in RFC 931.
//
// $Id: Ident.pmod,v 1.4 1998/05/27 05:47:41 neotron Exp $


int|array (string) lookup(object fd)
{
  mixed raddr; // Remote Address.
  mixed laddr; // Local Address.
  array err;
  object remote_fd;
  int i;
  if(!fd)
    return 0;
  err = catch(raddr = fd->query_address());
  if(err)
    throw(err + ({"Error in Protocols.Ident:"}));
  err = catch(laddr = fd->query_address(1));
  if(err)
    throw(err + ({"Error in Protocols.Ident:" }));
  if(!raddr || !laddr)
    throw(backtrace() +({ "Protocols.Ident - cannot lookup address"}));

  laddr = laddr / " ";
  raddr = raddr / " ";

  remote_fd = Stdio.FILE();
  if(!remote_fd->open_socket(0, laddr[0])) {
    destruct(remote_fd);
    throw(backtrace() +({ "Protocols.Ident: open_socket() failed."}));
  }

  if(err = catch(remote_fd->connect(raddr[0], 113)))
  {
    destruct(remote_fd);
    throw(err);
  }
  remote_fd->set_blocking();
  string query = raddr[1]+","+laddr[1]+"\r\n";
  int written;
  if((written = remote_fd->write(query)) != strlen(query)) {
    destruct(remote_fd);
    throw(backtrace() +({ "Protocols.Ident: short write ("+written+")."}));
  }
  mixed response = remote_fd->gets();//0xefffffff, 1);
  if(!response || !strlen(response))
  {
    destruct(remote_fd);
    throw(backtrace() +({ "Protocols.Ident: read failed."}));
  }
  remote_fd->close();
  destruct(remote_fd);
  response -= " ";
  response -= "\r";
  response /= ":";
  if(sizeof(response) < 2)
    return ({ "ERROR", "UNKNOWN-ERROR" });
  return response[1..];
}
