

// --------------- Standards.ASN1.Types private add-on --------------------
// This is very poor defined own ASN.1 objects (not enough time to clean it!)

//import .ldap_privates;

#define ASN1_BOOLEAN			.ldap_privates.asn1_boolean
#define ASN1_ENUMERATED			.ldap_privates.asn1_enumerated

#define ASN1_APPLICATION_SEQUENCE	.ldap_privates.asn1_application_sequence
#define ASN1_APPLICATION_OCTET_STRING	.ldap_privates.asn1_application_octet_string
#define ASN1_CONTEXT_SEQUENCE		.ldap_privates.asn1_context_sequence
#define ASN1_CONTEXT_BOOLEAN		.ldap_privates.asn1_context_boolean
#define ASN1_CONTEXT_INTEGER		.ldap_privates.asn1_context_integer
#define ASN1_CONTEXT_OCTET_STRING	.ldap_privates.asn1_context_octet_string
#define ASN1_CONTEXT_SET		.ldap_privates.asn1_context_set

// ------------- end of ASN.1 API hack -----------------------------


#define LDAP_DEFAULT_PORT       389
#define LDAPS_DEFAULT_PORT      636
#define LDAP_DEFAULT_HOST       "127.0.0.1"
#define LDAP_DEFAULT_URL       "ldap://"+LDAP_DEFAULT_HOST+":"+LDAP_DEFAULT_PORT+"/"
#define LDAP_DEFAULT_VERSION    3
#define V3_REFERRALS		1

#define ENABLE_PAGED_SEARCH

#ifdef DEBUG_PIKE_PROTOCOL_LDAP
#define DWRITE(X...)		werror("Protocols.LDAP: "+X)
#define DWRITE_HI(X...)		werror("Protocols.LDAP: "+X)
#else
#define DWRITE(X...)
#define DWRITE_HI(X...)
#endif

#ifdef DEBUG
#define DO_IF_DEBUG(X...) X
#else
#define DO_IF_DEBUG(X...)
#endif

// --- Enable run-time error ---
//#define THROW(X)
#define THROW(X)        throw(X)
#define ERROR(X...)     THROW (({sprintf (X), backtrace()}))
// --- Enable profiling ---
//#define DWRITE_PROF(X,Y)        werror(sprintf("Protocols.LDAP:Profile: "+X,Y))
#define DWRITE_PROF(X,Y)
