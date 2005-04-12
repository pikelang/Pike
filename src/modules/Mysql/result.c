/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: result.c,v 1.35 2005/04/12 00:38:04 nilsson Exp $
*/

/*
 * mysql query result
 *
 * Henrik Grubbström 1996-12-21
 */

/* Pike master headerfile */
#include "global.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_MYSQL
/*
 * Includes
 */
#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#elif defined(HAVE_WINSOCK_H)
#include <winsock.h>
#endif

/* From the mysql-dist */
/* Workaround for versions prior to 3.20.0 not beeing protected for
 * multiple inclusion.
 */
#ifndef _mysql_h
#ifdef HAVE_MYSQL_H
#include <mysql.h>
#else
#ifdef HAVE_MYSQL_MYSQL_H
#include <mysql/mysql.h>
#else
#ifndef DISABLE_BINARY
#error Need mysql.h header-file
#endif
#endif /* HAVE_MYSQL_MYSQL_H */
#endif /* HAVE_MYSQL_H */
#ifndef _mysql_h
#define _mysql_h
#endif
#endif


/* From the Pike-dist */
#include "svalue.h"
#include "mapping.h"
#include "object.h"
#include "program.h"
#include "stralloc.h"
#include "interpret.h"
#include "pike_error.h"
#include "builtin_functions.h"
#include "las.h"
#include "threads.h"
#include "multiset.h"
#include "bignum.h"
#include "module_support.h"

/* Local includes */
#include "precompiled_mysql.h"

/* System includes */
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif

#define sp Pike_sp

/* Define this to get support for field->default. NOT SUPPORTED */
#undef SUPPORT_DEFAULT

/* Define this to get the old fetch_fields() behaviour */
#undef OLD_SQL_COMPAT

/* Define this to get field_seek() and fetch_field() */
/* #define SUPPORT_FIELD_SEEK */

/*
 * Globals
 */

struct program *mysql_result_program = NULL;

/*
 * Functions
 */

/*
 * State maintenance
 */

static void init_res_struct(struct object *o)
{
  memset(PIKE_MYSQL_RES, 0, sizeof(struct precompiled_mysql_result));
}

static void exit_res_struct(struct object *o)
{
  if (PIKE_MYSQL_RES->result) {
    mysql_free_result(PIKE_MYSQL_RES->result);
    PIKE_MYSQL_RES->result = NULL;
  }
  if (PIKE_MYSQL_RES->connection) {
    free_object(PIKE_MYSQL_RES->connection);
    PIKE_MYSQL_RES->connection = NULL;
  }
}

/*
 * Help functions
 */

void mysqlmod_parse_field(MYSQL_FIELD *field, int support_default)
{
  if (field) {
    int nbits = 0;
    push_text("name"); push_text(field->name);
    push_text("table"); push_text(field->table);
    if (support_default) {
      push_text("default");
      if (field->def) {
	push_text(field->def);
      } else {
	push_int(0);
      }
    }
    push_text("type");
    switch(field->type) {
    case FIELD_TYPE_DECIMAL:
      push_text("decimal");
      break;
    case FIELD_TYPE_CHAR:
      push_text("char");
      break;
    case FIELD_TYPE_SHORT:
      push_text("short");
      break;
    case FIELD_TYPE_LONG:
      push_text("long");
      break;
    case FIELD_TYPE_FLOAT:
      push_text("float");
      break;
    case FIELD_TYPE_DOUBLE:
      push_text("double");
      break;
    case FIELD_TYPE_NULL:
      push_text("null");
      break;
    case FIELD_TYPE_TIME:
      push_text("time");
      break;
    case FIELD_TYPE_LONGLONG:
      push_text("longlong");
      break;
    case FIELD_TYPE_INT24:
      push_text("int24");
      break;
    case FIELD_TYPE_TINY_BLOB:
      push_text("tiny blob");
      break;
    case FIELD_TYPE_MEDIUM_BLOB:
      push_text("medium blob");
      break;
    case FIELD_TYPE_LONG_BLOB:
      push_text("long blob");
      break;
    case FIELD_TYPE_BLOB:
      push_text("blob");
      break;
    case FIELD_TYPE_VAR_STRING:
      push_text("var string");
      break;
    case FIELD_TYPE_STRING:
      push_text("string");
      break;
    case FIELD_TYPE_DATE:
      push_text("date");
      break;
    case FIELD_TYPE_DATETIME:
      push_text("datetime");
      break;
    case FIELD_TYPE_TIMESTAMP:
      push_text("timestamp");
      break;
    case FIELD_TYPE_YEAR:
      push_text("year");
      break;
    case FIELD_TYPE_NEWDATE:
      push_text("newdate");
      break;
    case FIELD_TYPE_ENUM:
      push_text("enum");
      break;
    case FIELD_TYPE_SET:
      push_text("set");
      break;
    default:
      push_text("unknown");
      break;
    }
    push_text("length"); push_int(field->length);
    push_text("max_length"); push_int(field->max_length);

    push_text("flags");
    if (IS_PRI_KEY(field->flags)) {
      nbits++;
      push_text("primary_key");
    }
    if (IS_NOT_NULL(field->flags)) {
      nbits++;
      push_text("not_null");
    }
    if (IS_BLOB(field->flags)) {
      nbits++;
      push_text("blob");
    }
    f_aggregate_multiset(nbits);

    push_text("decimals"); push_int(field->decimals);
      
    if (support_default) {
      f_aggregate_mapping(8*2);
    } else {
      f_aggregate_mapping(7*2);
    }
  } else {
    /*
     * Should this be an error?
     */
    push_int(0);
  }
}

