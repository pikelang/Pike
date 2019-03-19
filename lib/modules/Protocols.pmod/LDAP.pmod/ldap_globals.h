// -*- pike -*-

// --------------- Standards.ASN1.Types private add-on --------------------

#define ASN1_APPLICATION_SEQUENCE(T,A)	.ldap_privates.asn1_factory(Standards.ASN1.Types.Sequence, 1, T, A)
#define ASN1_APPLICATION_OCTET_STRING(T,A)	.ldap_privates.asn1_factory(Standards.ASN1.Types.OctetString, 1, T, A)
#define ASN1_CONTEXT_SEQUENCE(T,A)	.ldap_privates.asn1_factory(Standards.ASN1.Types.Sequence, 2, T, A)
#define ASN1_CONTEXT_BOOLEAN(T,A)	.ldap_privates.asn1_factory(Standards.ASN1.Types.Boolean, 2, T, A)
#define ASN1_CONTEXT_INTEGER(T,A)	.ldap_privates.asn1_factory(Standards.ASN1.Types.Integer, 2, T, A)
#define ASN1_CONTEXT_OCTET_STRING(T,A)	.ldap_privates.asn1_factory(Standards.ASN1.Types.OctetString, 2, T, A)
#define ASN1_CONTEXT_SET(T,A)		.ldap_privates.asn1_factory(Standards.ASN1.Types.Set, 2, T, A)

// ------------- end of ASN.1 API hack -----------------------------


#define LDAP_DEFAULT_PORT       389
#define LDAPS_DEFAULT_PORT      636
#define LDAP_DEFAULT_HOST       "127.0.0.1"
#define LDAP_DEFAULT_URL       "ldap://"+LDAP_DEFAULT_HOST+":"+LDAP_DEFAULT_PORT+"/"
#define LDAP_DEFAULT_VERSION    3
#define V3_REFERRALS		1

#define ENABLE_PAGED_SEARCH

#ifdef DEBUG_PIKE_PROTOCOL_LDAP
#define DWRITE(X...)		werror("Protocols.LDAP: " X)
#define DWRITE_HI(X...)		werror("Protocols.LDAP: " X)
#else
#define DWRITE(X...)
#define DWRITE_HI(X...)
#endif

#ifdef LDAP_DEBUG
#define DO_IF_DEBUG(X...) X
#else
#define DO_IF_DEBUG(X...)
#endif

// --- Enable run-time error ---
//#define THROW(X)
#define THROW(X)        throw(X)
#define ERROR(X...)     predef::error (X)
// --- Enable profiling ---
//#define DWRITE_PROF(X,Y)        werror("Protocols.LDAP:Profile: "+X,Y)
#define DWRITE_PROF(X,Y)
