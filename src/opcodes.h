/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: opcodes.h,v 1.6 2000/05/11 14:09:46 grubba Exp $
 */
#ifndef OPCODES_H
#define OPCODES_H

/* Opcodes */

/*
 * These values are used by the stack machine, and can not be directly
 * called from Pike.
 */
#define	F_OFFSET	257
#define	F_PREFIX_256	258
#define	F_PREFIX_512	259
#define	F_PREFIX_768	260
#define	F_PREFIX_1024	261
#define	F_PREFIX_CHARX256	262
#define	F_PREFIX_WORDX256	263
#define	F_PREFIX_24BITX256	264
#define	F_PREFIX2_256	265
#define	F_PREFIX2_512	266
#define	F_PREFIX2_768	267
#define	F_PREFIX2_1024	268
#define	F_PREFIX2_CHARX256	269
#define	F_PREFIX2_WORDX256	270
#define	F_PREFIX2_24BITX256	271
#define	F_POP_VALUE	272
#define	F_POP_N_ELEMS	273
#define	F_MARK	274
#define	F_MARK2	275
#define	F_MARK_X	276
#define	F_POP_MARK	277
#define	F_CALL_LFUN	278
#define	F_CALL_LFUN_AND_POP	279
#define	F_CALL_LFUN_AND_RETURN	280
#define	F_APPLY	281
#define	F_APPLY_AND_POP	282
#define	F_MARK_APPLY	283
#define	F_MARK_APPLY_POP	284
#define	F_APPLY_AND_RETURN	285
#define	F_MARK_AND_STRING	286
#define	F_APPLY_ASSIGN_LOCAL	287
#define	F_APPLY_ASSIGN_LOCAL_AND_POP	288

#define	F_BRANCH	289
#define	F_BRANCH_WHEN_ZERO	290
#define	F_BRANCH_WHEN_NON_ZERO	291
#define	F_BRANCH_AND_POP_WHEN_ZERO	292
#define	F_BRANCH_AND_POP_WHEN_NON_ZERO	293
#define	F_BRANCH_WHEN_LT	294
#define	F_BRANCH_WHEN_GT	295
#define	F_BRANCH_WHEN_LE	296
#define	F_BRANCH_WHEN_GE	297
#define	F_BRANCH_WHEN_EQ	298
#define	F_BRANCH_WHEN_NE	299
#define	F_BRANCH_IF_LOCAL	300
#define	F_BRANCH_IF_NOT_LOCAL	301
#define	F_BRANCH_IF_NOT_LOCAL_ARROW	302
#define	F_INC_LOOP	303
#define	F_DEC_LOOP	304
#define	F_INC_NEQ_LOOP	305
#define	F_DEC_NEQ_LOOP	306
#define	F_RECUR	307
#define	F_TAIL_RECUR	308
#define	F_COND_RECUR	309

#define	F_LEXICAL_LOCAL	310
#define	F_LEXICAL_LOCAL_LVALUE	311

#define	F_INDEX	312
#define	F_ARROW	313
#define	F_INDIRECT	314
#define	F_STRING_INDEX	315
#define	F_LOCAL_INDEX	316
#define	F_LOCAL_LOCAL_INDEX	317
#define	F_LOCAL_ARROW	318
#define	F_GLOBAL_LOCAL_INDEX	319
#define	F_POS_INT_INDEX	320
#define	F_NEG_INT_INDEX	321
#define	F_MAGIC_INDEX	322
#define	F_MAGIC_SET_INDEX	323
#define	F_LTOSVAL	324
#define	F_LTOSVAL2	325
#define	F_PUSH_ARRAY	326
#define	F_RANGE	327
#define	F_COPY_VALUE	328

/*
 * Basic value pushing
 */
#define	F_LFUN	329
#define	F_TRAMPOLINE	330
#define	F_GLOBAL	331
#define	F_GLOBAL_LVALUE	332
#define	F_LOCAL	333
#define	F_2_LOCALS	334
#define	F_LOCAL_LVALUE	335
#define	F_MARK_AND_LOCAL	336
#define	F_EXTERNAL	337
#define	F_EXTERNAL_LVALUE	338
#define	F_CLEAR_LOCAL	339
#define	F_CLEAR_2_LOCAL	340
#define	F_CLEAR_4_LOCAL	341
#define	F_CLEAR_STRING_SUBTYPE	342
#define	F_CONSTANT	343
#define	F_FLOAT	344
#define	F_STRING	345
#define	F_ARROW_STRING	346
#define	F_NUMBER	347
#define	F_NEG_NUMBER	348
#define	F_CONST_1	349
#define	F_CONST0	350
#define	F_CONST1	351
#define	F_BIGNUM	352
#define	F_UNDEFINED	353

