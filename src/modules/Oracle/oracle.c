/*
 * $Id: oracle.c,v 1.62 2005/02/10 14:49:20 grubba Exp $
 *
 * Pike interface to Oracle databases.
 *
 * original design by Marcus Comstedt
 * re-written for Oracle 8.x by Fredrik Hubinette
 *
 */

/*
 * Includes
 */

#include "global.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "svalue.h"
#include "object.h"
#include "array.h"
#include "stralloc.h"
#include "interpret.h"
#include "pike_types.h"
#include "pike_memory.h"
#include "threads.h"
#include "module_support.h"
#include "mapping.h"
#include "multiset.h"
#include "builtin_functions.h"
#include "opcodes.h"
#include "pike_macros.h"
#include "version.h"

#if (PIKE_MAJOR_VERSION - 0 > 7) || (PIKE_MAJOR_VERSION - 0 == 7 && PIKE_MINOR_VERSION - 0 >= 1)
/* must be included last */
#include "module_magic.h"
#endif

#ifdef HAVE_ORACLE

/* VERY VERY UGLY */
#define MOTIF

#ifdef HAVE_OCI_H
#include <oci.h>
#else /* !HAVE_OCI_H */
#include <ocidfn.h>
#include <ociapr.h>
#endif /* HAVE_OCI_H */

#include <math.h>

RCSID("$Id: oracle.c,v 1.62 2005/02/10 14:49:20 grubba Exp $");


#define BLOB_FETCH_CHUNK 16384

/* #define ORACLE_DEBUG */
#define ORACLE_USE_THREADS
#define SERIALIZE_CONNECT

/*
 * This define crippes the Pike module to use static define buffers.
 * It may be required to work around bugs in some versions of the
 * oracle libraries - Hubbe
 */

/* For some reason this crashes if I make this larger than 8000
 * I suspect a static buffer somewhere in Oracle, but I have no proof.
 * -Hubbe
 */

/* #define STATIC_BUFFERS 8000 */

#ifndef ORACLE_USE_THREADS

#undef THREADS_ALLOW
#define THREADS_ALLOW()
#undef THREADS_DISALLOW
#define THREADS_DISALLOW()

#endif

#define BLOCKSIZE 2048

#define IS_SUCCESS(RES) ((RES == OCI_SUCCESS) || (RES == OCI_SUCCESS_WITH_INFO))

#if defined(SERIALIZE_CONNECT)
DEFINE_MUTEX(oracle_serialization_mutex);
#endif



#define MY_START_CLASS(STRUCT) \
  start_new_program(); \
  offset=ADD_STORAGE(struct STRUCT); \
  set_init_callback(PIKE_CONCAT3(init_,STRUCT,_struct));\
  set_exit_callback(PIKE_CONCAT3(exit_,STRUCT,_struct));

