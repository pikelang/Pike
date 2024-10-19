/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef PROGRAM_ID_H
#define PROGRAM_ID_H
/* This file contains reserved id numbers for pike programs.
 * This file might be automatically generated in the future.
 * /Hubbe
 */


#define     PROG_STDIO_FD_ID                                          1
#define tObjImpl_STDIO_FD                     "\003\000\000\000\000\001"
#define   tObjIs_STDIO_FD                     "\003\001\000\000\000\001"
#define     PROG_THREAD_ID_ID                                         2
#define tObjImpl_THREAD_ID                    "\003\000\000\000\000\002"
#define   tObjIs_THREAD_ID                    "\003\001\000\000\000\002"
#define     PROG_THREAD_MUTEX_KEY_ID                                  3
#define tObjImpl_THREAD_MUTEX_KEY             "\003\000\000\000\000\003"
#define   tObjIs_THREAD_MUTEX_KEY             "\003\001\000\000\000\003"
#define     PROG_THREAD_MUTEX_ID                                      4
#define tObjImpl_THREAD_MUTEX                 "\003\000\000\000\000\004"
#define   tObjIs_THREAD_MUTEX                 "\003\001\000\000\000\004"
#define     PROG_THREAD_CONDITION_ID                                  5
#define tObjImpl_THREAD_CONDITION             "\003\000\000\000\000\005"
#define   tObjIs_THREAD_CONDITION             "\003\001\000\000\000\005"
#define OLD_PROG_THREAD_LOCAL_ID                                      6
#define     PROG_THREAD_DISABLE_THREADS_ID                            7
#define tObjImpl_THREAD_DISABLE_THREADS       "\003\000\000\000\000\007"
#define   tObjIs_THREAD_DISABLE_THREADS       "\003\001\000\000\000\007"
#define     PROG_PARSER_HTML_ID                                     010
#define tObjImpl_PARSER_HTML                  "\003\000\000\000\000\010"
#define   tObjIs_PARSER_HTML                  "\003\001\000\000\000\010"

#define     PROG___BUILTIN_ID                                       011
#define tObjImpl___BUILTIN                    "\003\000\000\000\000\011"
#define   tObjIs___BUILTIN                    "\003\001\000\000\000\011"

#define     PROG_STDIO_STAT_ID                                      012
#define tObjImpl_STDIO_STAT                   "\003\000\000\000\000\012"
#define   tObjIs_STDIO_STAT                   "\003\001\000\000\000\012"

#define     PROG_BACKTRACE_FRAME_ID				    013
#define     PROG_MULTI_STRING_REPLACE_ID			    015
#define     PROG_BACKEND_ID					    016
#define tObjImpl_BACKEND		      "\003\000\000\000\000\016"
#define   tObjIs_BACKEND		      "\003\001\000\000\000\016"
#define     PROG_MAPPING_ITERATOR_ID				    017
#define     PROG_ARRAY_ITERATOR_ID				    020
#define     PROG_MULTISET_ITERATOR_ID				    021
#define     PROG_STRING_ITERATOR_ID				    022
#define     PROG_FILE_LINE_ITERATOR_ID				    023
#define OLD_PROG_STRING_SPLIT_ITERATOR_ID			    024
#define     PROG_ITERATOR_ID					    025
#define tObjImpl_ITERATOR		      "\003\000\000\000\000\025"
#define   tObjIs_ITERATOR		      "\003\001\000\000\000\025"

