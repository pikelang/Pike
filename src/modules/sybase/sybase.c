/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: sybase.c,v 1.14 2004/10/07 22:49:58 nilsson Exp $
*/

/*
 * Sybase driver for the Pike programming language.
 *
 * By Francesco Chemolli <kinkie@roxen.com> 10/12/1999
 *
 */

/* The sybase libraries are thread-safe, and very well-documented
 * about that (page 2-131)
 * For simplicity's sake, for now I'll use a per-connection (== per-object)
 * lock to serialize connection-related calls, and a program-level lock
 * for context-related operations. I could use more locks, to have a
 * more fine-grained locking, but since I now aim to generic SQL-interface
 * compliancy, this should do nicely.
 */

/*
 * wishlist/todolist:
 * - have an asynchronous messages reporter
 * - solve the signals-related problems
 */

#include "sybase_config.h"
#include "global.h"

#ifdef HAVE_SYBASE


#include "stralloc.h"
#include "pike_error.h"
#include "program.h"
#include "las.h"
#include "threads.h"
#include "module_support.h"
#include "builtin_functions.h"
#include "dmalloc.h"
#include "port.h"
#include "multiset.h"
#include "mapping.h"

#include "sybase.h"


#define sp Pike_sp

/* define this to enable debugging */
/* #define SYBDEBUG */

/* define this to cancel pending results instead of retrieving them */
#define FLUSH_BY_CANCELING

/* the sybase libraries are not particularly clever in letting the user
 * know how much memory space he'll need to allocate in order to completely
 * fit a column. They _will_ notify him if an overflow occurs, but then
 * it's too late, isn't it? Some complex strategies, involving re-fetching
 * an offending row (if an overflow occurs, ct_fetch will return CS_ROW_FAIL),
 * or trying to query sybase about the needed sizes for each fetched row.
 * For now it's just easier to reserve more space than actually necessary
 * This define marks how much extra memory is to be reserved for each
 * column.
 */
#define EXTRA_COLUMN_SPACE 10240

/* big. VERY big. (2^31) */
#define MAX_RESULTS_SIZE 0x7FFFFFFF

#ifdef SYBDEBUG
#define sybdebug(X) fprintf X
#define errdebug(err) if(err) sybdebug((stderr,err))
#else
#define sybdebug(X)
#define errdebug(err)
#endif

/* pike 0.6 compatibility stuff */
#ifndef ADD_STORAGE
#define ADD_STORAGE(X) add_storage(sizeof(X))
#endif


/* Actual code */
#ifdef _REENTRANT
/* note to self. Whenever the mainlock is needed and there is an
 * object involved, always acquire the object's lock first.
 * post scriptum. Override this: if the main lock is needed, acquire
 * only that. However, if anybody experiences deadlocks, restore this
 * behaviour.
 */
static MUTEX_T mainlock;
#define SYB_LOCK(X) sybdebug((stderr,"locking %s\n",#X));mt_lock(&(X))
#define SYB_UNLOCK(X) sybdebug((stderr,"unlocking %s\n",#X));mt_unlock(&(X))
#define SYB_MT_INIT(X) mt_init(&(X))
#define SYB_MT_EXIT(X) mt_destroy(&(X))
#else
#define SYB_LOCK(lock)
#define SYB_UNLOCK(lock)
#define SYB_MT_INIT(lock)
#define SYB_MT_EXIT(lock)
#endif

#define FAILED(status) (status!=CS_SUCCEED)
#define OK(status) (status==CS_SUCCEED)

#define THIS ((pike_sybase_connection *) (Pike_fp->current_storage))



/*************/
/* Utilities */
/*************/

#ifdef SYBDEBUG

/* just a small thing. see f_fetch_row to understand how it's used */
#define SHOW_STATUS(Y,X) case X:sybdebug((stderr,"\t%s is %s\n",Y,#X));break

static void show_status(CS_RETCODE ret) {
    switch(ret) {
      SHOW_STATUS("status",CS_SUCCEED);
      SHOW_STATUS("status",CS_PENDING);
      SHOW_STATUS("status",CS_BUSY);
      SHOW_STATUS("status",CS_END_RESULTS);
      SHOW_STATUS("status",CS_FAIL);
      SHOW_STATUS("status",CS_END_DATA);
    default: 
      sybdebug((stderr,"Unknown status: %d\n",(int)ret));
      break;
    }
}
static void show_results_type (CS_INT rtype) {
    switch(rtype) {
      SHOW_STATUS("type",CS_ROW_RESULT);
      SHOW_STATUS("type",CS_CURSOR_RESULT);
      SHOW_STATUS("type",CS_PARAM_RESULT);
      SHOW_STATUS("type",CS_STATUS_RESULT);
      SHOW_STATUS("type",CS_COMPUTE_RESULT);
      SHOW_STATUS("type",CS_MSG_RESULT);
      SHOW_STATUS("type",CS_DESCRIBE_RESULT);
      SHOW_STATUS("type",CS_ROWFMT_RESULT);
      SHOW_STATUS("type",CS_COMPUTEFMT_RESULT);
      SHOW_STATUS("type",CS_CMD_DONE);
      SHOW_STATUS("type",CS_CMD_SUCCEED);
      SHOW_STATUS("type",CS_CMD_FAIL);
    default:
      sybdebug((stderr,"Unknown type: %d\n",(int)rtype));
      break;
    }
}
static void show_severity(CS_INT severity) {
    switch (severity) {
      SHOW_STATUS("severity",CS_SV_INFORM);
      SHOW_STATUS("severity",CS_SV_CONFIG_FAIL);
      SHOW_STATUS("severity",CS_SV_RETRY_FAIL);
      SHOW_STATUS("severity",CS_SV_API_FAIL);
      SHOW_STATUS("severity",CS_SV_RESOURCE_FAIL);
      SHOW_STATUS("severity",CS_SV_COMM_FAIL);
      SHOW_STATUS("severity",CS_SV_INTERNAL_FAIL);
      SHOW_STATUS("severity",CS_SV_FATAL);
    }  
}
#else /* SYBDEBUG */
#define show_status(X)
#define show_results_type(X)
#define show_severity(X)
#endif