/*! @module Mysql
 */

/*! @class mysql_result
 *!
 *! Objects of this class contain the result from Mysql queries.
 *!
 *! @seealso
 *!   @[Mysql.mysql], @[Mysql.mysql->big_query()]
 */

/*
 * Methods
 */

/*! @decl void create(Mysql.mysql connection)
 *!
 *! Make a new @[Mysql.mysql_result] object.
 *!
 *! @seealso
 *!   @[Mysql.mysql->big_query()], @[Mysql.mysql->list_dbs()],
 *!   @[Mysql.mysql->list_tables()], @[Mysql.mysql->list_processes()],
 *!   @[Mysql.mysql]
 */
static void f_create(INT32 args)
{
  if (!args) {
    Pike_error("Too few arguments to mysql_result()\n");
  }
  if (sp[-args].type != T_OBJECT) {
    Pike_error("Bad argument 1 to mysql_result()\n");
  }

  if (PIKE_MYSQL_RES->result) {
    mysql_free_result(PIKE_MYSQL_RES->result);
  }

  PIKE_MYSQL_RES->result = NULL;

  if (PIKE_MYSQL_RES->connection) {
    free_object(PIKE_MYSQL_RES->connection);
  }

  add_ref(PIKE_MYSQL_RES->connection = sp[-args].u.object);
  
  pop_n_elems(args);
}

/*! @decl int num_rows()
 *!
 *! Number of rows in the result.
 *!
 *! @seealso
 *!   @[num_fields()]
 */
static void f_num_rows(INT32 args)
{
  pop_n_elems(args);
  if (PIKE_MYSQL_RES->result) {
    push_int64(mysql_num_rows(PIKE_MYSQL_RES->result));
  } else {
    push_int(0);
  }
}

/*! @decl int num_fields()
 *!
 *! Number of fields in the result.
 *!
 *! @seealso
 *!   @[num_rows()]
 */
static void f_num_fields(INT32 args)
{
  pop_n_elems(args);
  if (PIKE_MYSQL_RES->result) {
    push_int(mysql_num_fields(PIKE_MYSQL_RES->result));
  } else {
    push_int(0);
  }
}

#ifdef SUPPORT_FIELD_SEEK

/*! @decl void field_seek(int field_no)
 *!
 *! Skip to specified field.
 *!
 *! Places the field cursor at the specified position. This affects
 *! which field mysql_result->fetch_field() will return next.
 *!
 *! Fields are numbered starting with 0.
 *!
 *! @note
 *!   This function is usually not enabled. To enable it
 *!   @tt{SUPPORT_FIELD_SEEK@} must be defined when compiling
 *!   the mysql-module.
 *!
 *! @seealso
 *!   @[fetch_field()], @[fetch_fields()]
 */
static void f_field_seek(INT32 args)
{
  if (!args) {
    Pike_error("Too few arguments to mysql->field_seek()\n");
  }
  if (sp[-args].type != T_INT) {
    Pike_error("Bad argument 1 to mysql->field_seek()\n");
  }
  if (!PIKE_MYSQL_RES->result) {
    Pike_error("Can't seek in uninitialized result object.\n");
  }
  mysql_field_seek(PIKE_MYSQL_RES->result, sp[-args].u.integer);
  pop_n_elems(args);
}

#endif /* SUPPORT_FIELD_SEEK */

/*! @decl int(0..1) eof()
 *!
 *! Sense end of result table.
 *!
 *! Returns @expr{1@} when all rows have been read, and @expr{0@} (zero)
 *! otherwise.
 *!
 *! @seealso
 *!   @[fetch_row()]
 */
