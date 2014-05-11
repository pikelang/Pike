/* nettle.h
 *
 * Shared declarations for the various files. */

struct program;
extern struct program *nettle_hash_program;
extern struct program *hash_instance_program;
extern struct program *nettle_hash_program;

#define NO_WIDE_STRING(s)	do {				\
    if ((s)->size_shift)					\
       Pike_error("Bad argument. Must be 8-bit string.\n");	\
  } while(0)


/* Hashing methods can normally process hundreds of megabytes per second
   so it's rather wasteful to enable threads during hashing of smaller
   data sizes. Limit is now 1 MB. */
#define HASH_THREADS_ALLOW_THRESHOLD (1024 * 1024)

/* Encrypt/decrypt methods are a bit more expensive. */
#define CIPHER_THREADS_ALLOW_THRESHOLD	1024

#ifdef HAVE_NETTLE_DSA_H
#include <nettle/dsa.h>
#endif
#ifdef dsa_params_init
/* We use the presence of the dsa_params_init remapping to detect Nettle
 * 3.0 or later. This is the recommended way to detect Nettle version
 * differences. In Nettle 3.0 length fields use size_t, where earlier
 * it was unsigned.
 */
typedef size_t		pike_nettle_size_t;
#else
typedef unsigned	pike_nettle_size_t;
#endif


char *pike_crypt_md5(int pl, const char *const pw,
                     int sl, const char *const salt,
                     int ml, const char *const magic);

void hash_init(void);

void hash_exit(void);

void cipher_init(void);

void cipher_exit(void);

void nt_init(void);

void nt_exit(void);

void hogweed_init(void);

void hogweed_exit(void);

void mac_init(void);

void mac_exit(void);

void aead_init(void);

void aead_exit(void);
