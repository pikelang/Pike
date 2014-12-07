/* nettle.h
 *
 * Shared declarations for the various files. */

#if 0
static void
werror(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
}
#else
#define werror(x)
#endif

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

/* In Nettle 2.0 the nettle_*_func typedefs lost their pointers. */
#ifdef HAVE_NETTLE_CRYPT_FUNC_IS_POINTER
/* Nettle 1.x */
typedef nettle_crypt_func		pike_nettle_crypt_func;
typedef nettle_hash_digest_func		pike_nettle_hash_digest_func;
typedef nettle_hash_update_func		pike_nettle_hash_update_func;
#else
/* Nettle 2.0 */

#ifdef dsa_params_init
/* Nettle 3.0 */
typedef nettle_cipher_func              *pike_nettle_crypt_func;
#else
typedef nettle_crypt_func		*pike_nettle_crypt_func;
#endif

typedef nettle_hash_digest_func		*pike_nettle_hash_digest_func;
typedef nettle_hash_update_func		*pike_nettle_hash_update_func;
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
