// An implementation of the IDENT protocol, specified in RFC 931.
//
// $Id: Ident.pmod,v 1.3 1998/04/08 17:25:12 neotron Exp $


int|array (string) lookup(object fd)
{
  mixed raddr; // Remote Address.
  mixed laddr; // Local Address.
  array err;
  object remote_fd;
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

  remote_fd = Stdio.File();
  if(!remote_fd->open_socket(0, laddr[0])) {
    destruct(remote_fd);
    throw(backtrace() +({ "Protocols.Ident: open_socket() failed."}));
  }
  if(err = catch(remote_fd->connect(raddr[0], 113)))
  {
    destruct(remote_fd);
    throw(err);
  }
  if(remote_fd->write(raddr[1]+","+laddr[1]+"\r\n") == -1) {
    destruct(remote_fd);
    throw(backtrace() +({ "Protocols.Ident: write failed."}));
  }
  mixed response = remote_fd->read();
  if(!response || !strlen(response))
  {
    destruct(remote_fd);
    throw(backtrace() +({ "Protocols.Ident: read failed."}));
  }
  remote_fd->close();
  destruct(remote_fd);
  response -= " ";
  response -= "\r\n";
  response /= ":";
  if(sizeof(response) < 2)
    return ({ "ERROR", "UNKNOWN-ERROR" });
  return response[1..];
}