static void f_eof(INT32 args)
{
  pop_n_elems(args);
  push_int(PIKE_MYSQL_RES->eof);
#if 0
  if (PIKE_MYSQL_RES->result) {
    push_int(mysql_eof(PIKE_MYSQL_RES->result));
  } else {
    push_int(0);
  }
#endif
}

#ifdef SUPPORT_FIELD_SEEK

/*! @decl int|mapping(string:mixed) fetch_field()
 *!
 *! Return specification of the current field.
 *!
 *! Returns a mapping with information about the current field, and
 *! advances the field cursor one step. Returns @expr{0@} (zero) if
 *! there are no more fields.
 *! 
 *! The mapping contains the same entries as those returned by
 *! @[Mysql.mysql->list_fields()], except that the entry @expr{"default"@}
 *! is missing.
 *!
 *! @note
 *!   This function is usually not enabled. To enable it
 *!   @tt{SUPPORT_FIELD_SEEK@} must be defined when compiling
 *!   the mysql-module.
 *!
 *! @seealso
 *!   @[fetch_fields()], @[field_seek()], @[Mysql.mysql->list_fields()]
 */
static void f_fetch_field(INT32 args)
{
  MYSQL_FIELD *field;
  MYSQL_RES *res = PIKE_MYSQL_RES->result;

  pop_n_elems(args);

  if (!res) {
    push_int(0);
    return;
  }

  THREADS_ALLOW();

  field = mysql_fetch_field(res);

  THREADS_DISALLOW();

  mysqlmod_parse_field(field, 0);
}

#endif /* SUPPORT_FIELD_SEEK */

/*! @decl array(int|mapping(string:mixed)) fetch_fields()
 *!
 *! Get specification of all remaining fields.
 *!
 *! Returns an array with one mapping for every remaining field in the
 *! result table.
 *! 
 *! The returned data is similar to the data returned by
 *! @[Mysql.mysql->list_fields()], except for that the entry
 *! @expr{"default"@} is missing.
 *!
 *! @note
 *!   Resets the field cursor to @expr{0@} (zero).
 *!
 *!   This function always exists even when @[fetch_field()] and
 *!   @[field_seek()] don't.
 *!
 *! @seealso
 *!   @[fetch_field()], @[field_seek()], @[Mysql.mysql->list_fields()]
 */
static void f_fetch_fields(INT32 args)
{
  MYSQL_FIELD *field;
  int i = 0;

  if (!PIKE_MYSQL_RES->result) {
    Pike_error("Can't fetch fields from uninitialized result object.\n");
  }

  pop_n_elems(args);

  while ((field = mysql_fetch_field(PIKE_MYSQL_RES->result))) {
    mysqlmod_parse_field(field, 0);
    i++;
  }
  f_aggregate(i);

  mysql_field_seek(PIKE_MYSQL_RES->result, 0);
}

/*! @decl void seek(int rows)
 *!
 *! Skip ahead @[rows] rows.
 *!
 *! @note
 *!   Can only seek forward.
 *!
 *! @seealso
 *!   @[fetch_row()]
 */
static void f_seek(INT32 args)
{
  INT_TYPE skip;
  get_all_args("seek",args,"%+",&skip);

  if (!PIKE_MYSQL_RES->result)
    Pike_error("Can't seek in uninitialized result object.\n");

  mysql_data_seek(PIKE_MYSQL_RES->result, skip);

  pop_n_elems(args);
}

/*! @decl int|array(string) fetch_row()
 *!
 *! Fetch the next row from the result.
 *!
 *! Returns an array with the contents of the next row in the result.
 *! Advances the row cursor to the next now.
 *!
 *! Returns @expr{0@} (zero) at the end of the table.
 *!
 *! @seealso
 *!   @[seek()]
 */
