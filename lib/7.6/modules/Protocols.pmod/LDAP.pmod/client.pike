// $Id

#pike 7.7

inherit Protocols.LDAP.client;

int compare (string dn, array(string) aval)
{
  return ::compare(dn, aval[0], aval[1]);
}
