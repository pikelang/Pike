/*
 * precompiled_crypto.h
 *
 * A pike module for getting access to some common cryptos.
 *
 * Henrik Grubbström 1996-10-15
 */

/* This file should be removed when things have stabilized */

#ifndef PRECOMPILED_CRYPTO_H
#define PRECOMPILED_CRYPTO_H

#define MODULE_PREFIX ""
#define MOD_INIT(x) PIKE_CONCAT3(pike_crypto_,x,_init)
#define MOD_EXIT(x) PIKE_CONCAT3(pike_crypto_,x,_exit)
#define MOD_INIT2(x) PIKE_CONCAT(ignore_,x)

#if 0
struct pike_md2 {
  MD2_CTX ctx;
  unsigned char checksum[MD2_DIGEST_LENGTH];
  struct pike_string *string;
};
#endif

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
#endif

#if 1

/* /precompiled/crypto/md5 */
void MOD_INIT2(md5)(void);
void MOD_INIT(md5)(void);
void MOD_EXIT(md5)(void);

/* /precompiled/crypto/idea */
void MOD_INIT2(idea)(void);
void MOD_INIT(idea)(void);
void MOD_EXIT(idea)(void);

/* /precompiled/crypto/cast */
void MOD_INIT2(cast)(void);
void MOD_INIT(cast)(void);
void MOD_EXIT(cast)(void);

/* /precompiled/crypto/des */
void MOD_INIT2(des)(void);
void MOD_INIT(des)(void);
void MOD_EXIT(des)(void);

/* /precompiled/crypto/rc4 */
void MOD_INIT2(rc4)(void);
void MOD_INIT(rc4)(void);
void MOD_EXIT(rc4)(void);

/* /precompiled/crypto/invert */
void MOD_INIT2(invert)(void);
void MOD_INIT(invert)(void);
void MOD_EXIT(invert)(void);

/* /precompiled/crypto/invert */
void MOD_INIT2(rc4)(void);
void MOD_INIT(rc4)(void);
void MOD_EXIT(rc4)(void);

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
