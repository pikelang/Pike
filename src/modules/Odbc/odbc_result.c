/*
 * $Id: odbc_result.c,v 1.4 1997/08/15 20:20:11 grubba Exp $
 *
 * Pike  interface to ODBC compliant databases
 *
 * Henrik Grubbström
 */

/*
 * Includes
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_ODBC

#include "global.h"
RCSID("$Id: odbc_result.c,v 1.4 1997/08/15 20:20:11 grubba Exp $");

#include "interpret.h"
#include "object.h"
#include "threads.h"
#include "svalue.h"
#include "stralloc.h"
#include "mapping.h"
#include "array.h"
#include "multiset.h"
#include "program.h"
#include "array.h"
#include "builtin_functions.h"
#include "pike_memory.h"
#include "module_support.h"

#include "precompiled_odbc.h"

/*
 * Globals
 */

struct program *odbc_result_program = NULL;

/*
 * Functions
 */

/*
 * Help functions
 */

static void clean_sql_res(void)
{
  if (PIKE_ODBC_RES->field_info) {
    free(PIKE_ODBC_RES->field_info);
    PIKE_ODBC_RES->field_info = NULL;
  }
  if (PIKE_ODBC_RES->fields) {
    free_array(PIKE_ODBC_RES->fields);
    PIKE_ODBC_RES->fields = NULL;
  }
  if (PIKE_ODBC_RES->obj) {
    free_object(PIKE_ODBC_RES->obj);
    PIKE_ODBC_RES->obj = NULL;
    PIKE_ODBC_RES->odbc = NULL;
  }
  PIKE_ODBC_RES->hstmt = SQL_NULL_HSTMT;
}

static INLINE volatile void odbc_check_error(const char *fun, const char *msg,
					     RETCODE code, void (*clean)(void))
{
  if ((code != SQL_SUCCESS) && (code != SQL_SUCCESS_WITH_INFO)) {
    odbc_error(fun, msg, PIKE_ODBC_RES->odbc, PIKE_ODBC_RES->hstmt,
	       code, clean);
  }
}

/*
 * State maintenance
 */
 
static void init_res_struct(struct object *o)
{
  memset(PIKE_ODBC_RES, 0, sizeof(struct precompiled_odbc_result));
  PIKE_ODBC_RES->hstmt = SQL_NULL_HSTMT;
}
 
static void exit_res_struct(struct object *o)
{
  if (PIKE_ODBC_RES->hstmt != SQL_NULL_HSTMT) {
    HSTMT hstmt = PIKE_ODBC_RES->hstmt;
    PIKE_ODBC_RES->hstmt = SQL_NULL_HSTMT;
    odbc_check_error("exit_res_struct", "Freeing of HSTMT failed",
		     SQLFreeStmt(hstmt, SQL_DROP), clean_sql_res);
  }
  clean_sql_res();
}

/*
 * More help functions
 */