#define MY_END_CLASS(NAME) \
  PIKE_CONCAT(NAME,_program) = end_program(); \
  PIKE_CONCAT(NAME,_identifier) = add_program_constant(#NAME, PIKE_CONCAT(NAME,_program), 0);

#ifdef ORACLE_DEBUG
#define LOCK(X) do { \
   fprintf(stderr,"Locking  " #X " ...  from %s:%d\n",__FUNCTION__,__LINE__); \
   mt_lock( & (X) ); \
   fprintf(stderr,"Locking  " #X " done from %s:%d\n",__FUNCTION__,__LINE__); \
}while(0)

#define UNLOCK(X) do { \
   fprintf(stderr,"unlocking " #X "      from %s:%d\n",__FUNCTION__,__LINE__);\
   mt_unlock( & (X) ); \
}while(0)

#else
#define LOCK(X) mt_lock( & (X) );
#define UNLOCK(X) mt_unlock( & (X) );
#endif

#ifndef ADD_FUNCTION

#define ADD_FUNCTION add_function
#define ADD_STORAGE(X) add_storage(sizeof(X))
#define tNone ""
#define tInt "int"
#define tStr "string"
#define tFlt "float"
#define tObj "object"
#define tVoid "void"
#define tMix "mixed"

#define tMap(X,Y) "mapping(" X ":" Y ")"
#define tOr(X,Y) X "|" Y
#define tArr(X) "array(" X ")"
#define tFunc(X,Y) "function(" X ":" Y ")"
#define tFuncV(X,Z,Y) "function(" X Z "...:" Y ")"
#define tComma ","

#define string_builder dynamic_buffer_s
#define init_string_builder(X,Y) initialize_buf(X)
#define string_builder_allocate(X,Y,Z) low_make_buf_space(Y,X);
#define finish_string_builder(X) low_free_buf(X)
#define free_string_builder(X) toss_buffer(X)

#define STRING_BUILDER_STR(X) ((X).s.str)
#define STRING_BUILDER_LEN(X) ((X).s.len)
#include "dynamic_buffer.h"

#else
#define STRING_BUILDER_STR(X) ((X).s)
#define STRING_BUILDER_LEN(X) ((X).s->len)
#define tComma ""
#include "bignum.h"

#endif

#ifndef Pike_thread_id
#define Pike_thread_id thread_id
#endif

#ifndef Pike_sp
#define Pike_sp sp
#define Pike_fp fp
#endif

#ifndef CHECK_INTERPRETER_LOCK
#define CHECK_INTERPRETER_LOCK()
#endif


#ifndef CURRENT_STORAGE
#define CURRENT_STORAGE (Pike_fp->current_storage)
#endif

#ifdef DEBUG_MALLOC
#define THISOBJ dmalloc_touch(struct pike_frame *,Pike_fp)->current_object
#else
#define THISOBJ (Pike_fp->current_object)
#endif


#define PARENTOF(X) (X)->parent


/* This define only exists in Pike 7.1.x, if it isn't defined
 * we have to provide this function ourselves -Hubbe
 */
#if PIKE_MAJOR_VERSION - 0 < 7

void *parent_storage(int depth)
{
  struct inherit *inherit;
  struct program *p;
  struct object *o;
  INT32 i;
  if(!depth) return fp->current_storage;
  depth--;

  inherit=&Pike_fp->context;
  o=Pike_fp->current_object;
  
  if(!o)
    Pike_error("Current object is destructed\n");
  
  while(1)
  {
    if(inherit->parent_offset)
    {
      i=o->parent_identifier;
      o=o->parent;
      depth+=inherit->parent_offset-1;
    }else{
      i=inherit->parent_identifier;
      o=inherit->parent;
    }
    
    if(!o) return 0;
    if(!(p=o->prog)) return 0;
    
#ifdef DEBUG_MALLOC
    if (o->refs == 0x55555555) {
      fprintf(stderr, "The object %p has been zapped!\n", o);
      describe(p);
      fatal("Object zapping detected.\n");
    }
    if (p->refs == 0x55555555) {
      fprintf(stderr, "The program %p has been zapped!\n", p);
      describe(p);
      fprintf(stderr, "Which taken from the object %p\n", o);
      describe(o);
      fatal("Looks like the program %p has been zapped!\n", p);
    }
#endif /* DEBUG_MALLOC */
    
#ifdef PIKE_DEBUG
    if(i < 0 || i > p->num_identifier_references)
      fatal("Identifier out of range!\n");
#endif
    
    inherit=INHERIT_FROM_INT(p, i);
    
#ifdef DEBUG_MALLOC
    if (inherit->storage_offset == 0x55555555) {
      fprintf(stderr, "The inherit %p has been zapped!\n", inherit);
      debug_malloc_dump_references(inherit,0,2,0);
      fprintf(stderr, "It was extracted from the program %p %d\n", p, i);
      describe(p);
      fprintf(stderr, "Which was in turn taken from the object %p\n", o);
      describe(o);
      fatal("Looks like the program %p has been zapped!\n", p);
    }
#endif /* DEBUG_MALLOC */
    
    if(!depth) break;
    --depth;
  }

  return o->storage + inherit->storage_offset;
}
#endif

#ifdef PIKE_DEBUG
void *check_storage(void *storage, unsigned long magic, char *prog)
{
  if( storage && magic != *((unsigned long *)storage))
  {
    fprintf(stderr, "Wrong magic number! expected a %s\n",prog);
    fprintf(stderr, "Expected %lx, got %lx\n",magic,*((unsigned long *)storage));
    fatal("Wrong program, expected %s!\n",prog);
  }
  return storage;
}
#else
#define check_storage(X,Y,Z) (X)
#endif

#define STORAGE(O) ((O)->storage + (O)->prog->inherits[0].storage_offset)
#define THIS_DBCON ((struct dbcon *)check_storage(CURRENT_STORAGE,0xdbc04711UL,"dbcon"))
#define THIS_QUERY_DBCON ((struct dbcon *)check_storage(parent_storage(1),0xdbc04711UL,"dbcon"))
#define THIS_RESULT_DBCON ((struct dbcon *)check_storage(parent_storage(2),0xdbc04711UL,"dbcon"))
#define THIS_QUERY ((struct dbquery *)check_storage(CURRENT_STORAGE,0xdb994711UL,"dbquery"))
#define THIS_RESULT_QUERY ((struct dbquery *)check_storage(parent_storage(1),0xdb994711UL,"dbquery"))
#define THIS_RESULT ((struct dbresult *)check_storage(CURRENT_STORAGE,0xdbe04711UL,"dbresult"))
#define THIS_RESULTINFO ((struct dbresultinfo *)check_storage(CURRENT_STORAGE,0xdbe14711UL,"dbresultinfo"))
#define THIS_DBDATE ((struct dbdate *)check_storage(CURRENT_STORAGE,0xdbda4711UL,"dbdate"))
#define THIS_DBNULL ((struct dbnull *)check_storage(CURRENT_STORAGE,0xdb004711UL,"dbnull"))

static struct program *oracle_program = NULL;
static struct program *compile_query_program = NULL;
static struct program *big_query_program = NULL;
static struct program *dbresultinfo_program = NULL;
static struct program *Date_program = NULL;
static struct program *NULL_program = NULL;

static int oracle_identifier;
static int compile_query_identifier;
static int big_query_identifier;
static int dbresultinfo_identifier;
static int Date_identifier;

static struct object *nullstring_object;
static struct object *nullfloat_object;
static struct object *nullint_object;
static struct object *nulldate_object;

static OCIEnv *oracle_environment=0;

static OCIEnv *get_oracle_environment(void)
{
  sword rc;
  if(!oracle_environment)
  {
    rc=OCIEnvInit(&oracle_environment, OCI_DEFAULT, 0, 0);
    if(rc != OCI_SUCCESS)
      Pike_error("Failed to initialize oracle environment.\n");
  }
  return oracle_environment;
}

struct inout
{
  sb2 indicator;
  ub2 rcode;
  ub2 len; /* not really used? */
  short has_output;
  sword ftype;

  sb4 xlen;
  struct string_builder output;
  ub4 curlen;

  union dbunion
  {
    double f;
#ifdef INT64
    INT64 i;
#else
    INT32 i;
#endif
    char shortstr[32];
    OCIDate date;
    OCINumber num;
#ifdef STATIC_BUFFERS
    char str[STATIC_BUFFERS];
#endif
  } u;
  OCILobLocator *lob;
};

static void free_inout(struct inout *i);
static void init_inout(struct inout *i);

/****** connection ******/
struct dbcon
{
#ifdef PIKE_DEBUG
  unsigned long magic;
#endif
  OCIError *error_handle;
  OCISvcCtx *context;

  DEFINE_MUTEX(lock);

  int resultobject_busy;
  int timeout_limit;
};

static void init_dbcon_struct(struct object *o)
{
#ifdef ORACLE_DEBUG
  fprintf(stderr,"%s\n",__FUNCTION__);
#endif
#ifdef PIKE_DEBUG
  ((unsigned long *)(Pike_fp->current_storage))[0]=0xdbc04711UL;
#endif
  THIS_DBCON->error_handle=0;
  THIS_DBCON->context=0;
  THIS_DBCON->timeout_limit = 30; /* default value */
  THIS_DBCON->resultobject_busy = 0;
  mt_init( & THIS_DBCON->lock );
}

static void exit_dbcon_struct(struct object *o)
{
#ifdef ORACLE_DEBUG
  fprintf(stderr,"%s\n",__FUNCTION__);
#endif
  OCILogoff(THIS_DBCON->context, THIS_DBCON->error_handle);
  OCIHandleFree(THIS_DBCON->error_handle, OCI_HTYPE_ERROR);
  mt_destroy( & THIS_DBCON->lock );
}

/****** query ******/

struct dbquery
{
#ifdef PIKE_DEBUG
  unsigned long magic;
#endif
  OCIStmt *statement;
  INT_TYPE query_type;
  DEFINE_MUTEX(lock);

  INT_TYPE cols;
  struct array *field_info;
  struct mapping *output_variables;
};


void init_dbquery_struct(struct object *o)
{
#ifdef ORACLE_DEBUG
  fprintf(stderr,"%s\n",__FUNCTION__);
#endif
#ifdef PIKE_DEBUG
  ((unsigned long *)(Pike_fp->current_storage))[0]=0xdb994711UL;
#endif
  THIS_QUERY->cols=-2;
  THIS_QUERY->statement=0;
  mt_init(& THIS_QUERY->lock);
}

void exit_dbquery_struct(struct object *o)
{
#ifdef ORACLE_DEBUG
  fprintf(stderr,"%s\n",__FUNCTION__);
#endif
  OCIHandleFree(THIS_QUERY->statement, OCI_HTYPE_STMT);
  mt_destroy(& THIS_QUERY->lock);
}


/****** dbresult ******/

struct dbresult
{
#ifdef PIKE_DEBUG
  unsigned long magic;
#endif
  char dbcon_lock;
  char dbquery_lock;
};


static void init_dbresult_struct(struct object *o)
{
#ifdef ORACLE_DEBUG
  fprintf(stderr,"%s\n",__FUNCTION__);
#endif
#ifdef PIKE_DEBUG
  ((unsigned long *)(Pike_fp->current_storage))[0]=0xdbe04711UL;
#endif
  THIS_RESULT->dbcon_lock=0;
  THIS_RESULT->dbquery_lock=0;
  THIS_RESULT_DBCON->resultobject_busy = 1;
}

static void exit_dbresult_struct(struct object *o)
{
  struct dbquery *dbquery=THIS_RESULT_QUERY;
#ifdef ORACLE_DEBUG
  fprintf(stderr,"%s\n",__FUNCTION__);
#endif
  /* Variables are freed automatically */
  if(THIS_RESULT->dbquery_lock && dbquery)
  {
    struct dbcon *dbcon=THIS_RESULT_DBCON;
    dbcon->resultobject_busy = 0;
    UNLOCK( dbquery->lock );
    if(THIS_RESULT->dbcon_lock && dbcon)
    {
      UNLOCK( dbcon->lock );
    }
  }
#ifdef ORACLE_DEBUG
  else
  {
    fprintf(stderr,"exit_dbresult_struct %p %p\n",
	    PARENTOF(THISOBJ),
	    PARENTOF(THISOBJ)?PARENTOF(THISOBJ)->prog:0);
  }
#endif
}

/****** dbresultinfo ******/

struct dbresultinfo
{
#ifdef PIKE_DEBUG
  unsigned long magic;
#endif
  INT_TYPE length;
  INT_TYPE decimals;
  INT_TYPE real_type;
  struct pike_string *name;
  struct pike_string *type;

  OCIDefine *define_handle;

  struct inout data;
};


static void init_dbresultinfo_struct(struct object *o)
{
#ifdef ORACLE_DEBUG
  fprintf(stderr,"%s\n",__FUNCTION__);
#endif
#ifdef PIKE_DEBUG
  ((unsigned long *)(Pike_fp->current_storage))[0]=0xdbe14711UL;
#endif
  THIS_RESULTINFO->define_handle=0;
  init_inout(& THIS_RESULTINFO->data);
}

static void exit_dbresultinfo_struct(struct object *o)
{
#ifdef ORACLE_DEBUG
  fprintf(stderr,"%s\n",__FUNCTION__);
#endif
  if(THIS_RESULTINFO->define_handle) {
    debug_malloc_touch(THIS_RESULTINFO->define_handle);
    OCIHandleFree(THIS_RESULTINFO->define_handle, OCI_HTYPE_DEFINE);
  }
  free_inout( & THIS_RESULTINFO->data);
}

#ifdef ORACLE_DEBUG
static void gc_dbresultinfo_struct(struct object *o)
{
  THIS_RESULTINFO;
}
#endif

static void protect_dbresultinfo(INT32 args)
{
  Pike_error("You may not change variables in dbresultinfo objects.\n");
}

/****** dbdate ******/

struct dbdate
{
#ifdef PIKE_DEBUG
  unsigned long magic;
#endif
  OCIDate date;
};

static void init_dbdate_struct(struct object *o)
{
#ifdef PIKE_DEBUG
  ((unsigned long *)(Pike_fp->current_storage))[0]=0xdbda4711UL;
#endif
}
static void exit_dbdate_struct(struct object *o) {}

/****** dbnull ******/

struct dbnull
{
#ifdef PIKE_DEBUG
  unsigned long magic;
#endif
  struct svalue type;
};

static void init_dbnull_struct(struct object *o)
{
#ifdef PIKE_DEBUG
  ((unsigned long *)(Pike_fp->current_storage))[0]=0xdb004711UL;
#endif
}
static void exit_dbnull_struct(struct object *o) {}

/************/

static void ora_error_handler(OCIError *err, sword rc, char *func)
{
  /* FIXME: we might need to do switch(rc) */
  static text msgbuf[512];
  ub4 errcode;

#ifdef ORACLE_DEBUG
  fprintf(stderr,"%s\n",__FUNCTION__);
#endif

  OCIErrorGet(err,1,0,&errcode,msgbuf,sizeof(msgbuf),OCI_HTYPE_ERROR);
  if(func)
    Pike_error("%s:code=%d:%s",func,rc,msgbuf);
  else
    Pike_error("Oracle:code=%d:%s",rc,msgbuf);
}


static OCIError *global_error_handle=0;;

OCIError *get_global_error_handle(void)
{
  if (!global_error_handle) {
    sword rc;
    rc=OCIHandleAlloc(get_oracle_environment(),
		      (void **)& global_error_handle,
		      OCI_HTYPE_ERROR,
		      0,
		      0);

    if(rc != OCI_SUCCESS)
      Pike_error("Failed to allocate error handle.\n");
  }
  
  return global_error_handle;
}



static void f_num_fields(INT32 args)
{
  struct dbresult *dbresult = THIS_RESULT;
  struct dbquery *dbquery = THIS_RESULT_QUERY;
  struct dbcon *dbcon = THIS_RESULT_DBCON;

#ifdef ORACLE_DEBUG
  fprintf(stderr,"%s\n",__FUNCTION__);
#endif

  if(dbquery->cols == -2)
  {
    sword rc;
    ub4 columns;

    THREADS_ALLOW();

/*    LOCK(dbcon->lock);  */

    rc=OCIAttrGet(dbquery->statement,
		  OCI_HTYPE_STMT,
		  &columns,
		  0,
		  OCI_ATTR_PARAM_COUNT,
		  dbcon->error_handle); /* <- FIXME */


    THREADS_DISALLOW();
/*    UNLOCK(dbcon->lock); */

    if(rc != OCI_SUCCESS)
      ora_error_handler(dbcon->error_handle, rc,"OCIAttrGet");

    dbquery->cols = columns; /* -1 ? */
  }
  pop_n_elems(args);
  push_int(dbquery->cols);
}

static sb4 output_callback(struct inout *inout,
			   ub4 index,
			   void **bufpp,
			   ub4 **alenpp,
			   ub1 *piecep,
			   dvoid **indpp,
			   ub2 **rcodepp)
{
#ifdef ORACLE_DEBUG
  fprintf(stderr,"%s(%d)\n",__FUNCTION__,inout->ftype);
#endif
  inout->has_output=1;
  *indpp = (dvoid *) &inout->indicator;
  *rcodepp=&inout->rcode;
  *alenpp=&inout->xlen;

  switch(inout->ftype)
  {
    default:
#ifdef ORACLE_DEBUG
      fprintf(stderr,"Unhandled data type in %s: %d\n",__FUNCTION__,inout->ftype);
#endif

    case SQLT_CHR:
    case SQLT_STR:
    case SQLT_LBI:
    case SQLT_LNG:
      if(!STRING_BUILDER_STR(inout->output))
      {
	init_string_builder(& inout->output,0);
      }else{
	STRING_BUILDER_LEN(inout->output)+=inout->xlen-BLOCKSIZE;
	inout->xlen=0;
      }
      
      inout->xlen=BLOCKSIZE;
      *bufpp=string_builder_allocate(& inout->output, inout->xlen, 0);
      *piecep = OCI_NEXT_PIECE;
#ifdef ORACLE_DEBUG
      MEMSET(*bufpp, '#', inout->xlen);
      ((char *)*bufpp)[inout->xlen-1]=0;
#endif
      return OCI_CONTINUE;

  case SQLT_CLOB:
  case SQLT_BLOB:
    *bufpp=inout->lob;
    inout->xlen=sizeof(inout->lob); /* ? */
    *piecep = OCI_ONE_PIECE;
    return OCI_CONTINUE;

    case SQLT_FLT:
      *bufpp=&inout->u.f;
      inout->xlen=sizeof(inout->u.f);
      *piecep = OCI_ONE_PIECE;
      return OCI_CONTINUE;

    case SQLT_INT:
      *bufpp=&inout->u.i;
      inout->xlen=sizeof(inout->u.i);
      *piecep = OCI_ONE_PIECE;
      return OCI_CONTINUE;

    case SQLT_NUM:
      *bufpp=&inout->u.num;
      inout->xlen=sizeof(inout->u.num);
      *piecep = OCI_ONE_PIECE;
      return OCI_CONTINUE;

    case SQLT_ODT:
      *bufpp=&inout->u.date;
      inout->xlen=sizeof(inout->u.date);
      *piecep = OCI_ONE_PIECE;
      return OCI_CONTINUE;

      return 0;
  }

}
			   

static sb4 define_callback(void *dbresultinfo,
			   OCIDefine *def,
			   ub4 iter,
			   void **bufpp,
			   ub4 **alenpp,
			   ub1 *piecep,
			   dvoid **indpp,
			   ub2 **rcodep)
{
#ifdef ORACLE_DEBUG
  fprintf(stderr,"%s ..",__FUNCTION__);
#endif
  return
    output_callback( &((struct dbresultinfo *)dbresultinfo)->data,
		     iter,
		     bufpp,
		     alenpp,
		     piecep,
		     indpp,
		     rcodep);
}


static void f_fetch_fields(INT32 args)
{
  struct dbresult *dbresult = THIS_RESULT;
  struct dbquery *dbquery=THIS_RESULT_QUERY;
  struct dbcon *dbcon=THIS_RESULT_DBCON;
  INT32 i;
  sword rc;

#ifdef ORACLE_DEBUG
  fprintf(stderr,"%s\n",__FUNCTION__);
#endif

  pop_n_elems(args);

  if(!dbquery->field_info)
  {
    /* Get the number of rows */
    if(dbquery->cols == -2)
    {
      f_num_fields(0);
      pop_stack();
    }

    check_stack(dbquery->cols);
    
    for(i=0; i<dbquery->cols; i++)
    {
      char *errfunc=0;
      OCIParam *column_parameter;
      ub2 type;
      ub2 size;
      sb1 scale;
      char *name;
      ub4 namelen;
      struct object *o;
      struct dbresultinfo *info;
      char *type_name;
      int data_size;
      
/*      pike_gdb_breakpoint(); */
      THREADS_ALLOW();
/*      LOCK(dbcon->lock); */

      do {
	rc=OCIParamGet(dbquery->statement,OCI_HTYPE_STMT,
		       dbcon->error_handle,
		       (void **)&column_parameter,
		       i+1);
	
	if(!IS_SUCCESS(rc)) { errfunc="OciParamGet"; break; }
	
	rc=OCIAttrGet((void *)column_parameter,
		      OCI_DTYPE_PARAM,
		      &type,
		      (ub4*)0,
		      OCI_ATTR_DATA_TYPE,
		      dbcon->error_handle);
	
	if(!IS_SUCCESS(rc)) { errfunc="OCIAttrGet, OCI_ATTR_DATA_TYPE"; break;}
	
	rc=OCIAttrGet((void *)column_parameter,
		      OCI_DTYPE_PARAM,
		      &size,
		      (ub4*)0,
		      OCI_ATTR_DATA_SIZE,
		      dbcon->error_handle);
	
	if(!IS_SUCCESS(rc)) { errfunc="OCIAttrGet, OCI_ATTR_DATA_SIZE"; break;}
	
	rc=OCIAttrGet((void *)column_parameter,
		      OCI_DTYPE_PARAM,
		      &scale,
		      (ub4*)0,
		      OCI_ATTR_SCALE,
		      dbcon->error_handle);
	
	if(!IS_SUCCESS(rc)) { errfunc="OCIAttrGet, OCI_ATTR_SCALE"; break;}
	
	rc=OCIAttrGet((void *)column_parameter,
		      OCI_DTYPE_PARAM,
		      &name,
		      &namelen,
		      OCI_ATTR_NAME,
		      dbcon->error_handle);

	if(!IS_SUCCESS(rc)) { errfunc="OCIAttrGet, OCI_ATTR_NAME"; break;}

      }while(0);

      THREADS_DISALLOW();
/*      UNLOCK(dbcon->lock); */

      if(!IS_SUCCESS(rc))
	ora_error_handler(dbcon->error_handle, rc, errfunc);

#ifdef ORACLE_DEBUG
      name[namelen]=0;
      fprintf(stderr,"FIELD: name=%s length=%d type=%d\n",name,size,type);
#endif


      push_object( o=clone_object(dbresultinfo_program,0) );
      info= (struct dbresultinfo *)STORAGE(o);

      info->name=make_shared_binary_string(name, namelen);
      info->length=size;
      info->decimals=scale;
      info->real_type=type;

      data_size=0;

      switch(type)
      {
	case SQLT_INT:
	  type_name="int";
	  data_size=sizeof(info->data.u.i);
	  type=SQLT_INT;
	  break;

	case SQLT_NUM:
	  type_name="number";
	  if(scale>0)
	  {
	    data_size=sizeof(info->data.u.f);
	    type=SQLT_FLT;
	  }else{
#if 0
	    data_size=sizeof(info->data.u.i);
	    type=SQLT_INT;
#else
	    
	    data_size=sizeof(info->data.u.num);
#ifdef ORACLE_DEBUG
/* 	    OCINumberSetZero(dbcon->error_handle, &info->data.u.num); */
	    MEMSET(&info->data.u.num, 0, data_size);
#endif
	    type=SQLT_NUM;
#endif
	  }
	  break;
	      
	case SQLT_FLT:
	  type_name="float";
	  data_size=sizeof(info->data.u.f);
	  type=SQLT_FLT;
	  break;

	case SQLT_STR: /* string */
	case SQLT_AFC: /* char */
	case SQLT_AVC: /* charz */

	case SQLT_CHR: /* varchar2 */
	case SQLT_VCS: /* varchar */
	case SQLT_LNG: /* long */
	case SQLT_LVC: /* long varchar */
	  type_name="char";
	  data_size=-1;
	  type=SQLT_LNG;
	  break;
	  
      case SQLT_CLOB:
      case SQLT_BLOB:
	if(type == SQLT_BLOB)
	  type_name = "blob";
	else
	  type_name="clob";
	if (rc = OCIDescriptorAlloc(
				    (dvoid *) get_oracle_environment(),
				    (dvoid **) &info->data.lob, 
				    (ub4)OCI_DTYPE_LOB, 
				    (size_t) 0, 
				    (dvoid **) 0))
	  {
#ifdef ORACLE_DEBUG
	    fprintf(stderr,"OCIDescriptorAlloc failed!\n");
#endif
	    info->data.lob = 0;
	    info->define_handle = 0;
	    ora_error_handler(dbcon->error_handle, rc, "OCIDescriptorAlloc");
	  }
	data_size=sizeof(info->data.lob); /* ? */
	break;

	case SQLT_RID:
	case SQLT_RDD:
	  type_name="rowid";
	  data_size=-1;
	  type=SQLT_LNG;
	  break;

	case SQLT_DAT:
	case SQLT_ODT:
	  type_name="date";
	  data_size=sizeof(info->data.u.date);
	  type=SQLT_ODT;
	  break;

	case SQLT_BIN: /* raw */
	case SQLT_VBI: /* varraw */
	case SQLT_LBI: /* long raw */
	  type_name="raw"; 
	  data_size=-1;
	  type=SQLT_LBI;
	  /*** dynamic ****/
	  break;

	case SQLT_LAB:
	  type_name="mslabel";
	  type=SQLT_LBI;
	  data_size=-1;
	  break;


	default:
	  type_name="unknown";
	  type=SQLT_LBI;
	  data_size=-1;
          break;
      }

      info->data.ftype=type;

      if(type_name)
	info->type=make_shared_string(type_name);

      rc=OCIDefineByPos(dbquery->statement,
			&info->define_handle,
			dbcon->error_handle,
			i+1,
			info->data.lob ? 
			& info->data.lob :
			& info->data.u,
#ifdef STATIC_BUFFERS
			data_size<0? STATIC_BUFFERS :data_size,
#else
			data_size<0? 8000 :data_size,
#endif
			type,
			& info->data.indicator,
			& info->data.len,
			& info->data.rcode,
#ifdef STATIC_BUFFERS
			0
#else
			OCI_DYNAMIC_FETCH
#endif
	);

#ifdef ORACLE_DEBUG
      fprintf(stderr,"data_size=%d type=%d SQLT_INT=%d\n",
	      data_size,
	      type,
	      SQLT_INT);
#endif

      if(!IS_SUCCESS(rc))
	ora_error_handler(dbcon->error_handle, rc, "OCIDefineByPos");

#ifndef STATIC_BUFFERS
      rc=OCIDefineDynamic(info->define_handle,
			  dbcon->error_handle,
			  info,
			  define_callback);
      if(!IS_SUCCESS(rc))
	ora_error_handler(dbcon->error_handle, rc, "OCIDefineDynamic");
#endif

    }
    f_aggregate(dbquery->cols);
    add_ref( dbquery->field_info=Pike_sp[-1].u.array );
  }else{
    ref_push_array( dbquery->field_info);
  }
}

static void push_inout_value(struct inout *inout,
			     struct dbcon *dbcon)
{
  ub4   loblen = 0;
  ub1   *bufp = 0;
  ub4   amtp = 0;
  char buffer[100];
  sword rc;
  sb4 bsize=100;
  int rslt;
  char *errfunc=0;
  
#ifdef ORACLE_DEBUG
  fprintf(stderr,"%s .. (type = %d, indicator = %d, len= %d)\n",__FUNCTION__,inout->ftype,inout->indicator, inout->xlen);
#endif

  if(inout->indicator)
  {
    switch(inout->ftype)
    {
      case SQLT_CLOB:
      case SQLT_BLOB:
      case SQLT_BIN:
      case SQLT_LBI:
      case SQLT_AFC:
      case SQLT_LAB:
      case SQLT_LNG:
      case SQLT_CHR:
      case SQLT_STR:
	ref_push_object(nullstring_object);
	break;

      case SQLT_ODT:
      case SQLT_DAT:
	ref_push_object(nulldate_object);
	break;
	
      case SQLT_NUM:
      case SQLT_INT:
	ref_push_object(nullint_object);
	break;
	
      case SQLT_FLT:
	ref_push_object(nullfloat_object);
	break;
	
      default:
	Pike_error("Unknown data type.\n");
	break;
    }
    return;
  }

  switch(inout->ftype)
  {
    case SQLT_BIN:
    case SQLT_LBI:
    case SQLT_AFC:
    case SQLT_LAB:
    case SQLT_LNG:
    case SQLT_CHR:
    case SQLT_STR:

#ifdef STATIC_BUFFERS
      if(!STRING_BUILDER_STR(inout->output))
      {
	push_string(make_shared_binary_string(inout->u.str,inout->len));
	break;
      }
#endif
      STRING_BUILDER_LEN(inout->output)+=inout->xlen-BLOCKSIZE;
      if(inout->ftype == SQLT_STR) 
	STRING_BUILDER_LEN(inout->output)--;
      inout->xlen=0;
      push_string(finish_string_builder(& inout->output));
      STRING_BUILDER_STR(inout->output)=0;;
      break;

    case SQLT_CLOB:
    case SQLT_BLOB:
    {
      sword ret;
      if((ret = OCILobGetLength(dbcon->context, dbcon->error_handle,
				inout->lob, &loblen)) != OCI_SUCCESS) {
#ifdef ORACLE_DEBUG
	fprintf(stderr,"OCILobGetLength failed.\n");
#endif
	errfunc = "OCILobGetLength";
      } else {
	amtp = loblen;
	if(bufp = malloc(loblen)) {
	  if((ret = OCILobRead(dbcon->context,
			       dbcon->error_handle,
			       inout->lob, 
			       &amtp, 
			       1, 
			       (dvoid *) bufp,
			       loblen, 
			       (dvoid *)0, 
			       (sb4 (*)(dvoid *, CONST dvoid *, ub4, ub1)) 0,
			       (ub2) 0, 
			       (ub1) SQLCS_IMPLICIT)) != OCI_SUCCESS) 
	    {
#ifdef ORACLE_DEBUG
	      fprintf(stderr,"OCILobRead failed\n");
#endif
	      errfunc = "OCILobRead";
	    }
	}
	else {
	  ret = 1;
	  errfunc = "malloc";
	}
      }
#ifdef ORACLE_DEBUG
      fprintf(stderr,"LOB length: %d\n",loblen);
#endif
      if(ret == OCI_SUCCESS)
	push_string(make_shared_binary_string(loblen ? bufp : "",loblen));
      else
	ora_error_handler(dbcon->error_handle, ret, errfunc);
      
      if(bufp)
	free(bufp);
#if 0
      /*  Handle automatically freed when environment handle is deallocated.
	  Not needed according according to doc.*/
      if(inout->lob)
	OCIDescriptorFree((dvoid *) inout->lob, (ub4) OCI_DTYPE_LOB);
#endif
      }
      break;
	
    case SQLT_ODT:
    case SQLT_DAT:
      push_object(low_clone(Date_program));
      call_c_initializers(Pike_sp[-1].u.object);
      ((struct dbdate *)STORAGE(Pike_sp[-1].u.object))->date = inout->u.date;
      break;

    case SQLT_NUM:
    {
#ifdef INT64
      INT64 integer;
#else
      INT32 integer;
#endif
      sword ret;

      /* Kluge */
      MEMMOVE(inout->u.shortstr+1,inout->u.shortstr,inout->xlen);
      inout->u.shortstr[0]=inout->xlen;

#if 0
      for(ret=0;ret<22;ret++)
	fprintf(stderr,"%02x ",((unsigned char *)&inout->u.num)[ret]);
      fprintf(stderr,"\n");
#endif

      ret=OCINumberToInt(dbcon->error_handle,
			 &inout->u.num,
			 sizeof(integer),
			 OCI_NUMBER_SIGNED,
			 &integer);

      if(IS_SUCCESS(ret))
      {
	push_int(integer);
      }else{
#ifdef AUTO_BIGNUM
	unsigned char buffer[80];
	ub4 buf_size=sizeof(buffer)-1;
#define FMT "FM99999999999999999999999999999999999999"
/* There should be no more than 38 '9':s in the FMT string. Oracle only
 * allows 38 digits of precision, and can cause an "ORA-22061: invalid
 * format text" if the format string requests more digits than this.
 */

	ret=OCINumberToText(dbcon->error_handle,
			    &inout->u.num,
			    FMT,
			    sizeof(FMT)-sizeof(""),
			    0,
			    0,
			    &buf_size,
			    buffer);
	if(IS_SUCCESS(ret))
	{
	  push_string(make_shared_binary_string(buffer,buf_size));
	  convert_stack_top_to_bignum();
	}else
#endif
	  ora_error_handler(dbcon->error_handle, ret, "OCINumberToInt");
      }
    }
    break;
      
    case SQLT_INT:
      push_int64(inout->u.i);
      break;
      
    case SQLT_FLT:
      push_float(inout->u.f); /* We might need to push a Matrix here */
      break;
      
    default:
      Pike_error("Unknown data type.\n");
      break;
  }
  free_inout(inout);
}

static void init_inout(struct inout *i)
{
  STRING_BUILDER_STR(i->output)=0;
  i->has_output=0;
  i->xlen=0;
  i->len=0;
  i->indicator=0;
}

static void free_inout(struct inout *i)
{
  if(STRING_BUILDER_STR(i->output))
  {
    free_string_builder(& i->output);
    init_inout(i);
  }
}


static void f_fetch_row(INT32 args)
{
  int i;
  sword rc;
  struct dbresult *dbresult = THIS_RESULT;
  struct dbquery *dbquery = THIS_RESULT_QUERY;
  struct dbcon *dbcon = THIS_RESULT_DBCON;

#ifdef ORACLE_DEBUG
  fprintf(stderr,"%s ..",__FUNCTION__);
#endif

  pop_n_elems(args);

  if(dbquery->cols==-2)
  {
    f_fetch_fields(0);
    pop_stack();
  }

  THREADS_ALLOW();
#ifdef ORACLE_DEBUG
  fprintf(stderr,"OCIStmtFetch\n");
#endif
  rc=OCIStmtFetch(dbquery->statement,
		  dbcon->error_handle,
		  1,
		  OCI_FETCH_NEXT,
		  OCI_DEFAULT);
#ifdef ORACLE_DEBUG
  fprintf(stderr,"OCIStmtFetch done\n");
#endif
  THREADS_DISALLOW();

  if(rc==OCI_NO_DATA)
  {
    push_int(0);
    return;
  }

  if(!IS_SUCCESS(rc))
    ora_error_handler(dbcon->error_handle, rc, "OCIStmtFetch");

  check_stack(dbquery->cols);

  for(i=0;i<dbquery->cols;i++)
  {
    if(dbquery->field_info->item[i].type == T_OBJECT &&
       dbquery->field_info->item[i].u.object->prog == dbresultinfo_program)
    {
      struct dbresultinfo *info;
      info=(struct dbresultinfo *)STORAGE(dbquery->field_info->item[i].u.object);

      /* Extract data from 'info' */
      push_inout_value(& info->data, dbcon);
    }
  }
  f_aggregate(dbquery->cols);
}

static void f_oracle_create(INT32 args)
{
  char *err=0;
  struct dbcon *dbcon = THIS_DBCON;
  struct pike_string *uid, *passwd, *host, *database;
  sword rc;

  check_all_args("Oracle.oracle->create", args,
		 BIT_STRING|BIT_INT, BIT_STRING|BIT_INT, BIT_STRING,
		 BIT_STRING|BIT_VOID|BIT_INT, 0);

  host = (Pike_sp[-args].type == T_STRING? Pike_sp[-args].u.string : NULL);
  database = (Pike_sp[1-args].type == T_STRING? Pike_sp[1-args].u.string : NULL);
  uid = (Pike_sp[2-args].type == T_STRING? Pike_sp[2-args].u.string : NULL);
  if(args >= 4)
    passwd = (Pike_sp[3-args].type == T_STRING? Pike_sp[3-args].u.string : NULL);
  else
    passwd = NULL;

  THREADS_ALLOW();

  LOCK(dbcon->lock);
  LOCK(oracle_serialization_mutex);

  do  {
    rc=OCIHandleAlloc(get_oracle_environment(),
		      (void **)& dbcon->error_handle,
		      OCI_HTYPE_ERROR,
		      0,
		      0);
    if(rc != OCI_SUCCESS) break;

#ifdef ORACLE_DEBUG
    fprintf(stderr,"OCIHandleAlloc -> %p\n",dbcon->error_handle);
#endif

#if 0
    if(OCIHandleAlloc(get_oracle_environment(),&THIS_DBCON->srvhp,
		      OCI_HTYPE_SERVER, 0,0)!=OCI_SUCCESS)
      Pike_error("Failed to allocate server handle.\n");
    
    if(OCIHandleAlloc(get_oracle_environment(),&THIS_DBCON->srchp,
		      OCI_HTYPE_SVCCTX, 0,0)!=OCI_SUCCESS)
      Pike_error("Failed to allocate service context.\n");
#endif


    rc=OCILogon(get_oracle_environment(),
		dbcon->error_handle,
		&dbcon->context,
		uid->str, uid->len, 
		(passwd? passwd->str:NULL), (passwd? passwd->len:-1),
		(host? host->str:NULL), (host? host->len:-1));
  
  }while(0);

  UNLOCK(oracle_serialization_mutex);
  UNLOCK(dbcon->lock);

  THREADS_DISALLOW();


  if(rc != OCI_SUCCESS)
    ora_error_handler(dbcon->error_handle, rc, 0);

  pop_n_elems(args);

#ifdef ORACLE_DEBUG
  fprintf(stderr,"oracle.create error_handle -> %p\n",dbcon->error_handle);
#endif

  return;
}

static void f_dbcon_timeout_limit(INT32 args)
{
  struct dbcon *dbcon=THIS_DBCON;
  int           new_timeout;
  /* No arguments: get timeout. One argument: set timeout. */
#ifdef ORACLE_DEBUG
  fprintf(stderr, "%s, dbcon=%p, args=%d\n", __FUNCTION__, dbcon, args);
#endif
  if (args)
  {
    get_all_args("Oracle->timeout", args, "%i", &new_timeout);
    if (new_timeout < 0)
      Pike_error("Negative timeout specified.\n");
    if (new_timeout > 3600) /* max one hour */
      new_timeout = 3600;
    dbcon->timeout_limit = new_timeout;
  }
  pop_n_elems(args);
  push_int(dbcon->timeout_limit);
}

static void f_compile_query_create(INT32 args)
{
  int rc;
  struct pike_string *query;
  struct dbquery *dbquery=THIS_QUERY;
  struct dbcon *dbcon=THIS_QUERY_DBCON;
  char *errfunc=0;

#ifdef ORACLE_DEBUG
  fprintf(stderr,"%s\n",__FUNCTION__);
#endif

  get_all_args("Oracle->compile_query", args, "%S", &query);

#ifdef ORACLE_DEBUG
  fprintf(stderr,"f_compile_query_create: dbquery: %p\n",dbquery);
  fprintf(stderr,"            dbcon: %p\n",dbcon);
  fprintf(stderr,"     error_handle: %p\n",dbcon->error_handle);
  fprintf(stderr,"resultobject_busy: %d\n", dbcon->resultobject_busy);
#endif

  if (dbcon->resultobject_busy)
  {
    int t, timeout_limit = threads_disabled ? 0 : dbcon->timeout_limit;

    for(t = 0; t < timeout_limit && dbcon->resultobject_busy; ++t)
    {
      THREADS_ALLOW();
      sleep(1);
      THREADS_DISALLOW();
    }
    if (dbcon->resultobject_busy)
      Pike_error("Oracle connection busy; previous result object "
		 "still active.\n");
  }

  THREADS_ALLOW();
  LOCK(dbcon->lock);
  
  rc=OCIHandleAlloc(get_oracle_environment(),
		    (void **)&dbquery->statement,
		    OCI_HTYPE_STMT,
		    0,0);
  
  if(rc == OCI_SUCCESS)
  {
    rc=OCIStmtPrepare(dbquery->statement,
		      dbcon->error_handle,
		      query->str,
		      query->len,
		      OCI_NTV_SYNTAX,
		      OCI_DEFAULT);

    if(rc == OCI_SUCCESS)
    {
      ub2 query_type;
      rc=OCIAttrGet(dbquery->statement,
		    OCI_HTYPE_STMT,
		    &query_type,
		    0,
		    OCI_ATTR_STMT_TYPE,
		    dbcon->error_handle);
      if(rc == OCI_SUCCESS)
      {
#ifdef ORACLE_DEBUG
	fprintf(stderr,"     query_type: %d\n",query_type);
#endif
	dbquery->query_type = query_type;
      }else{
	errfunc="OCIAttrGet";
      }
    }else{
      errfunc="OCIStmtPrepare";
    }
  }else{
    errfunc="OCIHandleAlloc";
  }
  
  THREADS_DISALLOW();
#ifdef ORACLE_DEBUG
  fprintf(stderr, "    Unlock; rc = %d, OCI_SUCCESS = %d\n", rc, OCI_SUCCESS);
#endif
  UNLOCK(dbcon->lock);


  if(rc != OCI_SUCCESS)
    ora_error_handler(dbcon->error_handle, rc, 0);

  pop_n_elems(args);
  push_int(0);
}

struct bind
{
  OCIBind *bind;
  struct svalue ind; /* The name of the input/output variable */

  struct svalue val; /* The input value */
  void *addr;
  int len;
  sb2 indicator;

  struct inout data;
};

#define MAX_NUMBER_OF_BINDINGS 256

struct bind_block
{
  struct bind bind[MAX_NUMBER_OF_BINDINGS];
  int bindnum;
};


static sb4 input_callback(void *vbind_struct,
			   OCIBind *bindp,
			   ub4 iter,
			   ub4 index,
			   void **bufpp,
			   ub4 *alenp,
			   ub1 *piecep,
			   dvoid **indpp)
{
  struct bind * bind = (struct bind *)vbind_struct;
#ifdef ORACLE_DEBUG
  fprintf(stderr,"%s %ld %ld\n",__FUNCTION__,(long)iter,(long)index);
#endif

  *bufpp = bind->addr;
  *alenp = bind->len;
  *indpp = (dvoid *) &bind->ind;
  *piecep = OCI_ONE_PIECE;

  return OCI_CONTINUE;
}

static sb4 bind_output_callback(void *vbind_struct,
				OCIBind *bindp,
				ub4 iter,
				ub4 index,
				void **bufpp,
				ub4 **alenpp,
				ub1 *piecep,
				dvoid **indpp,
				ub2 **rcodepp)
{
  struct bind * bind = (struct bind *)vbind_struct;
#ifdef ORACLE_DEBUG
  fprintf(stderr,"Output... %ld\n",(long)bind->data.xlen);
#endif

  output_callback( &bind->data,
		   iter,
		   bufpp,
		   alenpp,
		   piecep,
		   indpp,
		   rcodepp);
  
  return OCI_CONTINUE;
}

static void free_bind_block(struct bind_block *bind)
{

#ifdef ORACLE_DEBUG
  fprintf(stderr,"%s\n",__FUNCTION__);
#endif

  while(bind->bindnum>=0)
  {
    free_svalue( & bind->bind[bind->bindnum].ind);
    free_svalue( & bind->bind[bind->bindnum].val);
    free_inout(& bind->bind[bind->bindnum].data);
    bind->bindnum--;
  }
}

/*
 * FIXME: This function should probably lock the statement
 * handle until it is freed...
 */
static void f_big_query_create(INT32 args)
{
  sword rc;
  struct mapping *bnds=0;
  struct dbresult *dbresult=THIS_RESULT;
  struct dbquery *dbquery=THIS_RESULT_QUERY;
  struct dbcon *dbcon=THIS_RESULT_DBCON;
  ONERROR err;
  INT32 autocommit=0;
  int i,num;
  struct object *new_parent=0;
  struct bind_block bind;

  extern int d_flag;

  bind.bindnum=-1;

#ifdef ORACLE_DEBUG
  fprintf(stderr,"%s\n",__FUNCTION__);
#endif

  check_all_args("Oracle.oracle.compile_query->big_query", args,
		 BIT_VOID | BIT_MAPPING | BIT_INT, 
		 BIT_VOID | BIT_INT,
		 BIT_VOID | BIT_OBJECT,
		 0);

#ifdef _REENTRANT
  if(d_flag)
  {
      CHECK_INTERPRETER_LOCK();
      if(d_flag>1 && thread_for_id(th_self()) != Pike_thread_id)
        fatal("thread_for_id() (or Pike_thread_id) failed in interpreter.h! %p != %p\n",thread_for_id(th_self()),Pike_thread_id);
  }
#endif

  switch(args)
  {
    default:
      new_parent=Pike_sp[2-args].u.object;

    case 2:
      autocommit=Pike_sp[1-args].u.integer;

    case 1:
      if(Pike_sp[-args].type == T_MAPPING)
	bnds=Pike_sp[-args].u.mapping;

    case 0: break;
  }

  if(bnds && m_sizeof(bnds) > MAX_NUMBER_OF_BINDINGS)
    Pike_error("Too many variables.\n");

  destruct_objects_to_destruct();

  /* Optimize me with trylock! */
  THREADS_ALLOW();
  LOCK(dbquery->lock);
  dbresult->dbquery_lock=1;
  THREADS_DISALLOW();

  /* Time to re-parent if required */
  if(new_parent &&
     PARENTOF(PARENTOF(THISOBJ)) != new_parent)
  {
    if(new_parent->prog != PARENTOF(PARENTOF(THISOBJ))->parent->prog)
      Pike_error("Bad argument 3 to big_query.\n");

    /* We might need to check that there are no locks held here
     * but I don't beleive that could happen, so just go with it...
     */
    free_object(PARENTOF(PARENTOF(THISOBJ)));
    add_ref( PARENTOF(PARENTOF(THISOBJ)) = new_parent );
  }

#ifdef _REENTRANT
  if(d_flag)
  {
      CHECK_INTERPRETER_LOCK();
      if(d_flag>1 && thread_for_id(th_self()) != Pike_thread_id)
        fatal("thread_for_id() (or Pike_thread_id) failed in interpreter.h! %p != %p\n",thread_for_id(th_self()),Pike_thread_id);
  }
#endif

  SET_ONERROR(err, free_bind_block, &bind);

  if(bnds != NULL)
  {
    INT32 e;
    struct keypair *k;
    MAPPING_LOOP(bnds)
      {
	struct svalue *value=&k->val;
	OCIBind *bindp;
	sword rc = 0;
	void *addr;
	sword len, fty;
	int mode=OCI_DATA_AT_EXEC;
	long rlen=0x7fffffff;
	bind.bindnum++;
	
	assign_svalue_no_free(& bind.bind[bind.bindnum].ind, & k->ind);
	assign_svalue_no_free(& bind.bind[bind.bindnum].val, & k->val);
	bind.bind[bind.bindnum].indicator=0;

	init_inout(& bind.bind[bind.bindnum].data);

      retry:
	switch(value->type)
	{
	  case T_OBJECT:
	    if(value->u.object->prog == Date_program)
	    {
	      bind.bind[bind.bindnum].data.u.date=((struct dbdate *)STORAGE(value->u.object))->date;
	      addr = &bind.bind[bind.bindnum].data.u.date;
	      rlen = len = sizeof(bind.bind[bind.bindnum].data.u.date);
	      fty=SQLT_ODT;
	      break;
	    }
	    if(value->u.object->prog == NULL_program)
	    {
	      bind.bind[bind.bindnum].indicator=-1;
	      value=& ((struct dbnull *)STORAGE(value->u.object))->type;
	      goto retry;
	    }
	    Pike_error("Bad value type in argument 2 to "
		  "Oracle.oracle->big_query()\n");
	    break;

	  case T_STRING:
	    addr = (ub1 *)value->u.string->str;
	    len = value->u.string->len;
	    fty = SQLT_LNG;
	    break;
	    
	  case T_FLOAT:
	    addr = &value->u.float_number;
	    rlen = len = sizeof(value->u.float_number);
	    fty = SQLT_FLT;
	    break;
	    
	  case T_INT:
	    if(value->subtype)
	    {
#ifdef ORACLE_DEBUG
	      fprintf(stderr,"NULL IN\n");
#endif
	      bind.bind[bind.bindnum].indicator=-1;
	      addr = 0;
	      len = 0;
	      fty = SQLT_LNG;
	    }else{
	      bind.bind[bind.bindnum].data.u.i=value->u.integer;
	      addr = &bind.bind[bind.bindnum].data.u.i;
	      rlen = len = sizeof(bind.bind[bind.bindnum].data.u.i);
	      fty = SQLT_INT;
	    }
	    break;
	    
	  case T_MULTISET:
	    if(value->u.multiset->ind->size == 1 &&
	       ITEM(value->u.multiset->ind)[0].type == T_STRING)
	    {
	      addr = (ub1 *)ITEM(value->u.multiset->ind)[0].u.string->str;
	      len = ITEM(value->u.multiset->ind)[0].u.string->len;
	      fty = SQLT_LBI;
	      break;
	    }
	    
	  default:
	    Pike_error("Bad value type in argument 2 to "
		  "Oracle.oracle->big_query()\n");
	}
	
	bind.bind[bind.bindnum].addr=addr;
	bind.bind[bind.bindnum].len=len;
	bind.bind[bind.bindnum].data.ftype=fty;
	bind.bind[bind.bindnum].bind=0;
	bind.bind[bind.bindnum].data.curlen=1;
	
#ifdef ORACLE_DEBUG
	fprintf(stderr,"BINDING... rlen=%ld\n",(long)rlen);
#endif
	if(k->ind.type == T_INT)
	{
	  rc = OCIBindByPos(dbquery->statement,
			    & bind.bind[bind.bindnum].bind,
			    dbcon->error_handle,
			    k->ind.u.integer,
			    addr,
			    rlen,
			    fty,
			    & bind.bind[bind.bindnum].data.indicator,
			    & bind.bind[bind.bindnum].data.len,
			    & bind.bind[bind.bindnum].data.rcode,
			    0,
			    0,
			    mode);
	}
	else if(k->ind.type == T_STRING)
	{
	  rc = OCIBindByName(dbquery->statement,
			     & bind.bind[bind.bindnum].bind,
			     dbcon->error_handle,
			     k->ind.u.string->str,
			     k->ind.u.string->len,
			     addr,
			     rlen,
			     fty,
			     & bind.bind[bind.bindnum].data.indicator,
			     & bind.bind[bind.bindnum].data.len,
			     & bind.bind[bind.bindnum].data.rcode,
			     0,
			     0,
			     mode);
	}
	else
	{
	  Pike_error("Bad index type in argument 2 to "
		"Oracle.oracle->big_query()\n");
	}
	if(rc)
	{
	  UNLOCK(dbcon->lock);
	  ora_error_handler(dbcon->error_handle, rc, "OCiBindByName/Pos");
	}

	if(mode == OCI_DATA_AT_EXEC)
	{
	  rc=OCIBindDynamic(bind.bind[bind.bindnum].bind,
			    dbcon->error_handle,
			    (void *)(bind.bind + bind.bindnum),
			    input_callback,
			    (void *)(bind.bind + bind.bindnum),
			    bind_output_callback);
	  if(rc)
	  {
	    UNLOCK(dbcon->lock);
	    ora_error_handler(dbcon->error_handle, rc, "OCiBindDynamic");
	  }
	}
      }
  }
  THREADS_ALLOW();
  LOCK(dbcon->lock);
  dbresult->dbcon_lock=1;

#ifdef ORACLE_DEBUG
  fprintf(stderr,"OCIExec query_type=%d\n",dbquery->query_type);
#endif
  rc = OCIStmtExecute(dbcon->context,
		      dbquery->statement,
		      dbcon->error_handle,
		      dbquery->query_type == OCI_STMT_SELECT ? 0 : 1,
		      0,
		      0,0,
		      autocommit?OCI_DEFAULT:OCI_COMMIT_ON_SUCCESS);

#ifdef ORACLE_DEBUG
  fprintf(stderr,"OCIExec done\n");
#endif
  THREADS_DISALLOW();

#ifdef _REENTRANT
  if(d_flag)
  {
      CHECK_INTERPRETER_LOCK();
      if(d_flag>1 && thread_for_id(th_self()) != Pike_thread_id)
        fatal("thread_for_id() (or Pike_thread_id) failed in interpreter.h! %p != %p\n",thread_for_id(th_self()),Pike_thread_id);
  }
#endif

  if(!IS_SUCCESS(rc))
    ora_error_handler(dbcon->error_handle, rc, 0);

  pop_n_elems(args);

  
  for(num=i=0;i<=bind.bindnum;i++)
    if(bind.bind[i].data.has_output)
      num++;

  if(!num)
  {
    /* This will probably never happen, but better safe than sorry */
    if(dbquery->output_variables)
    {
      free_mapping(dbquery->output_variables);
      dbquery->output_variables=0;
    }
  }else{
    if(!dbquery->output_variables)
      dbquery->output_variables=allocate_mapping(num);

    for(num=i=0;i<=bind.bindnum;i++)
    {
      if(bind.bind[i].data.has_output)
      {
	push_inout_value(& bind.bind[i].data, dbcon);
	mapping_insert(dbquery->output_variables, & bind.bind[i].ind, Pike_sp-1);
	pop_stack();
      }
    }
  }

  free_bind_block(&bind);
  UNSET_ONERROR(err);

#ifdef _REENTRANT
  if(d_flag)
  {
      CHECK_INTERPRETER_LOCK();
      if(d_flag>1 && thread_for_id(th_self()) != Pike_thread_id)
        fatal("thread_for_id() (or Pike_thread_id) failed in interpreter.h! %p != %p\n",thread_for_id(th_self()),Pike_thread_id);
  }
#endif

}

static void dbdate_create(INT32 args)
{
  struct tm *tm;
  time_t t;
  sword rc;

  check_all_args("Oracle.Date",args,BIT_INT|BIT_STRING,0);
  switch(Pike_sp[-args].type)
  {
    case T_STRING:
      rc=OCIDateFromText(get_global_error_handle(),
			 Pike_sp[-args].u.string->str,
			 Pike_sp[-args].u.string->len,
			 0,
			 0,
			 0,
			 0,
			 & THIS_DBDATE->date);
      if(rc != OCI_SUCCESS)
	ora_error_handler(get_global_error_handle(), rc,"OCIDateFromText");
      break;

    case T_INT:
      t=Pike_sp[-1].u.integer;
      tm=localtime(&t);
      if (!tm) Pike_error ("localtime() failed to convert %ld\n", (long) t);
      OCIDateSetDate(&THIS_DBDATE->date, tm->tm_year, tm->tm_mon, tm->tm_mday);
      OCIDateSetTime(&THIS_DBDATE->date, tm->tm_hour, tm->tm_min, tm->tm_sec);
      break;
  }
}

static void dbdate_sprintf(INT32 args)
{
  char buffer[100];
  sword rc;
  sb4 bsize=100;
  rc=OCIDateToText(get_global_error_handle(),
		   &THIS_DBDATE->date,
		   0,
		   0,
		   0,
		   0,
		   &bsize,
		   buffer);
		
  if(!IS_SUCCESS(rc))
    ora_error_handler(get_global_error_handle(), rc,"OCIDateToText");

  pop_n_elems(args);
  push_text(buffer);
}

static void dbdate_cast(INT32 args)
{
  char *s;
  get_all_args("Oracle.Date->cast",args,"%s",&s);
  if(!strcmp(s,"int"))
  {
    struct tm *tm;
    time_t t;
    ub1 hour, min, sec, month,day;
    sb2 year;

    extern void f_mktime(INT32 args);

    OCIDateGetDate(&THIS_DBDATE->date, &year, &month, &day);
    OCIDateGetTime(&THIS_DBDATE->date, &hour, &min, &sec);

    push_int(sec);
    push_int(min);
    push_int(hour);
    push_int(day);
    push_int(month);
    push_int(year);
    f_mktime(6);
    return;
  }
  if(!strcmp(s,"string"))
  {
    dbdate_sprintf(args);
    return;
  }
  Pike_error("Cannot cast Oracle.Date to %s\n",s);
}

static void dbnull_create(INT32 args)
{
  if(args<1) Pike_error("Too few arguments to Oracle.NULL->create\n");
  assign_svalue(& THIS_DBNULL->type, Pike_sp-args);
}

static void dbnull_sprintf(INT32 args)
{
  switch(THIS_DBNULL->type.type)
  {
    case T_INT: push_text("Oracle.NULLint"); break;
    case T_STRING: push_text("Oracle.NULLstring"); break;
    case T_FLOAT: push_text("Oracle.NULLfloat"); break;
    case T_OBJECT: push_text("Oracle.NULLdate"); break;

  }

}

static void dbnull_not(INT32 args)
{
  pop_n_elems(args);
  push_int(1);
}

void pike_module_init(void)
{
  long offset=0;
#ifdef ORACLE_HOME
  if(getenv("ORACLE_HOME")==NULL)
    putenv("ORACLE_HOME="ORACLE_HOME);
#endif
#ifdef ORACLE_SID
  if(getenv("ORACLE_SID")==NULL)
    putenv("ORACLE_SID="ORACLE_SID);
#endif

#ifdef ORACLE_DEBUG
  fprintf(stderr,"%s\n",__FUNCTION__);
#endif

  if(OCIInitialize(
    OCI_OBJECT | 
#ifdef ORACLE_USE_THREADS
    OCI_DEFAULT,
#else
    OCI_THREADED,
#endif
    0, 0, 0, 0) != OCI_SUCCESS)
  {
#ifdef ORACLE_DEBUG
    fprintf(stderr,"OCIInitialize failed\n");
#endif
    return;
  }

  if(oracle_program)
    fatal("Oracle module initiated twice!\n");

  MY_START_CLASS(dbcon); {

    MY_START_CLASS(dbquery); {
      add_function("create",f_compile_query_create,"function(string:void)",0);
      map_variable("_type","int",0,offset+OFFSETOF(dbquery,query_type),T_INT);
      map_variable("_cols","int",0,offset+OFFSETOF(dbquery, cols), T_INT);
      map_variable("_field_info","array(object)",0,
		   offset+OFFSETOF(dbquery, field_info),
		   T_ARRAY);
      map_variable("output_variables","mapping(string:mixed)",0,
		   offset+OFFSETOF(dbquery, output_variables),
		   T_MAPPING);
      
      
/*      add_function("query_type",f_query_type,"function(:int)",0); */

      MY_START_CLASS(dbresult); {
	ADD_FUNCTION("create", f_big_query_create,
		     tFunc(tOr(tVoid,tMap(tStr,tMix)) tComma tOr(tVoid,tInt) tComma tOr(tVoid,tObj),tVoid), ID_PUBLIC);
	
	/* function(:int) */
	ADD_FUNCTION("num_fields", f_num_fields,tFunc(tNone,tInt), ID_PUBLIC);
	
	/* function(:array(mapping(string:mixed))) */
	ADD_FUNCTION("fetch_fields", f_fetch_fields,tFunc(tNone,tArr(tMap(tStr,tMix))), ID_PUBLIC);
	
	/* function(:int|array(string|int)) */
	ADD_FUNCTION("fetch_row", f_fetch_row,tFunc(tNone,tOr(tInt,tArr(tOr(tStr,tInt)))), ID_PUBLIC);
	
	MY_END_CLASS(big_query);
#ifdef PROGRAM_USES_PARENT
	big_query_program->flags|=PROGRAM_USES_PARENT;
#endif
	big_query_program->flags|=PROGRAM_DESTRUCT_IMMEDIATE;
      }
      
      
      MY_START_CLASS(dbresultinfo); {
	map_variable("name","string",0,offset+OFFSETOF(dbresultinfo, name), T_STRING);
	map_variable("type","string",0,offset+OFFSETOF(dbresultinfo, type), T_STRING);
	map_variable("_type","int",0,offset+OFFSETOF(dbresultinfo, real_type), T_INT);
	map_variable("length","int",0,offset+OFFSETOF(dbresultinfo, length), T_INT);
	map_variable("decimals","int",0,offset+OFFSETOF(dbresultinfo, decimals), T_INT);
	
	ADD_FUNCTION("`->=",protect_dbresultinfo,tFunc(tStr tComma tMix,tVoid),0);
	ADD_FUNCTION("`[]=",protect_dbresultinfo,tFunc(tStr tComma tMix,tVoid),0);
#ifdef ORACLE_DEBUG
	set_gc_check_callback(gc_dbresultinfo_struct);
#endif
	MY_END_CLASS(dbresultinfo);
      }

      MY_END_CLASS(compile_query);
#ifdef PROGRAM_USES_PARENT
      compile_query_program->flags|=PROGRAM_USES_PARENT;
#endif
    }

    ADD_FUNCTION("timeout_limit", f_dbcon_timeout_limit,
		 tFunc(tOr(tInt,tVoid), tInt), ID_PUBLIC);

    ADD_FUNCTION("create", f_oracle_create,tFunc(tOr(tStr,tVoid) tComma tOr(tStr,tVoid) tComma tOr(tStr,tVoid) tComma tOr(tStr,tVoid),tVoid), ID_PUBLIC);
    
    MY_END_CLASS(oracle);
  }

  MY_START_CLASS(dbdate); {
    ADD_FUNCTION("create",dbdate_create,tFunc(tOr(tStr,tInt),tVoid),0);
    ADD_FUNCTION("cast",dbdate_cast,tFunc(tStr, tMix),0);
    ADD_FUNCTION("_sprintf",dbdate_sprintf,tFunc(tInt, tStr),0);
  }
  MY_END_CLASS(Date);

  MY_START_CLASS(dbnull); {
    ADD_FUNCTION("create",dbnull_create,tFunc(tOr(tStr,tInt),tVoid),0);
    ADD_FUNCTION("_sprintf",dbnull_sprintf,tFunc(tInt, tStr),0);
    ADD_FUNCTION("`!",dbnull_not,tFunc(tVoid, tInt),0);
    map_variable("type","mixed",0,offset+OFFSETOF(dbnull, type), T_MIXED);
  }
  NULL_program=end_program();
  add_program_constant("NULL", NULL_program, 0);

  push_text("");
  add_object_constant("NULLstring",nullstring_object=clone_object(NULL_program,1),0);

  push_int(0);
  add_object_constant("NULLint",nullint_object=clone_object(NULL_program,1),0);

  push_float(0.0);
  add_object_constant("NULLfloat",nullfloat_object=clone_object(NULL_program,1),0);

  push_object(low_clone(Date_program));
  call_c_initializers(Pike_sp[-1].u.object);
  add_object_constant("NULLdate",nulldate_object=clone_object(NULL_program,1),0);
}

static void call_atexits(void);

void pike_module_exit(void)
{
#ifdef ORACLE_DEBUG
  fprintf(stderr,"%s\n",__FUNCTION__);
#endif

  if(global_error_handle)
  {
    OCIHandleFree(global_error_handle, OCI_HTYPE_ERROR);
    global_error_handle=0;
  }

#define FREE_PROG(X) if(X) { free_program(X) ; X=NULL; }
#define FREE_OBJ(X) if(X) { free_object(X) ; X=NULL; }
  FREE_PROG(oracle_program);
  FREE_PROG(compile_query_program);
  FREE_PROG(big_query_program);
  FREE_PROG(dbresultinfo_program);
  FREE_PROG(Date_program);
  FREE_PROG(NULL_program);

  FREE_OBJ(nullstring_object);
  FREE_OBJ(nullint_object);
  FREE_OBJ(nullfloat_object);
  FREE_OBJ(nulldate_object);

  call_atexits();
}

#ifdef DYNAMIC_MODULE

static int atexit_cnt=0;
static void (*atexit_fnc[32])(void);

int atexit(void (*func)(void))
{
  if(atexit_cnt==32)
    return -1;
  atexit_fnc[atexit_cnt++]=func;
  return 0;
}

static void call_atexits(void)
{
#ifdef ORACLE_DEBUG
  fprintf(stderr,"%s\n",__FUNCTION__);
#endif

  while(atexit_cnt)
    (*atexit_fnc[--atexit_cnt])();
}

#else /* DYNAMIC_MODULE */

static void call_atexits(void)
{
}

#endif /* DYNAMIC_MODULE */

#else /* HAVE_ORACLE */

void pike_module_init(void)  {}
void pike_module_exit(void)  {}

#endif

