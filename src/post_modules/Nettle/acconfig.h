/* Configuration settings for Nettle. */

#ifndef PIKE_NETTLE_CONFIG_H
#define PIKE_NETTLE_CONFIG_H

@TOP@

/* Define this if the nettle_crypt_func typedef is a pointer type. */
#undef HAVE_NETTLE_CRYPT_FUNC_IS_POINTER

/* Define this if generage_dsa_keypair takes the qbits argument */
#undef HAVE_DSA_QBITS_KEYPAIR_ARG

/* Define this if struct aes_ctx has the field key_size. */
#undef HAVE_NETTLE_STRUCT_AES_CTX_FIELD_KEY_SIZE

@BOTTOM@

#include "nettle_config_fixup.h"

#endif /* !PIKE_NETTLE_CONFIG_H */