static void odbc_fix_fields(void)
{
  int i;
  struct odbc_field_info {
    int type;	/* Pike-type */
    int size;
  } *odbc_fields = alloca(sizeof(struct odbc_field_info)*PIKE_ODBC_RES->num_fields);
  size_t buf_size = 1024;
  char *buf = alloca(buf_size);
  int membuf_size = 0;
  char *membuf = NULL;

  if ((!buf)||(!odbc_fields)) {
    error("odbc_fix_fields(): Out of memory\n");
  }

  /*
   * First build the fields-array;
   */
  for (i=0; i < PIKE_ODBC_RES->num_fields; i++) {
    int nbits;
    SWORD name_len;
    SWORD sql_type;
    UDWORD precision;
    SWORD scale;
    SWORD nullable;

    while (1) {
      odbc_check_error("odbc_fix_fields", "Failed to fetch field info",
		       SQLDescribeCol(PIKE_ODBC_RES->hstmt, i+1,
				      buf, buf_size, &name_len,
				      &sql_type, &precision, &scale, &nullable),
		       0);
      if (name_len < (int)buf_size) {
	break;
      }
      do {
	buf_size *= 2;
      } while (name_len >= (int)buf_size);
      if (!(buf = alloca(buf_size))) {
	error("odbc_fix_fields(): Out of memory\n");
      }
    }
    odbc_fields[i].type = T_STRING;
    odbc_fields[i].size = precision+1;
    /* Create the mapping */
    push_text("name"); push_string(make_shared_binary_string(buf, name_len));
    push_text("type");
    switch(sql_type) {
    case SQL_CHAR:
      push_text("char");
      break;
    case SQL_DATE:
      push_text("date");
       odbc_fields[i].size = 7;
      break;
    case SQL_DECIMAL:
      push_text("decimal");
      odbc_fields[i].size += 2;
      break;
    case SQL_DOUBLE:
      push_text("double");
      /* odbc_fields[i].size = 8; */
      /* odbc_fields[i].type = T_FLOAT; */
      break;
    case SQL_INTEGER:
      push_text("integer");
      /* odbc_fields[i].size = 4; */
      /* odbc_fields[i].type = T_INT; */
      break;
    case SQL_LONGVARBINARY:
      push_text("long blob");
      break;
    case SQL_LONGVARCHAR:
      push_text("var string");
      break;
    case SQL_REAL:
      push_text("float");
      /* odbc_fields[i].size = 4; */
      /* odbc_fields[i].type = T_FLOAT; */
      break;
    case SQL_SMALLINT:
      push_text("short");
      /* odbc_fields[i].size = 4; */
      /* odbc_fields[i].type = T_INT; */
      break;
    case SQL_TIMESTAMP:
      push_text("time");
      odbc_fields[i].size = 22 + scale;
      break;
    case SQL_VARCHAR:
      push_text("var string");
      break;
    default:
      push_text("unknown");
      break;
    }
    push_text("length"); push_int(precision);
    push_text("decimals"); push_int(scale);
    push_text("flags");
    nbits = 0;
    if (nullable == SQL_NULLABLE) {
      nbits++;
      push_text("nullable");
    }
    if ((sql_type == SQL_LONGVARCHAR) ||
	(sql_type == SQL_LONGVARBINARY)) {
      nbits++;
      push_text("blob");
    }
    f_aggregate_multiset(nbits);

    f_aggregate_mapping(5*2);

    /* Align to longlong-word size */
    odbc_fields[i].size += 7;
    odbc_fields[i].size &= ~7;

    membuf_size += odbc_fields[i].size;
  }
  f_aggregate(PIKE_ODBC_RES->num_fields);

  sp[-1].u.array->refs++;
  PIKE_ODBC_RES->fields = sp[-1].u.array;
  pop_stack();

  PIKE_ODBC_RES->field_info = (struct field_info *)
    xalloc(sizeof(struct field_info) * PIKE_ODBC_RES->num_fields +
	   membuf_size);
  membuf = ((char *) PIKE_ODBC_RES->field_info) +
    sizeof(struct field_info) * PIKE_ODBC_RES->num_fields;

  /*
   * Now it's time to bind the columns
   */
  for (i=0; i < PIKE_ODBC_RES->num_fields; i++) {
    PIKE_ODBC_RES->field_info[i].type = odbc_fields[i].type;
    PIKE_ODBC_RES->field_info[i].buf = membuf;
    PIKE_ODBC_RES->field_info[i].size = odbc_fields[i].size;
    
    switch(odbc_fields[i].type) {
    case T_STRING:
      odbc_check_error("odbc_fix_fields", "Couldn't bind string field",
		       SQLBindCol(PIKE_ODBC_RES->hstmt, i+1,
				  SQL_C_CHAR, membuf, odbc_fields[i].size,
				  &PIKE_ODBC_RES->field_info[i].len), NULL);
      break;
    case T_INT:
      odbc_check_error("odbc_fix_fields", "Couldn't bind integer field",
		       SQLBindCol(PIKE_ODBC_RES->hstmt, i+1,
				  SQL_C_SLONG, membuf, odbc_fields[i].size,
				  &PIKE_ODBC_RES->field_info[i].len), NULL);
      break;
    case T_FLOAT:
      odbc_check_error("odbc_fix_fields", "Couldn't bind float field",
		       SQLBindCol(PIKE_ODBC_RES->hstmt, i+1,
				  SQL_C_DOUBLE, membuf, odbc_fields[i].size,
				  &PIKE_ODBC_RES->field_info[i].len), NULL);
      break;
    default:
      error("odbc_fix_fields(): Internal error: Unhandled type:%d\n",
	    odbc_fields[i].type);
      break;
    }
    membuf += odbc_fields[i].size;
  }
}