#define     PROG_STDIO_FD_REF_ID                                    026
#define tObjImpl_STDIO_FD_REF		      "\003\000\000\000\000\026"
#define   tObjIs_STDIO_FD_REF		      "\003\001\000\000\000\026"
#define     PROG_STDIO_FILE_LOCK_KEY_ID                             027
#define tObjImpl_STDIO_FILE_LOCK_KEY	      "\003\000\000\000\000\027"
#define   tObjIs_STDIO_FILE_LOCK_KEY	      "\003\001\000\000\000\027"
#define     PROG_STDIO_PORT_ID                                      030
#define tObjImpl_STDIO_PORT		      "\003\000\000\000\000\030"
#define   tObjIs_STDIO_PORT		      "\003\001\000\000\000\030"
#define     PROG_STDIO_SENDFILE_ID                                  031
#define tObjImpl_STDIO_SENDFILE		      "\003\000\000\000\000\031"
#define   tObjIs_STDIO_SENDFILE		      "\003\001\000\000\000\031"
#define     PROG_STDIO_UDP_ID                                       032
#define tObjImpl_STDIO_UDP		      "\003\000\000\000\000\032"
#define   tObjIs_STDIO_UDP		      "\003\001\000\000\000\032"
#define     PROG_STDIO_SOCK_ID                                      033
#define tObjImpl_STDIO_SOCK		      "\003\000\000\000\000\033"
#define   tObjIs_STDIO_SOCK		      "\003\001\000\000\000\033"
#define     PROG_STDIO_IPPROTO_ID                                   034
#define tObjImpl_STDIO_IPPROTO		      "\003\000\000\000\000\034"
#define   tObjIs_STDIO_IPPROTO		      "\003\001\000\000\000\034"

#define     PROG_THREAD_MUTEX_COMPAT_7_4_ID                         035
#define tObjImpl_THREAD_MUTEX_COMPAT_7_4      "\003\000\000\000\000\035"
#define   tObjIs_THREAD_MUTEX_COMPAT_7_4      "\003\001\000\000\000\035"
#define     PROG_PROCESS_ID					    036
#define tObjImpl_PROCESS                      "\003\000\000\000\000\036"
#define   tObjIs_PROCESS                      "\003\001\000\000\000\036"
#define RESERVED_FUNCTION_ITERATOR                                  037

#define     PROG_GMP_MPZ_ID                                         040
#define tObjImpl_GMP_MPZ                      "\003\000\000\000\000\040"
#define   tObjIs_GMP_MPZ                      "\003\001\000\000\000\040"
#define     PROG_GMP_BIGNUM_ID                                      041
#define tObjImpl_GMP_BIGNUM                   "\003\000\000\000\000\041"
#define   tObjIs_GMP_BIGNUM                   "\003\001\000\000\000\041"

/* Classes in builtin.cmod. */
#define     PROG_SINGLE_STRING_REPLACE_ID			    050
#define     PROG_BOOTSTRING_ID					    051
#define     PROG_TIME_ID					    052
/* Kludge for #define TIME time in port.h */
#define     PROG_time_ID					    052
#define     PROG_TIMER_ID					    053
#define     PROG_AUTOMAP_MARKER_ID				    054
#define     PROG_LIST_ID					    055
#define tObjImpl_LIST			      "\003\000\000\000\000\055"
#define     PROG_LIST__GET_ITERATOR_ID				    056
#define tObjImpl_LIST__GET_ITERATOR	      "\003\000\000\000\000\056"
#define     PROG_STRING_BUFFER_ID				    057
#define tObjImpl_STRING_BUFFER                "\003\000\000\000\000\057"

/* Classes in cpp.cmod. */
#define     PROG_STACK_ID					    064
#define tObjImpl_STACK			      "\003\000\000\000\000\064"
#define     PROG_DEFINE_ID					    065
#define tObjImpl_DEFINE			      "\003\000\000\000\000\065"
#define     PROG_CPP_ID						    066
#define tObjImpl_CPP			      "\003\000\000\000\000\066"

/* Classes in pike_compiler.cmod. */
#define     PROG_COMPILERENVIRONMENT_ID					    070
#define     PROG_COMPILERENVIRONMENT_PIKECOMPILER_ID			    071
#define tObjImpl_COMPILERENVIRONMENT_PIKECOMPILER     "\003\000\000\000\000\071"
#define     PROG_COMPILERENVIRONMENT_REPORTER_ID			    072
#define     PROG_COMPILERENVIRONMENT_PIKECOMPILER_COMPILERSTATE_ID	    073
#define     PROG_COMPILERENVIRONMENT_LOCK_ID				    074

