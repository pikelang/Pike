

// --------------- Standards.ASN1.Types private add-on --------------------
// This is very poor defined own ASN.1 objects (not enough time to clean it!)

//import .ldap_privates;

#define ASN1_BOOLEAN			.ldap_privates.asn1_boolean
#define ASN1_ENUMERATED			.ldap_privates.asn1_enumerated

#define ASN1_APPLICATION_SEQUENCE	.ldap_privates.asn1_application_sequence
#define ASN1_APPLICATION_OCTET_STRING	.ldap_privates.asn1_application_octet_string
#define ASN1_CONTEXT_SEQUENCE		.ldap_privates.asn1_context_sequence
#define ASN1_CONTEXT_INTEGER		.ldap_privates.asn1_context_integer
#define ASN1_CONTEXT_OCTET_STRING	.ldap_privates.asn1_context_octet_string
#define ASN1_CONTEXT_SET		.ldap_privates.asn1_context_set

// ------------- end of ASN.1 API hack -----------------------------


#define LDAP_DEFAULT_PORT       389
#define LDAP_DEFAULT_HOST       "127.0.0.1"
#define LDAP_DEFAULT_VERSION    2

#define UTF8_SUPPORT 0
#if UTF8_SUPPORT
#define LDAP_DEFAULT_CHARSET    "iso-8859-1"
#endif

// --- Debug low level operations ---
#define DWRITE(X)
//#define DWRITE(X)       werror("Protocols.LDAP: "+X)
// --- Debug high level operations ---
//#define DWRITE_HI(X)
#define DWRITE_HI(X)       werror("Protocols.LDAP: "+X)
// --- Enable run-time error ---
//#define THROW(X)
#define THROW(X)        throw(X)

