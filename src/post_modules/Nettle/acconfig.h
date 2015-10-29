/* Configuration settings for Nettle. */

#ifndef PIKE_NETTLE_CONFIG_H
#define PIKE_NETTLE_CONFIG_H

@TOP@

/* Define this if your struct yarrow256_ctx has the field seed_file. */
#undef HAVE_STRUCT_YARROW256_CTX_SEED_FILE

/* Define this if the nettle_crypt_func typedef is a pointer type. */
#undef HAVE_NETTLE_CRYPT_FUNC_IS_POINTER

@BOTTOM@

#include "nettle_config_fixup.h"

#endif /* !PIKE_NETTLE_CONFIG_H */