/* Common modules. */
#define     PROG_MODULE_MIME_ID					   0100
#define     PROG_MODULE_STDIO_ID				   0101

#define	    PROG_BASIC_TOKENIZER_ID				   0110
#define	    PROG_TOKEN_ID					   0111
#define tObjImpl_TOKEN			      "\003\000\000\000\000\111"

/* Stdio.Buffer */
#define     PROG_STDIO_BUFFER_ID				   0120
#define tObjImpl_STDIO_BUFFER		      "\003\000\000\000\000\120"
#define   tObjIs_STDIO_BUFFER		      "\003\001\000\000\000\120"

/* NOTE: 0144 == 100. */
/* 100 - 299 reserverd for Image.Image */


/* 100-119: Classes */
#define PROG_IMAGE_CLASS_START          100

#define PROG_IMAGE_IMAGE_ID             100
#define tObjImpl_IMAGE_IMAGE "\003\000\000\000\000\144"
#define   tObjIs_IMAGE_IMAGE "\003\001\000\000\000\144"
#define PROG_IMAGE_COLORTABLE_ID        101
#define tObjImpl_IMAGE_COLORTABLE "\003\000\000\000\000\145"
#define   tObjIs_IMAGE_COLORTABLE "\003\001\000\000\000\145"
#define PROG_IMAGE_LAYER_ID             102
#define tObjImpl_IMAGE_LAYER "\003\000\000\000\000\146"
#define   tObjIs_IMAGE_LAYER "\003\001\000\000\000\146"
#define PROG_IMAGE_FONT_ID              103
#define tObjImpl_IMAGE_FONT "\003\000\000\000\000\147"
#define   tObjIs_IMAGE_FONT "\003\001\000\000\000\147"

/* 120 - 159: Submodules */
#define PROG_IMAGE_SUBMODULE_START      120

/* 160 - : Submagic */
#define PROG_IMAGE_SUBMAGIC_START       160

/* 200 - 299: Submodule programs */
#define     PROG_IMAGE_COLOR_COLOR_ID                      0310 /* 200 */
#define tObjImpl_IMAGE_COLOR_COLOR    "\003\000\000\000\000\310"
#define   tObjIs_IMAGE_COLOR_COLOR    "\003\001\000\000\000\310"

