/*
   Compatibility code for 7.2-

   $Id$

 */

#pike __REAL_VERSION__

inherit Protocols.LDAP.client;

int bind(string|void dn, string|void password) {

  return !::bind(dn, password);
}


object|int search (string|void filter, int|void attrsonly,
		   array(string)|void attrs) {

  return ::search(filter, attrs, attrsonly);
}


int modify (string dn, mapping(string:array(mixed)) attropval) {

  ::modify(dn, attropval);
  return ldap_errno;
}

int delete (string dn) {

  ::delete(dn);
  return ldap_errno;
}

int compare (string dn, array(string) aval) {

  ::compare(dn, aval);
  return ldap_errno;
}


int add (string dn, mapping(string:array(string)) attrs) {

  ::add(dn, attrs);
  return ldap_errno;
}


int modifydn (string dn, string newrdn, int deleteoldrdn,
              string|void newsuperior) {

  ::modifydn(dn, newrdn, deleteoldrdn, newsuperior);
  return ldap_errno;
}
