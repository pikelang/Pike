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

char *pike_crypt_md5(int pl, const char *pw, int sl, const char *salt);

void hash_init(void);

void hash_exit(void);

void cipher_init(void);

void cipher_exit(void);

void nt_init(void);

void nt_exit(void);