/* 300 - 399 reserved for Nettle */
#define     PROG_NETTLE_HASH_ID					0454 /* 300 */
#define tObjImpl_NETTLE_HASH		   "\003\000\000\000\001\054"
#define     PROG_NETTLE_HASH_STATE_ID				0455 /* 301 */
#define tObjImpl_NETTLE_HASH_STATE	   "\003\000\000\000\001\055"
#define     PROG_NETTLE_CIPHER_ID				0466 /* 310 */
#define tObjImpl_NETTLE_CIPHER		   "\003\000\000\000\001\066"
#define     PROG_NETTLE_CIPHER_STATE_ID				0467 /* 311 */
#define tObjImpl_NETTLE_CIPHER_STATE       "\003\000\000\000\001\067"
#define     PROG_NETTLE_BUFFEREDCIPHER_ID			0470 /* 312 */
#define tObjImpl_NETTLE_BUFFERCIPHER       "\003\000\000\000\001\070"
#define     PROG_NETTLE_BUFFEREDCIPHER_CQ__BUFFER_ID		0471 /* 313 */
#define tObjImpl_NETTLE_BUFFEREDCIPHER_CQ__BUFFER "\003\000\000\000\001\071"
#define     PROG_NETTLE_BUFFEREDCIPHER_CQ__BUFFER_STATE_ID	0472 /* 314 */
#define tObjImpl_NETTLE_BUFFEREDCIPHER_CQ__BUFFER_STATE "\003\000\000\000\001\072"
#define     PROG_NETTLE_BLOCKCIPHER_ID				0473 /* 315 */
#define tObjImpl_NETTLE_BLOCKCIPHER        "\003\000\000\000\001\073"
#define     PROG_NETTLE_BLOCKCIPHER_CQ__CBC_ID			0474 /* 316 */
#define tObjImpl_NETTLE_BLOCKCIPHER_CQ__CBC "\003\000\000\000\001\074"
#define     PROG_NETTLE_BLOCKCIPHER_CQ__PCBC_ID			0475 /* 317 */
#define tObjImpl_NETTLE_BLOCKCIPHER_CQ__PCBC "\003\000\000\000\001\075"
#define     PROG_NETTLE_MAC_ID					0500 /* 320 */
#define tObjImpl_NETTLE_MAC                "\003\000\000\000\001\100"
#define     PROG_NETTLE_MAC_STATE_ID				0501 /* 321 */
#define tObjImpl_NETTLE_MAC_STATE          "\003\000\000\000\001\101"
#define     PROG_NETTLE_AEAD_ID					0512 /* 330 */
#define tObjImpl_NETTLE_AEAD               "\003\000\000\000\001\112"
#define     PROG_NETTLE_AEAD_STATE_ID				0513 /* 331 */
#define tObjImpl_NETTLE_AEAD_STATE         "\003\000\000\000\001\113"

/* 1000 - 1999 reserved for GTK. */
/* 2000 - 2999 reserved for GTK2. */

/* Reserve 0x8000 and up for programs that use generics.
 *
 * This is needed in order to avoid having to load dynamic
 * modules from mk_type() to determine use of generics.
 */
#define PROG_GENERICS_ID_START		0x8000

#define     PROG_FUTURE_ID					0x8000
#define tObjImpl_FUTURE                    "\003\000\000\000\200\000"
#define tFutureValueType	tGeneric(tObjImpl_FUTURE, 0)
#define tFuture(X)		tBind(tAssign(tFutureValueType, X), tObjImpl_FUTURE)
#define     PROG_PROMISE_ID					0x8001
#define tObjImpl_PROMISE                   "\003\000\000\000\200\001"
#define tPromiseValueType	tGeneric(tObjImpl_PROMISE, 0)
#define tPromise(X)		tBind(tAssign(tPromiseValueType, X), tObjImpl_PROMISE)

#define     PROG_THREAD_LOCAL_ID                                0x8006
#define tObjImpl_THREAD_LOCAL              "\003\000\000\000\200\006"
#define   tObjIs_THREAD_LOCAL              "\003\001\000\000\200\006"
#define tThreadLocalValueType	tGeneric(tObjImpl_THREAD_LOCAL, 0)
#define tThreadLocal(X)		tBind(tAssign(tThreadLocalValueType, X), tObjImpl_THREAD_LOCAL)

#define     PROG_STRING_SPLIT_ITERATOR_ID			0x8014
#define tObjImpl_STRING_SPLIT_ITERATOR     "\003\000\000\000\200\024"
#define tStringSplitIterValueType	tGeneric(tObjImpl_STRING_SPLIT_ITERATOR, 0)
#define tStringSplitIter(X)		tBind(tAssign(tStringSplitIterValueType, X), tObjImpl_STRING_SPLIT_ITERATOR)

#define     PROG_FUNCTION_ITERATOR_ID				0x801f
#define tObjImpl_FUNCTION_ITERATOR         "\003\000\000\000\200\037"
#define tFunctionIterValueType	tGeneric(tObjImpl_FUNCTION_ITERATOR, 0)
#define tFunctionIter(X)		tBind(tAssign(tFunctionIterValueType, X), tObjImpl_FUNCTION_ITERATOR)

/* Start for dynamically allocated program ids. */
#define PROG_DYNAMIC_ID_START		0x10000

#endif
