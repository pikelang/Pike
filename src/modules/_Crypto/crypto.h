/*
 * $Id: crypto.h,v 1.7 2001/04/30 00:33:41 lange Exp $
 *
 * Prototypes for some functions.
 *
 */

extern void assert_is_crypto_module(struct object *);

extern void pike_nt_init(void);
extern void pike_md2_init(void);
extern void pike_md2_exit(void);
extern void pike_md5_init(void);
extern void pike_md5_exit(void);
extern void pike_crypto_init(void);
extern void pike_crypto_exit(void);
extern void pike_idea_init(void);
extern void pike_idea_exit(void);
extern void pike_des_init(void);
extern void pike_des_exit(void);
extern void pike_cast_init(void);
extern void pike_cast_exit(void);
extern void pike_arcfour_init(void);
extern void pike_arcfour_exit(void);
extern void pike_invert_init(void);
extern void pike_invert_exit(void);
extern void pike_sha_init(void);
extern void pike_sha_exit(void);
extern void pike_cbc_init(void);
extern void pike_cbc_exit(void);
extern void pike_rijndael_init(void);
extern void pike_rijndael_exit(void);
extern void pike_rsa_init(void);
extern void pike_rsa_exit(void);
extern void pike_pipe_init(void);
extern void pike_pipe_exit(void);
extern char *crypt_md5(const char *pw, const char *salt);
