// Constants to make more human readable names for some known
// LDAP related object identifiers.

constant LDAP_CONTROL_MANAGE_DSA_IT = "2.16.840.1.113730.3.4.2";
//! Manage DSA IT LDAPv3 control (RFC 3296): Control to indicate that
//! the operation is intended to manage objects within the DSA
//! (server) Information Tree.

constant LDAP_CONTROL_VLVREQUEST = "2.16.840.1.113730.3.4.9";
//! LDAP Extensions for Scrolling View Browsing of Search Results
//! (internet draft): Control used to request virtual list view
//! support from the server.

constant LDAP_CONTROL_VLVRESPONSE = "2.16.840.1.113730.3.4.10";
//! LDAP Extensions for Scrolling View Browsing of Search Results
//! (internet draft): Control used to pass virtual list view (VLV)
//! data from the server to the client.

constant LDAP_PAGED_RESULT_OID_STRING = "1.2.840.113556.1.4.319";
//! Microsoft AD: Control to instruct the server to return the
//! results of a search request in smaller, more manageable packets
//! rather than in one large block.

constant LDAP_SERVER_ASQ_OID = "1.2.840.113556.1.4.1504";
//! Microsoft AD: Control to force the query to be based on a specific
//! DN-valued attribute.

constant LDAP_SERVER_CROSSDOM_MOVE_TARGET_OID = "1.2.840.113556.1.4.521";
//! Microsoft AD: Control used with an extended LDAP rename function
//! to move an LDAP object from one domain to another.

constant LDAP_SERVER_DIRSYNC_OID = "1.2.840.113556.1.4.841";
//! Microsoft AD: Control that enables an application to search the
//! directory for objects changed from a previous state.

constant LDAP_SERVER_DOMAIN_SCOPE_OID = "1.2.840.113556.1.4.1339";
//! Microsoft AD: Control used to instruct the LDAP server not to
//! generate any referrals when completing a request.

constant LDAP_SERVER_EXTENDED_DN_OID = "1.2.840.113556.1.4.529";
//! Microsoft AD: Control to request an extended form of an Active
//! Directory object distinguished name.

constant LDAP_SERVER_LAZY_COMMIT_OID = "1.2.840.113556.1.4.619";
//! Microsoft AD: Control used to instruct the server to return the
//! results of a DS modification command, such as add, delete, or
//! replace, after it has been completed in memory, but before it has
//! been committed to disk.

constant LDAP_SERVER_NOTIFICATION_OID = "1.2.840.113556.1.4.528";
//! Microsoft AD: Control used with an extended LDAP asynchronous
//! search function to register the client to be notified when changes
//! are made to an object in Active Directory.

constant LDAP_SERVER_PERMISSIVE_MODIFY_OID = "1.2.840.113556.1.4.1413";
//! Microsoft AD: An LDAP modify request will normally fail if it
//! attempts to add an attribute that already exists, or if it
//! attempts to delete an attribute that does not exist. With this
//! control, as long as the attribute to be added has the same value
//! as the existing attribute, then the modify will succeed. With this
//! control, deletion of an attribute that does not exist will also
//! succeed.

constant LDAP_SERVER_QUOTA_CONTROL_OID = "1.2.840.113556.1.4.1852";
//! Microsoft AD: Control used to pass the SID of a security
//! principal, whose quota is being queried, to the server in a LDAP
//! search operation.

constant LDAP_SERVER_RESP_SORT_OID = "1.2.840.113556.1.4.474";
//! Microsoft AD: Control used by the server to indicate the results
//! of a search function initiated using the @[LDAP_SERVER_SORT_OID]
//! control.

constant LDAP_SERVER_SD_FLAGS_OID = "1.2.840.113556.1.4.801";
//! Microsoft AD: Control used to pass flags to the server to control
//! various security descriptor results.

constant LDAP_SERVER_SEARCH_OPTIONS_OID = "1.2.840.113556.1.4.1340";
//! Microsoft AD: Control used to pass flags to the server to control
//! various search behaviors.

constant LDAP_SERVER_SHOW_DELETED_OID = "1.2.840.113556.1.4.417";
//! Microsoft AD: Control used to specify that the search results
//! include any deleted objects that match the search filter.

constant LDAP_SERVER_SORT_OID = "1.2.840.113556.1.4.473";
//! Microsoft AD: Control used to instruct the server to sort the
//! search results before returning them to the client application.

constant LDAP_SERVER_TREE_DELETE_OID = "1.2.840.113556.1.4.805";
//! Microsoft AD: Control used to delete an entire subtree in the
//! directory.

constant LDAP_SERVER_VERIFY_NAME_OID = "1.2.840.113556.1.4.1338";
//! Microsoft AD: Control used to instruct the DC accepting the update
//! which DC it should verify with, the existence of any DN attribute
//! values.
