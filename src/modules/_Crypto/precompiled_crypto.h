/*
 * precompiled_X.h
 *
 * A pike module for getting access to X11
 *
 * Henrik Grubbström 1996-10-15
 */

#ifndef PRECOMPILED_X_H
#define PRECOMPILED_X_H

/*
 * Includes
 */

#include <md2.h>
#include <md5.h>

#include <idea.h>
#include <des.h>

/*
 * Black magic
 */

#undef _

/*
 * Structures
 */

struct pike_crypto {
  struct object *object;
  INT32 block_size;
  INT32 backlog_len;
  unsigned char *backlog;
};

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

struct pike_idea {
  IDEA_KEY_SCHEDULE key;
};

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
};

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

/*
 * Globals
 */

/*
 * Prototypes
 */

void assert_is_crypto_module(struct object *);

/*
 * Module linkage
 */

/* /precompiled/crypto/md2 */
void init_md2_efuns(void);
void init_md2_programs(void);
void exit_md2(void);

/* /precompiled/crypto/md5 */
void init_md5_efuns(void);
void init_md5_programs(void);
void exit_md5(void);

/* /precompiled/crypto/md5 */
void init_idea_efuns(void);
void init_idea_programs(void);
void exit_idea(void);

/* /precompiled/crypto/des */
void init_des_efuns(void);
void init_des_programs(void);
void exit_des(void);

/* /precompiled/crypto/invert */
void init_invert_efuns(void);
void init_invert_programs(void);
void exit_invert(void);

/* /precompiled/crypto/cbc */
void init_cbc_efuns(void);
void init_cbc_programs(void);
void exit_cbc(void);

/* /precompiled/crypto/pipe */
void init_pipe_efuns(void);
void init_pipe_programs(void);
void exit_pipe(void);

#endif /* PRECOMPILED_X_H */
