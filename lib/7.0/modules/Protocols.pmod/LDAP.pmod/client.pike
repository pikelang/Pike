/*
   Compatibility code for 7.2-

   $Id: client.pike,v 1.1 2002/07/12 13:35:56 hop Exp $

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
  return ::last_rv->error_number();
}

int delete (string dn) {

  ::delete(dn);
  return ::last_rv->error_number();
}

int compare (string dn, array(string) aval) {

  ::compare(dn, aval);
  return ::last_rv->error_number();
}


int add (string dn, mapping(string:array(string)) attrs) {

  ::add(dn, attrs);
  return ::last_rv->error_number();
}


int modifydn (string dn, string newrdn, int deleteoldrdn,
              string|void newsuperior) {

  ::modifydn(dn, newrdn, deleteoldrdn, newsuperior);
  return ::last_rv->error_number();
}