/*
 * Must be called in a MT-safe fashion
 */
static INT32 guess_column_length (CS_COMMAND *cmd, int column_number) {
  CS_DATAFMT description;

  ct_describe(cmd,column_number,&description);
  return (INT32)description.maxlength;
}

/* this function does _NOT_ involve any pike processing
 * If run in MT-enabled sections, locking is to be insured by the caller
 * on a per-command-structure basis.
 */
static void flush_results_queue(pike_sybase_connection *this) {
  CS_INT rtype;
  CS_RETCODE ret;
  int j;
  CS_COMMAND *cmd;


  sybdebug((stderr,"Flushing the results queue\n"));
#ifdef FLUSH_BY_CANCELING
  ct_cancel(this->connection,NULL,CS_CANCEL_ALL);
#else
  cmd=this->cmd;
  for (j=0;j<100;j++) { /* safety valve, I don't want to loop forever */
    
    sybdebug((stderr,"Getting results #%d\n",j));
    ret=ct_results(cmd,&rtype);
    show_status(ret);
    show_results_type(rtype);

    switch(ret) {
    case CS_END_RESULTS:
      sybdebug((stderr,"Finished flushing results\n"));
      return;
    case CS_FAIL:
      ret=ct_cancel(this->connection,NULL,CS_CANCEL_ALL);
      continue;
    }
    
    /* I'd probably be getting a result back here. I don't need it, so
     * I'll just cancel */
    switch(rtype) {
    case CS_ROW_RESULT:
    case CS_STATUS_RESULT:
    case CS_PARAM_RESULT:
      sybdebug((stderr,"Unexpected result. Cancel-ing\n"));
      ret=ct_cancel(NULL,this->cmd,CS_CANCEL_CURRENT);
      show_status(ret);
      break;
    }
  }
#endif
}

/*
 * returns 1 if there's any Pike_error
 * the same thread-safety rules as flush_results_queue apply
 * QUESTION: 
 * Should I explore all messages, and leave in this->error the
 * last one with an error-level severity?
 * or should we maybe only consider server messages?
 */
static int handle_errors (pike_sybase_connection *this) {
  SQLCA message;
  CS_INT num,j, severity;
  CS_RETCODE ret;
  
  sybdebug((stderr,"Handling errors\n"));
  ret=ct_diag(this->connection,CS_STATUS,CS_ALLMSG_TYPE,CS_UNUSED,&num);
  show_status(ret);
  if (FAILED(ret)) {
    sybdebug((stderr,"Error while retrieving errors number"));
    return 1;
  }
  sybdebug ((stderr, "I have %d messages in queue\n",(int)num));

  if (!num) /* no need to go through the moves */
    return 0;

  for (j=1;j<=num;j++) {
    ct_diag(this->connection, CS_GET, SQLCA_TYPE, j, &message);
    if (FAILED(ret)) {
      sybdebug((stderr,"Error while retrieving last message\n"));
      return 1;
    }
    sybdebug((stderr,"\nMessage %d length: %ld; text:\n%s\n",j,
             ((int)message.sqlerrm.sqlerrml),message.sqlerrm.sqlerrmc));
    /* If it's severe enough? */
    severity=CS_SEVERITY(message.sqlcode);
    show_severity(severity);
  }

  MEMCPY(this->error,message.sqlerrm.sqlerrmc,
         message.sqlerrm.sqlerrml+1);

  this->had_error=1;
  ct_diag(this->connection,CS_CLEAR,SQLCA_TYPE,CS_UNUSED,NULL);
  
  return 0;
}


/*******************/
/* low_level stuff */
/*******************/

static void sybase_create (struct object * o) {
  pike_sybase_connection *this=THIS;
  
  sybdebug((stderr,"sybase_create()\n"));
  this->context=NULL;
  this->connection=NULL;
  this->cmd=NULL;
  this->busy=0;
  this->results=NULL;
  this->results_lengths=NULL;
  this->nulls=NULL;
  this->numcols=0;
  this->had_error=0;
  SYB_MT_INIT(THIS->lock);
}


