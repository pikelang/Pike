/*
 * Correct false positives from configure.
 *
 * Included from nettle_config.h to correct erroneous settings.
 * A typical cause of such is shadowing a more recent system nettle
 * with an older local version, where configure will find headerfiles
 * belonging to the system version if the header files aren't present
 * in the (intended) older version.
 *
 * Note: This must be in a separate file from nettle_config.h to
 *       avoid configure rewriting the #undef directives.
 */

#ifndef PIKE_NETTLE_CONFIG_FIXUP_H
#define PIKE_NETTLE_CONFIG_FIXUP_H

/* Camellia */
#ifndef HAVE_NETTLE_CAMELLIA128_CRYPT
#undef HAVE_NETTLE_CAMELLIA_H
#endif

/* GCM */
#ifndef HAVE_NETTLE_GCM_ENCRYPT
#undef HAVE_NETTLE_GCM_H
#endif

/* RIPEMD160 */
#ifndef HAVE_NETTLE_RIPEMD160_INIT
#undef HAVE_NETTLE_RIPEMD160_H
#endif

/* SHA3 */
#ifndef HAVE_NETTLE_SHA3_256_INIT
#undef HAVE_NETTLE_SHA3_H
#endif

/* GOST94 */
#ifndef HAVE_NETTLE_GOSTHASH94_INIT
#undef HAVE_NETTLE_GOSTHASH94_H
#endif

/* CHACHA */
#ifndef HAVE_NETTLE_CHACHA_CRYPT
#undef HAVE_NETTLE_CHACHA_H
#endif

/* CHACHA/POLY1305 */
#ifndef HAVE_NETTLE_CHACHA_POLY1305_ENCRYPT
#undef HAVE_NETTLE_CHACHA_POLY1305_H
#endif

/* POLY1305 */
#ifndef HAVE_NETTLE_POLY1305_DIGEST
#undef HAVE_NETTLE_POLY1305_H
#endif

/* EAX */
#ifndef HAVE_NETTLE_EAX_ENCRYPT
#undef HAVE_NETTLE_EAX_H
#endif

/* DSA */
#ifndef HAVE_NETTLE_DSA_SIGN
#undef HAVE_NETTLE_DSA_H
#endif

/* UMAC */
#ifndef HAVE_NETTLE_UMAC128_DIGEST
#undef HAVE_NETTLE_UMAC_H
#endif

/* ECDSA */
#ifndef HAVE_NETTLE_ECDSA_SIGN
#undef HAVE_NETTLE_ECDSA_H
#endif

#endif /* !PIKE_NETTLE_CONFIG_FIXUP_H */
