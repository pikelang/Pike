/*
 * $Id: result.c,v 1.1 1996/12/28 18:51:42 grubba Exp $
 *
 * mysql query result
 *
 * Henrik Grubbström 1996-12-21
 */

#ifdef HAVE_MYSQL
/*
 * Includes
 */

/* From the mysql-dist */
#ifndef MYSQL_MYSQL_H
#define MYSQL_MYSQL_H
#include <mysql.h>
#endif

/* dynamic_buffer.h contains a conflicting typedef for string
 * we don't use any dynamic buffers, so we have this work-around
 */
#define DYNAMIC_BUFFER_H
typedef struct dynamic_buffer_s dynamic_buffer;

/* From the Pike-dist */
#include <svalue.h>
#include <mapping.h>
#include <object.h>
#include <program.h>
#include <stralloc.h>
#include <interpret.h>
#include <error.h>
#include <builtin_functions.h>
#include <las.h>

/* Local includes */
#include "precompiled_mysql.h"

/* System includes */
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif

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
}

/*
 * Methods
 */

/* void create(object(mysql)) */
static void f_create(INT32 args)
{
  if (!args) {
    error("Too few arguments to mysql_result()\n");
  }
  if ((sp[-args].type != T_OBJECT) ||
      (sp[-args].u.object->prog != mysql_program)) {
    error("Bad argument 1 to mysql_result()\n");
  }

  PIKE_MYSQL_RES->result = ((struct precompiled_mysql *)sp[-args].u.object->storage)->last_result;
  ((struct precompiled_mysql *)sp[-args].u.object->storage)->last_result = NULL;
  
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

/* int eof() */
static void f_eof(INT32 args)
{
  pop_n_elems(args);
  push_int(mysql_eof(PIKE_MYSQL_RES->result));
}

/* array(int|mapping(string:mixed)) fetch_field() */
static void f_fetch_field(INT32 args)
{
  unsigned int i;

  pop_n_elems(args);

  for (i=0; i < mysql_num_fields(PIKE_MYSQL_RES->result); i++) {
    MYSQL_FIELD *field = mysql_fetch_field(PIKE_MYSQL_RES->result);

    if (field) {
      push_text("name"); push_text(field->name);
      push_text("table"); push_text(field->table);
      push_text("def");
      if (field->def) {
	push_text(field->def);
      } else {
	push_int(0);
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
      push_text("flags"); push_int(field->flags);		/*************/
      push_text("decimals"); push_int(field->decimals);
      
      f_aggregate_mapping(8*2);
    } else {
      /*
       * Should this be an error?
       */
      push_int(0);
    }
  }
  f_aggregate(mysql_num_fields(PIKE_MYSQL_RES->result));
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

/* void|array(string|int) fetch_row() */
static void f_fetch_row(INT32 args)
{
  /* I have no idea how this works.
   *
   * Stolen from Mysql.xs
   */
  MYSQL_ROW row = mysql_fetch_row(PIKE_MYSQL_RES->result);

  pop_n_elems(args);

  if (row) {
    int num_fields, i;

    /* Mysql.xs does a mysql_field_seek(result, 0) here,
     * but that seems to be a NOOP.
     */
    if ((num_fields = mysql_num_fields(PIKE_MYSQL_RES->result)) <= 0) {
      num_fields = 1;
    }
    for (i=0; i < num_fields; i++) {
      /* Mysql.xs does a mysql_fetch_field(result) here,
       * but throws away the result.
       */
      if (row[i]) {
	push_text(row[i]);
      } else {
	push_int(0);
      }
    }
    f_aggregate(num_fields);
  }
}

/*
 * Module linkage
 */

void init_mysql_res_efuns(void)
{
}

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

  add_function("create", f_create, "function(object:void)", OPT_SIDE_EFFECT);
  add_function("num_rows", f_num_rows, "function(void:int)", OPT_EXTERNAL_DEPEND);
  add_function("num_fields", f_num_fields, "function(void:int)", OPT_EXTERNAL_DEPEND);
  add_function("field_seek", f_field_seek, "function(int:void)", OPT_SIDE_EFFECT);
  add_function("eof", f_eof, "function(void:int)", OPT_EXTERNAL_DEPEND);
  add_function("fetch_field", f_fetch_field, "function(void:array(int|mapping(string:mixed)))", OPT_EXTERNAL_DEPEND);
  add_function("seek", f_seek, "function(int:void)", OPT_SIDE_EFFECT);
  add_function("fetch_row", f_fetch_row, "function(void:void|array(string|int))", OPT_EXTERNAL_DEPEND|OPT_SIDE_EFFECT);

  set_init_callback(init_res_struct);
  set_exit_callback(exit_res_struct);

  mysql_result_program = end_c_program("/precompiled/mysql_res");
  mysql_result_program->refs++;
}

void exit_mysql_res(void)
{
  if (mysql_result_program) {
    free_program(mysql_result_program);
    mysql_result_program = NULL;
  }
}

#endif /* HAVE_MYSQL */