static void f_fetch_row(INT32 args)
{
  int num_fields;
  MYSQL_ROW row;
#ifdef HAVE_MYSQL_FETCH_LENGTHS
  FETCH_LENGTHS_TYPE *row_lengths;
#endif /* HAVE_MYSQL_FETCH_LENGTHS */

  if (!PIKE_MYSQL_RES->result) {
    Pike_error("Can't fetch data from an uninitialized result object.\n");
  }

  num_fields = mysql_num_fields(PIKE_MYSQL_RES->result);
  row = mysql_fetch_row(PIKE_MYSQL_RES->result);
#ifdef HAVE_MYSQL_FETCH_LENGTHS
  row_lengths = mysql_fetch_lengths(PIKE_MYSQL_RES->result);
#endif /* HAVE_MYSQL_FETCH_LENGTHS */

  pop_n_elems(args);

  mysql_field_seek(PIKE_MYSQL_RES->result, 0);

  if ((num_fields > 0) && row) {
    int i;

    for (i=0; i < num_fields; i++) {
      if (row[i]) {
	MYSQL_FIELD *field;

	if ((field = mysql_fetch_field(PIKE_MYSQL_RES->result))) {
	  switch (field->type) {
#ifdef OLD_SQL_COMPAT
	    /* Integer types */
	  case FIELD_TYPE_SHORT:
	  case FIELD_TYPE_LONG:
	  case FIELD_TYPE_INT24:
	  case FIELD_TYPE_DECIMAL:
#if 0
	    /* This one will not always fit in an INT32 */
          case FIELD_TYPE_LONGLONG:
#endif /* 0 */
	    push_int(atoi(row[i]));
	    break;
	    /* Floating point types */
	  case FIELD_TYPE_FLOAT:
	  case FIELD_TYPE_DOUBLE:
	    push_float(atof(row[i]));
	    break;
#endif /* OLD_SQL_COMPAT */
	  default:
#ifdef HAVE_MYSQL_FETCH_LENGTHS
	    push_string(make_shared_binary_string(row[i], row_lengths[i]));
#else
	    push_text(row[i]);
#endif /* HAVE_MYSQL_FETCH_LENGTHS */
	    break;
	  }
	} else {
	  /* Probably doesn't happen, but... */
#ifdef HAVE_MYSQL_FETCH_LENGTHS
	  push_string(make_shared_binary_string(row[i], row_lengths[i]));
#else
	  push_text(row[i]);
#endif /* HAVE_MYSQL_FETCH_LENGTHS */
	}
      } else {
	/* NULL? */
	push_int(0);
	if(i+1<num_fields)
	  mysql_field_seek(PIKE_MYSQL_RES->result, i+1);
      }
    }
    f_aggregate(num_fields);
  } else {
    /* No rows left in result */
    PIKE_MYSQL_RES->eof = 1;
    push_int(0);
  }

  mysql_field_seek(PIKE_MYSQL_RES->result, 0);
}

/*! @endclass
 */

/*! @endmodule
 */

/*
 * Module linkage
 */


void init_mysql_res_programs(void)
{
  start_new_program();
  ADD_STORAGE(struct precompiled_mysql_result);

  /* function(object:void) */
  ADD_FUNCTION("create", f_create,tFunc(tObj,tVoid), ID_PUBLIC);
  /* function(void:int) */
  ADD_FUNCTION("num_rows", f_num_rows,tFunc(tVoid,tInt), ID_PUBLIC);
  /* function(void:int) */
  ADD_FUNCTION("num_fields", f_num_fields,tFunc(tVoid,tInt), ID_PUBLIC);
#ifdef SUPPORT_FIELD_SEEK
  /* function(int:void) */
  ADD_FUNCTION("field_seek", f_field_seek,tFunc(tInt,tVoid), ID_PUBLIC);
#endif /* SUPPORT_FIELD_SEEK */
  /* function(void:int) */
  ADD_FUNCTION("eof", f_eof,tFunc(tVoid,tInt), ID_PUBLIC);
#ifdef SUPPORT_FIELD_SEEK
  /* function(void:int|mapping(string:mixed)) */
  ADD_FUNCTION("fetch_field", f_fetch_field,tFunc(tVoid,tOr(tInt,tMap(tStr,tMix))), ID_PUBLIC);
#endif /* SUPPORT_FIELD_SEEK */
  /* function(void:array(int|mapping(string:mixed))) */
  ADD_FUNCTION("fetch_fields", f_fetch_fields,tFunc(tVoid,tArr(tOr(tInt,tMap(tStr,tMix)))), ID_PUBLIC);
  /* function(int:void) */
  ADD_FUNCTION("seek", f_seek,tFunc(tInt,tVoid), ID_PUBLIC);
  /* function(void:int|array(string|int|float)) */
  ADD_FUNCTION("fetch_row", f_fetch_row,tFunc(tVoid,tOr(tInt,tArr(tOr3(tStr,tInt,tFlt)))), ID_PUBLIC);

  set_init_callback(init_res_struct);
  set_exit_callback(exit_res_struct);

  mysql_result_program = end_program();
  add_program_constant("mysql_result",mysql_result_program, 0);
}

void exit_mysql_res(void)
{
  if (mysql_result_program) {
    free_program(mysql_result_program);
    mysql_result_program = NULL;
  }
}

#else
static int place_holder;	/* Keep the compiler happy */
#endif /* HAVE_MYSQL */