static void sybase_destroy (struct object * o) {
  pike_sybase_connection *this=THIS;
  CS_RETCODE ret;
  int j;

  sybdebug((stderr,"sybase_destroy()\n"));
  THREADS_ALLOW();
  SYB_LOCK(this->lock);


  if (this->busy > 0) {
    sybdebug((stderr,"We're busy while destroying. Trying to cancel\n"));
    ret=ct_cancel(this->connection,NULL,CS_CANCEL_ALL); 
    show_status(ret);
    /* we only have one active command, but what the hell.. */
    if (FAILED(ret)) {
      sybdebug((stderr,"\tUh oh... failed\n"));
    }
    this->busy--;
    sybdebug((stderr,"Busy status: %d\n",this->busy));
    /* if we fail, it's useless anyways. Maybe we should Pike_fatal() */
  }
  
  if (this->cmd) {
    sybdebug((stderr,"this->cmd still active. Dropping\n"));
    ret=ct_cmd_drop(this->cmd);
    show_status(ret);
    if (FAILED(ret)) {
      sybdebug((stderr,"\tHm... failed\n"));
    }
    this->cmd=NULL; 
    /* if we fail, it's useless anyways. Maybe we should Pike_fatal() */
  }

  if (this->results) {
    sybdebug((stderr,"Freeing results buffers\n"));
    for (j=0;j<this->numcols;j++) {
      free(this->results[j]);
    }
    free (this->results);
    free (this->results_lengths);
    free (this->nulls);
    this->results=NULL;
    this->results_lengths=NULL;
    this->nulls=NULL;
    this->numcols=0;
  }

  if (this->connection) {
    sybdebug((stderr,"this->connection still active. Closing\n"));
    ret=ct_close(this->connection,CS_UNUSED);
    show_status(ret);
    if (FAILED(ret)) {
      sybdebug((stderr,"\tArgh! Failed!\n"));
    }
    sybdebug((stderr,"Dropping connection\n"));
    ret=ct_con_drop(this->connection);
    show_status(ret);
    if (FAILED(ret)) {
      sybdebug((stderr,"Gasp! Failed!\n"));
    }
    this->connection=NULL;
    /* if we fail, it's useless anyways. Maybe we should Pike_fatal() */
  }

  SYB_LOCK(mainlock); /* this is really needed only here */

  if (this->context) {
    sybdebug((stderr,"this->context still active. Exiting\n"));
    ret = ct_exit(this->context, CS_UNUSED);
    show_status(ret);
    if (FAILED(ret)) {
      sybdebug((stderr,"\tGosh Batman! We failed!\n"));
    }
    ret = cs_ctx_drop(this->context);
    show_status(ret);
    if (FAILED(ret)) {
      sybdebug((stderr,"\tGosh Robin! You're right!\n"));
    }
    this->context=NULL;
    /* if we fail, it's useless anyways. Maybe we should Pike_fatal() */
  }

  SYB_UNLOCK(mainlock);
  SYB_UNLOCK(this->lock);
  
  THREADS_DISALLOW();
  SYB_MT_EXIT(THIS->lock);
}

/**********************/
/* exported functions */
/**********************/

static void f_error(INT32 args) {
  pike_sybase_connection *this=THIS;

  pop_n_elems(args);

  if (this->had_error)
    push_text(this->error);
  else
    push_int(0);
}


/* void connect(host, database, username, password, port) */
/*  The port and database arguments are silently ignored  */
static void f_connect (INT32 args) {
  CS_RETCODE ret;
  CS_CONTEXT *context;
  CS_CONNECTION *connection;
  char *username=NULL, *pass=NULL, *hostname=NULL, *err=NULL;
  int usernamelen=0, passlen=0, hostnamelen=CS_UNUSED;
  pike_sybase_connection *this=THIS;

  
  sybdebug((stderr,"sybase::connect(args=%d)\n",args));
  check_all_args("sybase->connect",args,
                 BIT_STRING|BIT_VOID,BIT_STRING|BIT_VOID,
                 BIT_STRING|BIT_VOID,BIT_STRING|BIT_VOID,
                 BIT_INT|BIT_VOID,0);

  /* fetch arguments, so that we can enable threads */
  if (sp[2-args].type == T_STRING && sp[2-args].u.string->len) { /*username*/
    username=sp[2-args].u.string->str;
    usernamelen=sp[2-args].u.string->len;
    sybdebug((stderr,"\tgot username: %s\n",username));
  }
  if (sp[3-args].type == T_STRING && sp[2-args].u.string->len) { /*password*/
      pass=sp[3-args].u.string->str;
    passlen=sp[3-args].u.string->len;
    sybdebug((stderr,"\tgot password: %s\n",pass));
  }
  if (sp[-args].type==T_STRING && sp[-args].u.string->len) { /*hostname*/
    hostname=sp[-args].u.string->str;
    hostnamelen=sp[-args].u.string->len;
    sybdebug((stderr,"\tgot hostname: %s\n",hostname));
  }

  THREADS_ALLOW();
  SYB_LOCK(mainlock); 
  
  /* It's OK not to lock here. It's just a check that should never happen.
   * if it happens, we're in deep sh*t already.*/
  if (!(context=this->context)) {
    err="Internal error: connection attempted, but no context available\n";
  }
  
  if (!err) {
    sybdebug((stderr,"\tallocating context\n"));
    ret=ct_con_alloc(context,&connection); /*sybase says it's thread-safe..*/
    show_status(ret);
    if (FAILED(ret)) {
      err="Cannot initialize connection\n";
    }
  }
  errdebug(err);
  
  if (!err) { /* initialize error-handling code */
    ret=ct_diag(connection,CS_INIT,CS_UNUSED,CS_UNUSED,NULL);
    show_status(ret);
    if (FAILED(ret)) {
      err="Can't initialize error-handling code\n";
    }
  }
  errdebug(err);

  /* if there already was an error, we just locked uselessly.
   * No big deal, it should never happen anyways... */

  /* username */
  if (!err && usernamelen) {
    sybdebug((stderr,"\tsetting username to \"%s\"\n",username));
    ret=ct_con_props(connection,CS_SET,CS_USERNAME,
                     username,usernamelen,NULL);
    show_status(ret);
    if (FAILED(ret)) {
      err="Can't set username\n";
    }
  }
  errdebug(err);

  /* password */
  if (!err && passlen) {
    sybdebug((stderr,"\tsetting password to \"%s\"\n",pass));
    ret=ct_con_props(connection,CS_SET,CS_PASSWORD,
                     pass,passlen,NULL);
    show_status(ret);
    if (FAILED(ret)) {
      err="Can't set password\n";
    }
  }
  errdebug(err);

  /* connect, finally */
  if (!err) {
    if (hostname) {
      sybdebug((stderr,"\tconnecting to hostname is \"%s\"\n",hostname));
    }
    ret=ct_connect(connection,(CS_CHAR *)hostname, hostnamelen);
    show_status(ret);
    if (FAILED(ret)) {
      err="Can't connect\n";
    } else {
      this->connection=connection;
    }
  }
  errdebug(err);

  if (err) ct_con_drop(connection);
      
  SYB_UNLOCK(mainlock);
  THREADS_DISALLOW();

  if (err) {
    handle_errors(this); /* let's centralize */
    Pike_error(err);
  }

  pop_n_elems(args);
  sybdebug((stderr,"sybase::connect exiting\n"));
  
}

