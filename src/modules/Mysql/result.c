/*
 * $Id: result.c,v 1.5 1997/04/20 03:55:46 grubba Exp $
 *
 * mysql query result
 *
 * Henrik Grubbström 1996-12-21
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_MYSQL
/*
 * Includes
 */

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
#error Need mysql.h header-file
#endif /* HAVE_MYSQL_MYSQL_H */
#endif /* HAVE_MYSQL_H */
#ifndef _mysql_h
#define _mysql_h
#endif
#endif

/* dynamic_buffer.h contains a conflicting typedef for string
 * we don't use any dynamic buffers, so we have this work-around
 */
#define DYNAMIC_BUFFER_H
typedef struct dynamic_buffer_s dynamic_buffer;

/* From the Pike-dist */
#include <global.h>
#include <svalue.h>
#include <mapping.h>
#include <object.h>
#include <program.h>
#include <stralloc.h>
#include <interpret.h>
#include <error.h>
#include <builtin_functions.h>
#include <las.h>
#include <threads.h>

/* Local includes */
#include "precompiled_mysql.h"

/* System includes */
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif

/* Define this to get support for field->default. NOT SUPPORTED */
#undef SUPPORT_DEFAULT

/* Define this to get field_seek() and fetch_field() */
/* #define SUPPORT_FIELD_SEEK */

/*
 * Globals
 */

RCSID("$Id: result.c,v 1.5 1997/04/20 03:55:46 grubba Exp $");

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

/*
 * Methods
 */

/* void create(object(mysql)) */
static void f_create(INT32 args)
{
  struct precompiled_mysql *mysql;

  if (!args) {
    error("Too few arguments to mysql_result()\n");
  }
  if ((sp[-args].type != T_OBJECT) ||
      (!(mysql = (struct precompiled_mysql *)get_storage(sp[-args].u.object,
							 mysql_program)))) {
    error("Bad argument 1 to mysql_result()\n");
  }

  PIKE_MYSQL_RES->connection = sp[-args].u.object;
  PIKE_MYSQL_RES->connection->refs++;
  PIKE_MYSQL_RES->result = mysql->last_result;
  mysql->last_result = NULL;
  
  pop_n_elems(args);

  if (!PIKE_MYSQL_RES->result) {
    error("mysql_result(): No result to clone\n");
  }
}

/* int num_rows() */
static void f_num_rows(INT32 args)
{
  pop_n_elems(args);
  push_int(mysql_num_rows(PIKE_MYSQL_RES->result));
}

/* int num_fields() */
static void f_num_fields(INT32 args)
{
  pop_n_elems(args);
  push_int(mysql_num_fields(PIKE_MYSQL_RES->result));
}

#ifdef SUPPORT_FIELD_SEEK

/* void field_seek(int fieldno) */
static void f_field_seek(INT32 args)
{
  if (!args) {
    error("Too few arguments to mysql->field_seek()\n");
  }
  if (sp[-args].type != T_INT) {
    error("Bad argument 1 to mysql->field_seek()\n");
  }
  mysql_field_seek(PIKE_MYSQL_RES->result, sp[-args].u.integer);
  pop_n_elems(args);
}

#endif /* SUPPORT_FIELD_SEEK */

/* int eof() */
static void f_eof(INT32 args)
{
  pop_n_elems(args);
  push_int(mysql_eof(PIKE_MYSQL_RES->result));
}

#ifdef SUPPORT_FIELD_SEEK

/* int|mapping(string:mixed) fetch_field() */
static void f_fetch_field(INT32 args)
{
  MYSQL_FIELD *field;
  MYSQL_RES *res = PIKE_MYSQL_RES->result;

  pop_n_elems(args);

  THREADS_ALLOW();

  field = mysql_fetch_field(res);

  THREADS_DISALLOW();

  mysqlmod_parse_field(field, 0);
}

#endif /* SUPPORT_FIELD_SEEK */

