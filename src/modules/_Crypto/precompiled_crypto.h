/*
 * precompiled_crypto.h
 *
 * A pike module for getting access to some common cryptos.
 *
 * Henrik Grubbström 1996-10-15
 */

#ifndef PRECOMPILED_CRYPTO_H
#define PRECOMPILED_CRYPTO_H

#define MODULE_PREFIX "/precompiled/crypto/"
#define MOD_INIT(x) init_##x##_efuns
#define MOD_INIT2(x) init_##x##_programs
#define MOD_EXIT(x)  exit_##x

/*
 * Includes
 */
#if 0
#include <md2.h>
#include <md5.h>

#include <idea.h>

/*
 * Black magic
 */

#undef _

/*
 * Structures
 */

#if
struct pike_crypto {
  struct object *object;
  INT32 block_size;
  INT32 backlog_len;
  unsigned char *backlog;
};
#endif

struct pike_md2 {
  MD2_CTX ctx;
  unsigned char checksum[MD2_DIGEST_LENGTH];
  struct pike_string *string;
};

struct pike_md5 {
  MD5_CTX ctx;
  unsigned char checksum[MD5_DIGEST_LENGTH];
  struct pike_string *string;
};

#if 0
struct pike_idea {
  IDEA_KEY_SCHEDULE key;
};
#endif

#if 0
struct pike_des {
  des_key_schedule key_schedules[2];
  des_cblock ivs[2];
  int flags3;
  des_cblock checksum;
  unsigned char overflow[8];
  unsigned overflow_len;
};

struct pike_cbc {
  struct object *object;
  unsigned char *iv;
  INT32 block_size;
  INT32 mode;
};

struct pike_pipe {
  struct object **objects;
  INT32 num_objs;
  INT32 block_size;
  INT32 mode;
};
#endif

/*
 * Defines
 */

#define PIKE_CRYPTO	((struct pike_crypto *)(fp->current_storage))

#define PIKE_MD2	((struct pike_md2 *)(fp->current_storage))
#define PIKE_MD5	((struct pike_md5 *)(fp->current_storage))

#define PIKE_IDEA	((struct pike_idea *)(fp->current_storage))
#define PIKE_DES	((struct pike_des *)(fp->current_storage))

#define PIKE_CBC	((struct pike_cbc *)(fp->current_storage))
#define PIKE_PIPE	((struct pike_pipe *)(fp->current_storage))

#endif
/*
 * Globals
 */

/*
 * Prototypes
 */

void assert_is_crypto_module(struct object *);

#if 0
/*
 * Module linkage
 */

/* /precompiled/crypto/md2 */
void MOD_INIT2(md2)(void);
void MOD_INIT(md2)(void);
void MOD_EXIT(md2)(void);

/* /precompiled/crypto/md5 */
void MOD_INIT2(md5)(void);
void MOD_INIT(md5)(void);
void MOD_EXIT(md5)(void);
#endif

#if 1
/* /precompiled/crypto/md5 */
void MOD_INIT2(idea)(void);
void MOD_INIT(idea)(void);
void MOD_EXIT(idea)(void);

/* /precompiled/crypto/des */
void MOD_INIT2(des)(void);
void MOD_INIT(des)(void);
void MOD_EXIT(des)(void);

/* /precompiled/crypto/invert */
void MOD_INIT2(invert)(void);
void MOD_INIT(invert)(void);
void MOD_EXIT(invert)(void);

/* sha */
void MOD_INIT2(sha)(void);
void MOD_INIT(sha)(void);
void MOD_EXIT(sha)(void);

/* /precompiled/crypto/cbc */
void MOD_INIT2(cbc)(void);
void MOD_INIT(cbc)(void);
void MOD_EXIT(cbc)(void);

/* /precompiled/crypto/pipe */
void MOD_INIT2(pipe)(void);
void MOD_INIT(pipe)(void);
void MOD_EXIT(pipe)(void);
#endif

#endif /* PRECOMPILED_CRYPTO_H */