/* create (host,database,username,password,port) */
static void f_create (INT32 args) {
  char *host,*db,*user,*pass;
  CS_RETCODE ret;
  CS_CONTEXT *context;
  char* err=NULL;
  pike_sybase_connection *this=THIS;
  
  sybdebug((stderr,"sybase::create(args=%d)\n",args));

  check_all_args("sybase->create",args,
                 BIT_STRING|BIT_VOID,BIT_STRING|BIT_VOID,
                 BIT_STRING|BIT_VOID,BIT_STRING|BIT_VOID,
                 BIT_INT|BIT_VOID,0);
  
  /* if connected, disconnect */
  if (this->context)
    sybase_destroy(Pike_fp->current_object);

  THREADS_ALLOW();
  SYB_LOCK(mainlock);

  /* allocate context */
  ret=cs_ctx_alloc(CS_VERSION_110,&(this->context));
  show_status(ret);
  if (FAILED(ret))
    err = "Cannot allocate context!\n";
  
  context=this->context;

  /* initialize open client-library emulation */
  if (!err) {
    ret=ct_init(context,CS_VERSION_110);
    if (FAILED(ret)) {
      ct_exit(context,CS_UNUSED);
      err="Cannot initialize library version 1.11.X!\n";
    }
  }

  /* if there was no error, we can try to connect. Since f_connect
   * will do its own threads meddling, we must disable threads first
   */

  if (err) { /* there was an error. bail out */
    cs_ctx_drop(context);
    this->context=NULL;
  }

  SYB_UNLOCK(mainlock);
  THREADS_DISALLOW();
  
  if (err) Pike_error(err); /* throw the exception if appropriate */

  /* now connect */
  if (args)
    f_connect(args);

  sybdebug((stderr,"sybase::create exiting\n"));
  /* the args are popped in f_connect */
}

/* TODO: should I cancel a pending query instead of throwing errors?
 *
 * object(sybase.sybase_result)|string big_query(string q)
 *
 * NOTICE: the string return value is an extension to the standard API.
 *  It will returned ONLY if it is the return status from a stored procedure
 */

#define PS_NORESULT 1
#define PS_RESULT_THIS 2
#define PS_RESULT_STATUS 3 
/* if function_result == PS_RESULT_STATUS, the value to be returned
   is *function_result */

