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
#define     PROG_MAPPING_ITERATOR_ID				    017
#define     PROG_ARRAY_ITERATOR_ID				    020
#define     PROG_MULTISET_ITERATOR_ID				    021
#define     PROG_STRING_ITERATOR_ID				    022
#define     PROG_FILE_LINE_ITERATOR_ID				    023
#define     PROG_STRING_SPLIT_ITERATOR_ID			    024
#define     PROG_GMP_MPZ_ID                                         025

/* 100 - 300 reserverd for Image.Image */


/* 100-119: Classes */
#define PROG_IMAGE_CLASS_START          100

#define PROG_IMAGE_IMAGE_ID             100
#define PROG_IMAGE_COLORTABLE_ID        101
#define PROG_IMAGE_LAYER_ID             102
#define PROG_IMAGE_FONT_ID              103
#define PROG_IMAGE_POLY_ID              104

/* 120 - 159: Submodules */
#define PROG_IMAGE_SUBMODULE_START      120

/* 160 - : Submagic */
#define PROG_IMAGE_SUBMAGIC_START       160

/* 200 - 300: Submodule programs */
#define PROG_IMAGE_COLOR_COLOR_ID       200

/* 1000 - 2000 reserved for GTK. */

#endif
