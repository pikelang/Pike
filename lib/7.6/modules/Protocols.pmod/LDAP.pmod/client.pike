// $Id: client.pike,v 1.2 2005/03/13 13:08:23 mast Exp $

#pike 7.7

inherit Protocols.LDAP.client;

int compare (string dn, array(string) aval)
{
  return ::compare(dn, aval[0], aval[1]);
}