static void f_big_query(INT32 args) {
  pike_sybase_connection *this=THIS;
  char *query, *err=NULL, *function_result=NULL;
  int querylen, numcols,j, done=0;
  CS_COMMAND *cmd=this->cmd;
  CS_RETCODE ret;
  CS_DATAFMT description, got_desc;
  CS_INT rtype;
  char ** results;
  CS_INT *results_lengths;
  CS_SMALLINT *nulls;
  int toreturn=PS_NORESULT; /* one of the #defines here above */
  
  
  
  /* check, get and pop args */
  check_all_args("sybase->big_query",args,BIT_STRING,0);
  query=sp[-args].u.string->str;
  querylen=sp[-args].u.string->len;
  sybdebug((stderr,"query: '%s'\n",query));
  /* pop_n_elems(args); Let's try moving it later */

  sybdebug((stderr,"Busy status: %d\n",this->busy));

  /* verify that we're not busy handing out results */
  if (this->busy > 0)
    Pike_error("Busy\n");

  THREADS_ALLOW();
  SYB_LOCK(this->lock);
  
  if (cmd==NULL) {
    /* no sense in alloc-ing everytime */
    sybdebug((stderr,"Allocating command structure\n"));
    ret=ct_cmd_alloc(this->connection, &cmd);
    show_status(ret);
    
    if (FAILED(ret)) {
      sybdebug((stderr,"\tUh oh... problems\n"));
      err="Error allocating command\n";
    }

    this->cmd=cmd;
  }

  if (this->results) {
    sybdebug((stderr,"Freeing previous results DMA memory\n"));
    for (j=0;j<(this->numcols);j++) {
      sybdebug((stderr,"Column %d (%p)\n",j,this->results[j]));
      free(this->results[j]);
    }
    sybdebug((stderr,"Freeing main results array\n"));
    free(this->results);
    free(this->results_lengths);
    free(this->nulls);
    this->results=NULL;
    this->results_lengths=NULL;
    this->nulls=NULL;
    this->numcols=0;
  }

  if (!err) { /* we can't be done yet */
    sybdebug((stderr,"issung command\n"));
    ret=ct_command(cmd,CS_LANG_CMD,query,querylen,CS_UNUSED);
    show_status(ret);
    if (FAILED(ret)) {
      sybdebug((stderr,"\tUh oh... something wrong here\n"));
      err="Error while setting command\n";
    }
  }

  if (!err) { /* we can't be done yet */
    sybdebug((stderr,"Sending command\n"));
    ret=ct_send(cmd);
    show_status(ret);
    this->busy++;
    sybdebug((stderr,"Busy status: %d\n",this->busy));
    if (FAILED(ret)) {
      sybdebug((stderr,"\thm... I don't like what I'm seeing\n"));
      err="Error while sending command\n";
    }
  }
  
  if (err) { /* we can't be done yet */
    sybdebug((stderr,"Problems. Dropping command\n"));
    ct_cmd_drop(cmd);
    this->cmd=NULL;
/*     this->busy--; */
/*     sybdebug((stderr,"Busy status: %d\n",this->busy)); */
  }


  /* let's move the first part of a process here.
   */
  
  while (!err && !done) {
    /* okay, let's see what we got */
    sybdebug((stderr,"Issuing results\n"));
    ret=ct_results(cmd,&rtype);
    show_status(ret);
    show_results_type(rtype);
    
    switch(ret) {
    case CS_PENDING:
    case CS_BUSY:
      sybdebug((stderr,"Got strange stuff\n"));
      err="Async operations are not supported\n";
      break;
    case CS_END_RESULTS:
      sybdebug((stderr,"No more results\n"));
/*       this->busy--; */
/*       sybdebug((stderr,"Busy status: %d\n",this->busy)); */
      done=1;/* we return 0 from a query. Sounds good.. */
      break;
    case CS_FAIL: /* uh oh, something's terribly wrong */
      sybdebug((stderr,"Command failed. Canceling\n"));
      ct_cancel(this->connection,NULL,CS_CANCEL_ALL);/* try to cancel result */
/*       this->busy--; */
/*       sybdebug((stderr,"Busy status: %d\n",this->busy)); */
      /* we should really check the return code here.. */
      err="Canceled\n";
      flush_results_queue(this);
      break;
    case CS_CANCELED:
      sybdebug((stderr,"Results canceled\n"));
/*       this->busy--; */
      flush_results_queue(this);
      break;
    default:; /* Ok */
    }

    if (err||done) break; /* get out of the while cycle */

  /* We should probably put this in a do..while cycle, so that we can
   * ignore STATUS results and such: only the first three carry useful
   * information..
   */
  /* we need to set the fetch-cycle up. It really looks like DMA to me.. */
    switch (rtype) {
    case CS_ROW_RESULT:
    case CS_CURSOR_RESULT:
    case CS_PARAM_RESULT:
      sybdebug((stderr,"Got useful results\n"));
      /* we're getting rows of data here. Also, I'm skipping a few command
         results, for command which should be pretty safe */
      ct_res_info(cmd,CS_NUMDATA,&numcols,CS_UNUSED,NULL);
      this->numcols=numcols;
      sybdebug((stderr,"I have %d columns' worth of data\n",numcols));
      sybdebug((stderr,"Allocating results**\n"));
      /* it looks like xalloc can't be used here. Hubbe? */
      results=(char**)malloc(numcols*sizeof(char*));
      results_lengths=(CS_INT*)malloc(numcols*sizeof(CS_INT));
      nulls=(CS_SMALLINT*)malloc(numcols*sizeof(CS_SMALLINT));
      this->results=results;
      this->results_lengths=results_lengths;
      this->nulls=nulls;
      
      /* these values are set for each column, since we're fetching
       * one row per cycle in fetch_row */
/*       description.datatype=CS_TEXT_TYPE; */
      description.datatype=CS_CHAR_TYPE; /* let's see if this works better */
      description.format=CS_FMT_NULLTERM;
      description.maxlength=MAX_RESULTS_SIZE-1;
      description.scale=CS_SRC_VALUE;
      description.precision=CS_SRC_VALUE;
      description.count=1;
      description.locale=NULL;

      for(j=0;j<numcols;j++) {
        INT32 length=guess_column_length(cmd,j+1);
        sybdebug((stderr,"beginning work on column %d (index %d)\n",j+1,j));
        sybdebug((stderr,"Allocating %ld+%d+1 bytes\n",
                 length,EXTRA_COLUMN_SPACE));
        /* it looks like xalloc can't be used here. Hubbe? */
        results[j]=(char*)malloc((length+EXTRA_COLUMN_SPACE+1)*
                                 sizeof(char));
        if (results[j]==NULL) {
          sybdebug((stderr,"PANIC! COULDN'T ALLOCATE MEMORY!"));
        }

        sybdebug((stderr,"Binding column %d\n",j+1));
        description.maxlength=length+EXTRA_COLUMN_SPACE; 
        /* maxlength used to be MAX_RESULTS_SIZE-1. Let's make sure we don't
         * goof*/
        ret=ct_bind(cmd,j+1,&description,results[j],
                    &(results_lengths[j]),&(nulls[j]));
        /* TODO: replace the first NULL with the length indicator */
        /* TODO: replace the second NULL with (SQL)NULL indicators */
        /* see page 3-10 */
        show_status(ret);
        if (FAILED(ret)) {
          err="Failed to bind column\n";
          break;
        }
      }
      toreturn=PS_RESULT_THIS;
      done=1;
      break;
    case CS_STATUS_RESULT:
      sybdebug((stderr,"Got status result. Retrieving\n"));
      do {
        INT32 length;
        CS_DATAFMT description;
        CS_INT rows_read;

        length=guess_column_length(cmd,1);

        sybdebug((stderr,"Guessed length of %d\n",length));
        function_result=(char*)malloc((length+EXTRA_COLUMN_SPACE+1)*
                                         sizeof(char));

        description.datatype=CS_CHAR_TYPE;
        description.format=CS_FMT_NULLTERM;
        description.maxlength=length+EXTRA_COLUMN_SPACE;
        description.scale=CS_SRC_VALUE;
        description.precision=CS_SRC_VALUE;
        description.count=1;
        description.locale=NULL;
        
        
        sybdebug((stderr,"Binding...\n"));
        ret=ct_bind(cmd,1,&description,function_result,NULL,NULL); 
        /* TODO: use binary strings */

        do {
          sybdebug((stderr,"Fetching\n"));
          ret=ct_fetch(cmd,CS_UNUSED,CS_UNUSED,CS_UNUSED,&rows_read);
          show_status(ret);
          sybdebug((stderr,"Got: %s\n",function_result));
        } while (OK(ret));
        
        
        
        toreturn=PS_RESULT_STATUS;
        
      } while (0);
      /* flush_results_queue(this); No need to. We'll loop in the while
       * cycle, and if there's some result we'll catch it elsewhere.
       */
      break;
    case CS_COMPUTE_RESULT:
/*       this->busy--; */
      err="Compute result is not supported\n";
      flush_results_queue(this);
      break;
    case CS_MSG_RESULT:
/*       this->busy--; */
      err="Message result is not supported\n";
      flush_results_queue(this);
      break;
    case CS_DESCRIBE_RESULT:
/*       this->busy--; */
      err="Describe result is not supported\n";
      flush_results_queue(this);
      break;
    case CS_ROWFMT_RESULT:
/*       this->busy--; */
      err="Row Format result is not supported\n";
      flush_results_queue(this);
      break;
    case CS_COMPUTEFMT_RESULT:
/*       this->busy--; */
      err="Compute Format result is not supported\n";
      flush_results_queue(this);
      break;
    case CS_CMD_DONE:
/*       this->busy--; */
      flush_results_queue(this);
      done=1;
      break;
    case CS_CMD_SUCCEED:
/*       this->busy--; */
      flush_results_queue(this);
      done=1;
      break;
    case CS_CMD_FAIL:
/*       this->busy--; */
      err="Command failed\n";
      flush_results_queue(this);
      break;
    }
  }
        
  sybdebug((stderr,"Busy status: %d\n",this->busy));

  SYB_UNLOCK(this->lock);
  THREADS_DISALLOW();

  if (err) {
    handle_errors(this);
    this->busy--;
    Pike_error(err);
  }

  pop_n_elems(args); /* moved from earlier. */

  /* we're surely done */
  switch (toreturn) {
  case PS_NORESULT:
    this->busy--;
    push_int(0);
    break;
  case PS_RESULT_THIS:
    ref_push_object(Pike_fp->current_object);
    break;
  case PS_RESULT_STATUS:
    this->busy--;
    push_text(function_result);
    free(function_result);
    function_result=NULL;
    break;
  default:
    Pike_fatal("Internal error! Wrong result in big_query\n");
    break;
  }
  /* extra safety check. Paranoia. */
  if (function_result) {
    sybdebug((stderr,"Internal error! Function_result!=NULL"));
    free(function_result);
  }
}