/*
 * Methods
 */

/* void create(object(odbc)) */
static void f_create(INT32 args)
{
  if (!args) {
    error("Too few arguments to odbc_result()\n");
  }
  if ((sp[-args].type != T_OBJECT) ||
      (!(PIKE_ODBC_RES->odbc =
	 (struct precompiled_odbc *)get_storage(sp[-args].u.object,
						odbc_program)))) {
    error("Bad argument 1 to odbc_result()\n");
  }
 
  PIKE_ODBC_RES->obj = sp[-args].u.object;
  PIKE_ODBC_RES->obj->refs++;
  PIKE_ODBC_RES->hstmt = PIKE_ODBC_RES->odbc->hstmt;
  PIKE_ODBC_RES->odbc->hstmt = SQL_NULL_HSTMT;
  
  pop_n_elems(args);
 
  if (PIKE_ODBC_RES->hstmt == SQL_NULL_HSTMT) {
    free_object(PIKE_ODBC_RES->obj);
    PIKE_ODBC_RES->odbc = NULL;
    PIKE_ODBC_RES->obj = NULL;
    error("odbc_result(): No result to clone\n");
  }
  PIKE_ODBC_RES->num_fields = PIKE_ODBC_RES->odbc->num_fields;
  PIKE_ODBC_RES->num_rows = PIKE_ODBC_RES->odbc->affected_rows;
  
  odbc_fix_fields();
}
 
/* int num_rows() */
static void f_num_rows(INT32 args)
{
  pop_n_elems(args);
  push_int(PIKE_ODBC_RES->num_rows);
}

/* int num_fields() */
static void f_num_fields(INT32 args)
{
  pop_n_elems(args);
  push_int(PIKE_ODBC_RES->num_fields);
}

/* array(int|mapping(string:mixed)) fetch_fields() */
static void f_fetch_fields(INT32 args)
{
  pop_n_elems(args);

  PIKE_ODBC_RES->fields->refs++;
  push_array(PIKE_ODBC_RES->fields);
}
 
/* int|array(string|float|int) fetch_row() */
static void f_fetch_row(INT32 args)
{
  int i;
  RETCODE code;
 
  pop_n_elems(args);

  code = SQLFetch(PIKE_ODBC_RES->hstmt);
  
  if (code == SQL_NO_DATA_FOUND) {
    /* No rows left in result */
    push_int(0);
  } else {
    odbc_check_error("odbc->fetch_row", "Couldn't fetch row",
		     code, NULL);
 
    for (i=0; i < PIKE_ODBC_RES->num_fields; i++) {
      if (PIKE_ODBC_RES->field_info[i].len != SQL_NULL_DATA) {
	switch (PIKE_ODBC_RES->field_info[i].type) {
	case T_INT:
	  push_int(*((int *)PIKE_ODBC_RES->field_info[i].buf));
	  break;
	case T_FLOAT:
	  push_float(*((double *)PIKE_ODBC_RES->field_info[i].buf));
	  break;
	case T_STRING:
	  push_string(make_shared_binary_string(PIKE_ODBC_RES->field_info[i].buf,
						PIKE_ODBC_RES->field_info[i].len));
	  break;
	default:
	  error("odbc->fetch_row(): Internal error: Unknown type (%d)\n",
		PIKE_ODBC_RES->field_info[i].type);
	  break;
	}
      } else {
	/* NULL */
	push_int(0);
      }
    }
    f_aggregate(PIKE_ODBC_RES->num_fields);
  }
}
 
/* int eof() */
static void f_eof(INT32 args)
{
  error("odbc->eof(): Not implemented yet!\n");
}

/* void seek() */
static void f_seek(INT32 args)
{
  error("odbc->seek(): Not implemented yet!\n");
}
 
/*
 * Module linkage
 */
 
void init_odbc_res_programs(void)
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
  add_storage(sizeof(struct precompiled_odbc_result));
 
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
 
  odbc_result_program = end_program();
  add_program_constant("odbc_result",odbc_result_program, 0);
}
 
void exit_odbc_res(void)
{
  if (odbc_result_program) {
    free_program(odbc_result_program);
    odbc_result_program = NULL;
  }
}

#endif /* HAVE_ODBC */