/*
 * These are the predefined functions that can be accessed from Pike.
 */
#define	F_INC	354
#define	F_DEC	355
#define	F_POST_INC	356
#define	F_POST_DEC	357
#define	F_INC_AND_POP	358
#define	F_DEC_AND_POP	359
#define	F_INC_LOCAL	360
#define	F_INC_LOCAL_AND_POP	361
#define	F_POST_INC_LOCAL	362
#define	F_DEC_LOCAL	363
#define	F_DEC_LOCAL_AND_POP	364
#define	F_POST_DEC_LOCAL	365
#define	F_RETURN	366
#define	F_DUMB_RETURN	367
#define	F_RETURN_0	368
#define	F_RETURN_1	369
#define	F_RETURN_LOCAL	370
#define	F_RETURN_IF_TRUE	371
#define	F_THROW_ZERO	372

#define	F_ASSIGN	373
#define	F_ASSIGN_AND_POP	374
#define	F_ASSIGN_LOCAL	375
#define	F_ASSIGN_LOCAL_AND_POP	376
#define	F_ASSIGN_GLOBAL	377
#define	F_ASSIGN_GLOBAL_AND_POP	378
#define	F_LOCAL_2_LOCAL	379
#define	F_LOCAL_2_GLOBAL	380
#define	F_GLOBAL_2_LOCAL	381
#define	F_ADD	382
#define	F_SUBTRACT	383
#define	F_ADD_INT	384
#define	F_ADD_NEG_INT	385
#define	F_ADD_TO_AND_POP	386
#define	F_ADD_FLOATS	387
#define	F_ADD_INTS	388
#define	F_MULTIPLY	389
#define	F_DIVIDE	390
#define	F_MOD	391

#define	F_LT	392
#define	F_GT	393
#define	F_EQ	394
#define	F_GE	395
#define	F_LE	396
#define	F_NE	397
#define	F_NEGATE	398
#define	F_NOT	399
#define	F_COMPL	400
#define	F_AND	401
#define	F_OR	402
#define	F_XOR	403
#define	F_LSH	404
#define	F_RSH	405
#define	F_LAND	406
#define	F_LOR	407
#define	F_EQ_OR	408
#define	F_EQ_AND	409

#define	F_SWITCH	410
#define	F_SSCANF	411
#define	F_CATCH	412
#define	F_CAST	413
#define	F_SOFT_CAST	414
#define	F_FOREACH	415

#define	F_SIZEOF	416
#define	F_SIZEOF_LOCAL	417
#define	F_CALL_FUNCTION	418
#define	F_CALL_FUNCTION_AND_RETURN	419

/*
 * These are token values that needn't have an associated code for the
 * compiled file
 */
#define	F_MAX_OPCODE	420

#define	F_ADD_EQ	422
#define	F_AND_EQ	423
#define	F_ARG_LIST	424
#define	F_COMMA_EXPR	425
#define	F_BREAK	427
#define	F_CASE	428
#define	F_CONTINUE	432
#define	F_DEFAULT	433
#define	F_DIV_EQ	434
#define	F_DO	435
#define	F_EFUN_CALL	439
#define	F_FOR	443
#define	F_IDENTIFIER	446
#define	F_LSH_EQ	458
#define	F_LVALUE_LIST	459
#define	F_ARRAY_LVALUE	460
#define	F_MOD_EQ	463
#define	F_MULT_EQ	464
#define	F_OR_EQ	467
#define	F_RSH_EQ	473
#define	F_SUB_EQ	478
#define	F_VAL_LVAL	480
#define	F_XOR_EQ	483
#define	F_NOP	484

#define	F_ALIGN	486
#define	F_POINTER	487
#define	F_LABEL	488
#define	F_DATA	489
#define	F_START_FUNCTION	490
#define	F_BYTE	491

#define	F_MAX_INSTR	492

/* Prototypes begin here */
void index_no_free(struct svalue *to,struct svalue *what,struct svalue *ind);
void o_index(void);
void o_cast(struct pike_string *type, INT32 run_time_type);
void f_cast(void);
void o_sscanf(INT32 args);
void f_sscanf(INT32 args);
/* Prototypes end here */

#endif
