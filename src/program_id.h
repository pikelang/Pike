/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: program_id.h,v 1.23 2004/11/26 16:42:38 grubba Exp $
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
#define     PROG_THREAD_LOCAL_ID                                      6
#define tObjImpl_THREAD_LOCAL                 "\003\000\000\000\000\006"
#define   tObjIs_THREAD_LOCAL                 "\003\001\000\000\000\006"
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
#define     PROG_BUFFER_ID					    014
#define     PROG_MULTI_STRING_REPLACE_ID			    015
#define     PROG_BACKEND_ID					    016
#define tObjImpl_BACKEND		      "\003\000\000\000\000\016"
#define   tObjIs_BACKEND		      "\003\001\000\000\000\016"
#define     PROG_MAPPING_ITERATOR_ID				    017
#define     PROG_ARRAY_ITERATOR_ID				    020
#define     PROG_MULTISET_ITERATOR_ID				    021
#define     PROG_STRING_ITERATOR_ID				    022
#define     PROG_FILE_LINE_ITERATOR_ID				    023
#define     PROG_STRING_SPLIT_ITERATOR_ID			    024
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

#define     PROG_THREAD_MUTEX_COMPAT_7_4_ID                          35
#define tObjImpl_THREAD_MUTEX_COMPAT_7_4      "\003\000\000\000\000\035"
#define   tObjIs_THREAD_MUTEX_COMPAT_7_4      "\003\001\000\000\000\035"

#define     PROG_GMP_MPZ_ID                                         040
#define tObjImpl_GMP_MPZ                      "\003\000\000\000\000\040"
#define   tObjIs_GMP_MPZ                      "\003\001\000\000\000\040"

/* Classes in builtin.cmod. */
#define     PROG_SIGNLE_STRING_REPLACE_ID			    050
#define     PROG_BOOTSTRING_ID					    051
#define     PROG_TIME_ID					    052
/* Kludge for #define TIME time in port.h */
#define     PROG_time_ID					    052
#define     PROG_TIMER_ID					    053
#define     PROG_AUTOMAP_MARKER_ID				    054
#define     PROG_LIST_ID					    055
#define tObjImpl_LIST			      "\003\000\000\000\000\055"
#define     PROG_LIST__GET_ITERATOR_ID				    056
#define tObjImpl_LIST__GET_ITERATOR_ID	      "\003\000\000\000\000\056"

/* Common modules. */
#define     PROG_MODULE_MIME_ID					   0100
#define     PROG_MODULE_FILES_ID				   0101

#define	    PROG_BASIC_TOKENIZER_ID				   0110
#define	    PROG_TOKEN_ID					   0111
#define tObjImpl_TOKEN			      "\003\000\000\000\000\111"

/* NOTE: 0144 == 100. */
/* 100 - 300 reserverd for Image.Image */


/* 100-119: Classes */
#define PROG_IMAGE_CLASS_START          100

#define PROG_IMAGE_IMAGE_ID             100
#define PROG_IMAGE_COLORTABLE_ID        101
#define PROG_IMAGE_LAYER_ID             102
#define PROG_IMAGE_FONT_ID              103

/* 120 - 159: Submodules */
#define PROG_IMAGE_SUBMODULE_START      120

/* 160 - : Submagic */
#define PROG_IMAGE_SUBMAGIC_START       160

/* 200 - 300: Submodule programs */
#define     PROG_IMAGE_COLOR_COLOR_ID                      0310 /* 200 */
#define tObjImpl_IMAGE_COLOR_COLOR_ID "\003\000\000\000\000\310"
#define   tObjIs_IMAGE_COLOR_COLOR_ID "\003\001\000\000\000\310"

/* 1000 - 2000 reserved for GTK. */

/* Start for dynamically allocated program ids. */
#define PROG_DYNAMIC_ID_START		0x10000

#endif