/*
 * The while cycle is supposed to be outside this function call, in the pike
 * code.
 */
/* void|array(mix) fetch_row() */
static void f_fetch_row(INT32 args) {
  pike_sybase_connection *this=THIS;
  CS_RETCODE ret;
  CS_COMMAND *cmd=this->cmd;
  CS_INT numread,j;
  char **results=this->results;
  CS_INT *results_lengths=this->results_lengths;
  CS_SMALLINT *nulls=this->nulls;
  int numcols=this->numcols;

  pop_n_elems(args);

  if (this->busy<=0)
    Pike_error("No pending results\n");

  THREADS_ALLOW();
  SYB_LOCK(this->lock);

  sybdebug((stderr,"Fetching row\n"));
  ret=ct_fetch(cmd,CS_UNUSED,CS_UNUSED,CS_UNUSED,&numread);
  show_status(ret);

  SYB_UNLOCK(this->lock);
  THREADS_DISALLOW();

  switch(ret) {
  case CS_SUCCEED:
    sybdebug((stderr,"Got row\n"));
    for(j=0;j<numcols;j++) {
      if (nulls[j] != -1) {
        /* !null */
        push_string(make_shared_binary_string(results[j],
                                              results_lengths[j]-1));
      } else {
        /* null */
        push_int(0);
      }
    }
    f_aggregate(numcols);
    return;
  case CS_END_DATA:
    sybdebug((stderr,"Got end of data\n"));
    flush_results_queue(this);
    push_int(0);
    this->busy--;
    sybdebug((stderr,"Busy status: %d\n",this->busy));
    return;
  case CS_ROW_FAIL:
    handle_errors(this);
    Pike_error("Recoverable error while fetching row\n");
    break;
  case CS_FAIL:
    handle_errors(this);
    ct_cancel(this->connection,cmd,CS_CANCEL_ALL);
    this->busy--;
    sybdebug((stderr,"Busy status: %d\n",this->busy));
    Pike_error("Unrecoverable error while fetching row\n");
    break;
  case CS_CANCELED:
    sybdebug((stderr,"Canceled\n"));
    push_int(0);
    this->busy--;
    sybdebug((stderr,"Busy status: %d\n",this->busy));
    return;
  case CS_PENDING:
  case CS_BUSY:
    Pike_error("Asynchronous operations are not supported\n");
    break;
  }
  Pike_error("Internal error. We shouldn't get here\n");
}