/* array(int|mapping(string:mixed)) fetch_fields() */
static void f_fetch_fields(INT32 args)
{
  MYSQL_FIELD *field;
  int i = 0;
  
  pop_n_elems(args);

  while ((field = mysql_fetch_field(PIKE_MYSQL_RES->result))) {
    mysqlmod_parse_field(field, 0);
    i++;
  }
  f_aggregate(i);

  mysql_field_seek(PIKE_MYSQL_RES->result, 0);
}

/* void seek(int row) */
static void f_seek(INT32 args)
{
  if (!args) {
    error("Too few arguments to mysql_result->seek()\n");
  }
  if (sp[-args].type != T_INT) {
    error("Bad argument 1 to mysql_result->seek()\n");
  }
  if (sp[-args].u.integer < 0) {
    error("Negative argument 1 to mysql_result->seek()\n");
  }
  mysql_data_seek(PIKE_MYSQL_RES->result, sp[-args].u.integer);

  pop_n_elems(args);
}

/* int|array(string|float|int) fetch_row() */
static void f_fetch_row(INT32 args)
{
  int num_fields = mysql_num_fields(PIKE_MYSQL_RES->result);
  MYSQL_ROW row = mysql_fetch_row(PIKE_MYSQL_RES->result);
#ifdef HAVE_MYSQL_FETCH_LENGTHS
  int *row_lengths = mysql_fetch_lengths(PIKE_MYSQL_RES->result);
#endif /* HAVE_MYSQL_FETCH_LENGTHS */

  pop_n_elems(args);

  mysql_field_seek(PIKE_MYSQL_RES->result, 0);

  if ((num_fields > 0) && row) {
    int i;

    for (i=0; i < num_fields; i++) {
      if (row[i]) {
	MYSQL_FIELD *field;

	if (field = mysql_fetch_field(PIKE_MYSQL_RES->result)) {
	  switch (field->type) {
	    /* Integer types */
	  case FIELD_TYPE_SHORT:
	  case FIELD_TYPE_LONG:
	  case FIELD_TYPE_INT24:
#if 0
	    /* This one will not always fit in an INT32 */
          case FIELD_TYPE_LONGLONG:
#endif /* 0 */
	    push_int(atoi(row[i]));
	    break;
	    /* Floating point types */
	  case FIELD_TYPE_DECIMAL:	/* Is this a float or an int? */
	  case FIELD_TYPE_FLOAT:
	  case FIELD_TYPE_DOUBLE:
	    push_float(atof(row[i]));
	    break;
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
      }
    }
    f_aggregate(num_fields);
  } else {
    /* No rows left in result */
    push_int(0);
  }

  mysql_field_seek(PIKE_MYSQL_RES->result, 0);
}

/*
 * Module linkage
 */


void init_mysql_res_programs(void)
{
  /*
   * start_new_program();
   *
   * add_storage();
   *
   * add_function();
   * add_function();
   * ...
   *
   * set_init_callback();
   * set_exit_callback();
   *
   * program = end_c_program();
   * program->refs++;
   *
   */
 
  start_new_program();
  add_storage(sizeof(struct precompiled_mysql_result));

  add_function("create", f_create, "function(object:void)", ID_PUBLIC);
  add_function("num_rows", f_num_rows, "function(void:int)", ID_PUBLIC);
  add_function("num_fields", f_num_fields, "function(void:int)", ID_PUBLIC);
#ifdef SUPPORT_FIELD_SEEK
  add_function("field_seek", f_field_seek, "function(int:void)", ID_PUBLIC);
#endif /* SUPPORT_FIELD_SEEK */
  add_function("eof", f_eof, "function(void:int)", ID_PUBLIC);
#ifdef SUPPORT_FIELD_SEEK
  add_function("fetch_field", f_fetch_field, "function(void:int|mapping(string:mixed))", ID_PUBLIC);
#endif /* SUPPORT_FIELD_SEEK */
  add_function("fetch_fields", f_fetch_fields, "function(void:array(int|mapping(string:mixed)))", ID_PUBLIC);
  add_function("seek", f_seek, "function(int:void)", ID_PUBLIC);
  add_function("fetch_row", f_fetch_row, "function(void:int|array(string|int|float))", ID_PUBLIC);

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

#endif /* HAVE_MYSQL */