/* int num_fields() */
static void f_num_fields(INT32 args) {
  CS_INT cols;
  CS_RETCODE ret;
  char* err=NULL;
  pike_sybase_connection *this=THIS;

  pop_n_elems(args);
  if (this->busy<=0) {
    Pike_error("Issue a command first!\n");
  }

  THREADS_ALLOW();
  SYB_LOCK(this->lock);

  ret=ct_res_info(this->cmd,CS_NUMDATA,&cols,CS_UNUSED,NULL);
  if (FAILED(ret)) {
    err="Can't retrieve columns number information\n";
    handle_errors(this);
  }

  SYB_UNLOCK(this->lock);
  THREADS_DISALLOW();
  
  if (err) Pike_error(err);
  
  push_int(cols);
}

/* int affected_rows() */
static void f_affected_rows(INT32 args) {
  CS_INT rows;
  CS_RETCODE ret;
  char *err=NULL;
  pike_sybase_connection *this=THIS;

  pop_n_elems(args);
  
  THREADS_ALLOW();
  SYB_LOCK(this->lock);

  ret=ct_res_info(this->cmd,CS_ROW_COUNT,&rows,CS_UNUSED,NULL);
  if (FAILED(ret)) {
    err="Can't retrieve affected rows information\n";
    handle_errors(this);
  }

  SYB_UNLOCK(this->lock);
  THREADS_DISALLOW();
  
  if (err) Pike_error(err);
  
  push_int(rows);
}


#define desc_type(X,Y) case X: push_text(#Y);sybdebug((stderr,"type is %s\n",#Y)); break
static void f_fetch_fields(INT32 args) {
  pike_sybase_connection *this=THIS;
  int j, nflags, numcols=this->numcols;
  CS_DATAFMT descs[numcols], *desc;
  CS_RETCODE ret;
  char* err=NULL;

  if (this->busy<=0)
    Pike_error("You must issue a command first\n");

  pop_n_elems(args);

  THREADS_ALLOW();
  SYB_LOCK(this->lock);
  
  for (j=0;j<numcols;j++) {
    sybdebug((stderr,"Describing column %d\n",j+1));
    ret=ct_describe(this->cmd,j+1,&descs[j]);
    show_status(ret);
    if (FAILED(ret)) {
      err="Error while fetching descriptions\n";
      break;
    }
  }

  SYB_UNLOCK(this->lock);
  THREADS_DISALLOW();
  
  for(j=0;j<numcols;j++) {
    nflags=0;
    desc=&descs[j];
    push_text("name");
    push_text(desc->name);
    sybdebug((stderr,"name is %s\n",desc->name));
    push_text("type");
    switch(desc->datatype) {
      desc_type(CS_ILLEGAL_TYPE,illegal);
      desc_type(CS_CHAR_TYPE,char);
      desc_type(CS_BINARY_TYPE,bynary);
      desc_type(CS_LONGCHAR_TYPE,longchar);
      desc_type(CS_LONGBINARY_TYPE,longbinary);
      desc_type(CS_TEXT_TYPE,text);
      desc_type(CS_IMAGE_TYPE,image);
      desc_type(CS_TINYINT_TYPE,tinyint);
      desc_type(CS_SMALLINT_TYPE,smallint);
      desc_type(CS_INT_TYPE,integer);
      desc_type(CS_REAL_TYPE,real);
      desc_type(CS_FLOAT_TYPE,float);
      desc_type(CS_BIT_TYPE,bit);
      desc_type(CS_DATETIME_TYPE,datetime);
      desc_type(CS_DATETIME4_TYPE,datetime4);
      desc_type(CS_MONEY_TYPE,money);
      desc_type(CS_MONEY4_TYPE,money4);
      desc_type(CS_NUMERIC_TYPE,numeric);
      desc_type(CS_DECIMAL_TYPE,decimal);
      desc_type(CS_VARCHAR_TYPE,varchar);
      desc_type(CS_VARBINARY_TYPE,varbinary);
      desc_type(CS_LONG_TYPE,long);
      desc_type(CS_SENSITIVITY_TYPE,sensitivity);
      desc_type(CS_BOUNDARY_TYPE,boundary);
      desc_type(CS_VOID_TYPE,void);
      desc_type(CS_USHORT_TYPE,ushort);
    default:
      push_text("unknown");
    }
    push_text("max_length");
    push_int(desc->maxlength);
    sybdebug((stderr,"max_length is %d\n",desc->maxlength));

    push_text("flags");
    if (!(desc->status & CS_CANBENULL)) {
      sybdebug((stderr,"Flag: not null\n"));
      push_text("not_null");
      nflags++;
    }
    if (desc->status & CS_HIDDEN) {
      sybdebug((stderr,"Flag: hidden\n"));
      push_text("hidden");
      nflags++;
    }
    if (desc->status & CS_IDENTITY) {
      sybdebug((stderr,"Flag: identity\n"));
      push_text("identity");
      nflags++;
    }
    if (desc->status & CS_KEY) {
      sybdebug((stderr,"Flag: key\n"));
      push_text("key");
      nflags++;
    }
    if (desc->status & CS_VERSION_KEY) {
      sybdebug((stderr,"Flag: version_key\n"));
      push_text("version_key");
      nflags++;
    }
    if (desc->status & CS_TIMESTAMP) {
      sybdebug((stderr,"Flag: timestamp\n"));
      push_text("timestamp");
      nflags++;
    }
    if (desc->status & CS_UPDATABLE) {
      sybdebug((stderr,"Flag: updatable\n"));
      push_text("updatable");
      nflags++;
    }
    if (nflags) {
      sybdebug((stderr,"Aggregating flags: %d members\n",nflags));
      f_aggregate_multiset(nflags);
    } else {
      sybdebug((stderr,"No flags"));
      push_int(0);
    }
    
    f_aggregate_mapping(2*4);
  }

  sybdebug((stderr,"Aggregating columns: %d members\n",numcols));
  f_aggregate(numcols);

}


/********/
/* Glue */
/********/

static struct program* sybase_program;
PIKE_MODULE_EXIT {
  SYB_MT_EXIT(mainlock);
  if(sybase_program) free_program(sybase_program);
}

PIKE_MODULE_INIT {

  sybdebug((stderr,"sybase driver release " SYBASE_DRIVER_VERSION "\n"));

  start_new_program();
  ADD_STORAGE(pike_sybase_connection);
  set_init_callback(sybase_create);
  set_exit_callback(sybase_destroy);
  
#ifdef ADD_FUNCTION /* pike_0.7 */
  /* function(void|string,void|string,void|string,void|string,int|void:void) */
  ADD_FUNCTION("create",f_create,tFunc(tOr(tVoid,tStr) tOr(tVoid,tStr) 
                                       tOr(tVoid,tStr) tOr(tVoid,tStr)
                                       tOr(tInt,tVoid), tVoid),
               0);
  ADD_FUNCTION("connect",f_connect,tFunc(tOr(tVoid,tStr) tOr(tVoid,tStr) 
                                         tOr(tVoid,tStr) tOr(tVoid,tStr)
                                         tOr(tInt,tVoid), tVoid),
               0);
  ADD_FUNCTION("error",f_error,tFunc(tVoid,tOr(tVoid,tStr)),
	       0);

  ADD_FUNCTION("big_query",f_big_query,tFunc(tString,tOr(tInt,tObj)),
	       0);

  ADD_FUNCTION("fetch_row", f_fetch_row,
               tFunc(tVoid,tOr(tVoid,tArr(tMix))), 0);

  ADD_FUNCTION("num_fields", f_num_fields,
               tFunc(tVoid,tInt), 0);

  ADD_FUNCTION("affected_rows", f_affected_rows,
               tFunc(tVoid,tInt), 0);

  ADD_FUNCTION("fetch_fields", f_fetch_fields,
               tFunc(tVoid,tArr(tOr(tInt,tMap(tStr,tMix)))), ID_PUBLIC);
#else
  add_function("create",f_create,
               "function(void|string,void|string,void|string,void|string,int|void:void)",
               0);
  add_function("connect",f_connect,
               "function(void|string,void|string,void|string,void|string,int|void:void)",
               0);
  add_function("error",f_error,"function(void:void|string)",OPT_RETURN);
  add_function("big_query",f_big_query,"function(string:void|object)",
               OPT_RETURN);
  add_function("fetch_row", f_fetch_row,"function(void:void|array(mixed))",
               OPT_RETURN);
  add_function("num_fields", f_num_fields, "function(void:int)",0);
  add_function("affected_rows",f_affected_rows,"function(void:int)",0);
  add_function("fetch_fields",f_fetch_fields,
               "function(void:array(int|mapping(string:mixed)))",
               0);
#endif

  /* TODO */
  /* int num_rows() */ /* looks like sybase doesn't support this one.. */
  /* void seek(int skip) */ /*implemented in pike. Inefficient but simple */
  /* mapping* fetch_fields() */

  sybase_program=end_program();
  add_program_constant("sybase",sybase_program,0);
  
  SYB_MT_INIT(mainlock);
}


#else /* HAVE_SYBASE */
/* This must be included last! */
#include "module.h"

PIKE_MODULE_INIT {}
PIKE_MODULE_EXIT {}
#endif /* HAVE_SYBASE */
