/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/*
 * Pike embedding API.
 *
 * Henrik Grubbstr�m 2004-12-27
 */

#include "module.h"
#include "fdlib.h"
#include "backend.h"
#include "lex.h"
#include "pike_types.h"
#include "threads.h"
#include "builtin_functions.h"
#include "interpret.h"
#include "pike_error.h"
#include "pike_macros.h"
#include "callback.h"
#include "signal_handler.h"
#include "dynamic_load.h"
#include "gc.h"
#include "cpp.h"
#include "main.h"
#include "operators.h"
#include "rbtree.h"
#include "constants.h"
#include "version.h"
#include "pike_rusage.h"
#include "module_support.h"
#include "opcodes.h"
#include "pike_cpulib.h"
#include "pike_embed.h"
#include "pike_modules.h"

#if defined(__linux__) && defined(HAVE_DLOPEN) && defined(HAVE_DLFCN_H)
#include <dlfcn.h>
#endif

#include "las.h"

#include <errno.h>
#include <locale.h>

#include "time_stuff.h"

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#ifdef TRY_USE_MMX
#ifdef HAVE_MMX_H
#include <mmx.h>
#else
#include <asm/mmx.h>
#endif
int try_use_mmx;
#endif


/* Define this to trace the execution of main(). */
/* #define TRACE_MAIN */

/* Define this for extra C-stack debug. */
/* #define STACK_DEBUG */

#ifdef PIKE_EXTRA_DEBUG
#define TRACE_MAIN
#define STACK_DEBUG
#endif

#ifdef TRACE_MAIN
#define TRACE(X)	fprintf X
#else /* !TRACE_MAIN */
#define TRACE(X)
#endif /* TRACE_MAIN */

PMOD_EXPORT int debug_options=0;
PMOD_EXPORT int runtime_options=0;
PMOD_EXPORT int d_flag=0;
PMOD_EXPORT int c_flag=0;
PMOD_EXPORT int default_t_flag=0;
PMOD_EXPORT int a_flag=0;
PMOD_EXPORT int l_flag=0;
PMOD_EXPORT int p_flag=0;

int set_pike_debug_options(int bits, int mask)
{
  return debug_options = (debug_options & ~mask) | (bits & mask);
}

int set_pike_runtime_options(int bits, int mask)
{
  return runtime_options = (runtime_options & ~mask) | (bits & mask);
}

char **ARGV;
const char *master_file = NULL;

void init_pike(char **argv, const char *file)
{
  init_pike_memory();

  init_rusage();

  /* Attempt to make sure stderr is unbuffered. */
  setvbuf(stderr, NULL, _IONBF, 0);

#ifdef TRY_USE_MMX
  TRACE((stderr, "Init MMX...\n"));
  try_use_mmx=mmx_ok();
#endif

#ifdef OWN_GETHRTIME
/* initialize our own gethrtime conversion /Mirar */
  TRACE((stderr, "Init gethrtime...\n"));
  own_gethrtime_init();
#endif

  ARGV=argv;
  master_file = file;

  TRACE((stderr, "Main init...\n"));
  fd_init();
  {
    extern void init_node_s_blocks(void);
    init_node_s_blocks();
    init_multiset();
    init_builtin_constants();
  }

#ifdef HAVE_TZSET
  tzset();
#endif /* HAVE_TZSET */

#ifdef LC_NUMERIC
  setlocale(LC_NUMERIC, "C");
#endif
#ifdef LC_CTYPE
  setlocale(LC_CTYPE, "");
#endif
#ifdef LC_TIME
  setlocale(LC_TIME, "C");
#endif
#ifdef LC_COLLATE
  setlocale(LC_COLLATE, "");
#endif
#ifdef LC_MESSAGES
  setlocale(LC_MESSAGES, "");
#endif
}

static void (*pike_exit_cb)(int);

void init_pike_runtime(void (*exit_cb)(int))
{
  JMP_BUF back;
  int num;

  pike_exit_cb = exit_cb;

  TRACE((stderr, "Init C stack...\n"));

  Pike_interpreter.stack_top = (char *)&exit_cb;

  /* Adjust for anything already pushed on the stack.
   * We align on a 64 KB boundary.
   * Thus we at worst, lose 64 KB stack.
   *
   * We have to do it this way since some compilers don't like
   * & and | on pointers, and casting to an integer type is
   * too unsafe (consider 64-bit systems).
   */
#if STACK_DIRECTION < 0
  /* Equivalent with |= 0xffff */
  Pike_interpreter.stack_top += ~(PTR_TO_INT(Pike_interpreter.stack_top)) & 0xffff;
#else /* STACK_DIRECTION >= 0 */
  /* Equivalent with &= ~0xffff */
  Pike_interpreter.stack_top -= PTR_TO_INT(Pike_interpreter.stack_top) & 0xffff;
#endif /* STACK_DIRECTION < 0 */

#ifdef PROFILING
  Pike_interpreter.stack_bottom=Pike_interpreter.stack_top;
#endif

#if defined(HAVE_GETRLIMIT) && defined(RLIMIT_STACK)
  {
    struct rlimit lim;
    if(!getrlimit(RLIMIT_STACK, &lim))
    {
#ifdef RLIM_INFINITY
      if(lim.rlim_cur == (rlim_t)RLIM_INFINITY)
	lim.rlim_cur=1024*1024*32;
#endif

#ifdef Pike_INITIAL_STACK_SIZE
      if(lim.rlim_cur > Pike_INITIAL_STACK_SIZE)
	lim.rlim_cur=Pike_INITIAL_STACK_SIZE;
#endif

#if defined(__linux__) && defined(PIKE_THREADS)
      /* This is a really really *stupid* limit in glibc 2.x
       * which is not detectable since __pthread_initial_thread_bos
       * went static. On a stupidity-scale from 1-10, this rates a
       * solid 11. - Hubbe
       */
      if(lim.rlim_cur > 2*1024*1024) lim.rlim_cur=2*1024*1024;
#endif

#if STACK_DIRECTION < 0
      Pike_interpreter.stack_top -= lim.rlim_cur;
#else /* STACK_DIRECTION >= 0 */
      Pike_interpreter.stack_top += lim.rlim_cur;
#endif /* STACK_DIRECTION < 0 */

#if defined(__linux__) && defined(HAVE_DLOPEN) && defined(HAVE_DLFCN_H) && !defined(PPC)
      {
	char ** bos_location;
	void *handle;
	/* damn this is ugly -Hubbe */
	if((handle=dlopen(0, RTLD_LAZY)))
	{
	  bos_location=dlsym(handle,"__pthread_initial_thread_bos");

	  if(bos_location && *bos_location &&
	     (*bos_location - Pike_interpreter.stack_top) *STACK_DIRECTION < 0)
	  {
	    Pike_interpreter.stack_top=*bos_location;
	  }

	  dlclose(handle);
	}
      }
#else
#ifdef HAVE_PTHREAD_INITIAL_THREAD_BOS
      {
	extern char * __pthread_initial_thread_bos;
	/* Linux glibc threads are limited to a 4 Mb stack
	 * __pthread_initial_thread_bos is the actual limit
	 */

	if(__pthread_initial_thread_bos &&
	   (__pthread_initial_thread_bos - Pike_interpreter.stack_top) *STACK_DIRECTION < 0)
	{
	  Pike_interpreter.stack_top=__pthread_initial_thread_bos;
	}
      }
#endif /* HAVE_PTHREAD_INITIAL_THREAD_BOS */
#endif /* __linux__ && HAVE_DLOPEN && HAVE_DLFCN_H && !PPC*/

#if STACK_DIRECTION < 0
      Pike_interpreter.stack_top += 8192 * sizeof(char *);
#else /* STACK_DIRECTION >= 0 */
      Pike_interpreter.stack_top -= 8192 * sizeof(char *);
#endif /* STACK_DIRECTION < 0 */


#ifdef STACK_DEBUG
      fprintf(stderr, "1: C-stack: 0x%08p - 0x%08p, direction:%d\n",
	      &exit_cb, Pike_interpreter.stack_top, STACK_DIRECTION);
#endif /* STACK_DEBUG */
    }
  }
#else /* !HAVE_GETRLIMIT || !RLIMIT_STACK */
  /* 128 MB seems a bit extreme, most OS's seem to have their limit at ~8MB */
  Pike_interpreter.stack_top += STACK_DIRECTION * (1024*1024 * 8 - 8192 * (ptrdiff_t) sizeof(char *));
#ifdef STACK_DEBUG
  fprintf(stderr, "2: C-stack: 0x%08p - 0x%08p, direction:%d\n",
	  &exit_cb, Pike_interpreter.stack_top, STACK_DIRECTION);
#endif /* STACK_DEBUG */
#endif /* HAVE_GETRLIMIT && RLIMIT_STACK */

  TRACE((stderr, "Init time...\n"));
  UPDATE_CURRENT_TIME();

  TRACE((stderr, "Init threads...\n"));
  low_th_init();

  TRACE((stderr, "Init strings...\n"));
  init_shared_string_table();

  TRACE((stderr, "Init las...\n"));
  init_las();

  TRACE((stderr, "Init interpreter...\n"));
  init_interpreter();

  TRACE((stderr, "Init types...\n"));
  init_types();

  TRACE((stderr, "Init opcodes...\n"));
  init_opcodes();

  TRACE((stderr, "Init destruct...\n"));
  low_init_object();

  TRACE((stderr, "Init programs...\n"));
  init_program();

  TRACE((stderr, "Init objects...\n"));
  init_object();

  if(SETJMP(back))
  {
    if(throw_severity == THROW_EXIT)
    {
      num=throw_value.u.integer;
    }else{
      call_handle_error();
      num=10;
    }
    UNSETJMP(back);

    pike_do_exit(num);
  } else {
    back.severity=THROW_EXIT;

    TRACE((stderr, "Init modules...\n"));
    init_modules();

#ifdef TEST_MULTISET
    /* A C-level testsuite for the low level stuff in multisets. */
    test_multiset();
#endif
    UNSETJMP(back);
  }
}

/*
 * Support for limited number of instructions.
 */

/* FIXME: Thread specific limit? */
static unsigned long instructions_left = 0;

static void time_to_exit(struct callback *UNUSED(cb), void *UNUSED(tmp), void *UNUSED(ignored))
{
  if(!(instructions_left--))
  {
    /* FIXME: Special THROW_* type? */
    push_int(0);
    f_exit(1);
  }
}

void set_pike_evaluator_limit(unsigned long num_instrs)
{
  if (!num_instrs) return;
  if (!instructions_left) {
    add_to_callback(&evaluator_callbacks, time_to_exit, 0, 0);
  }
  instructions_left += num_instrs;
}

static struct callback_list post_master_callbacks;

PMOD_EXPORT struct callback *add_post_master_callback(callback_func call,
						      void *arg,
						      callback_func free_func)
{
  return add_to_callback(&post_master_callbacks, call, arg, free_func);
}

struct object *load_pike_master(void)
{
  struct object *m;
  TRACE((stderr, "Init master...\n"));

  if ((m = master())) {
    call_callback(& post_master_callbacks, 0);
    free_callback_list(& post_master_callbacks);
  }
  return m;
}

#ifdef PROFILING
#ifdef PIKE_DEBUG
void gdb_break_on_pike_stack_record(long stack_size)
{
  ;
}
#endif

static unsigned int samples[8200];
long record;

static void sample_stack(struct callback *cb,void *UNUSED(tmp),void *UNUSED(ignored))
{
  long stack_size=( ((char *)&cb) - Pike_interpreter.stack_bottom) * STACK_DIRECTION;
  stack_size>>=10;
  stack_size++;
  if(stack_size<0) stack_size=0;
  if(stack_size >= (long)NELEM(samples)) stack_size=NELEM(samples)-1;
  samples[stack_size]++;
#ifdef PIKE_DEBUG
  if(stack_size > record)
  {
    gdb_break_on_pike_stack_record(stack_size);
    record=stack_size;
  }
#endif
}

#endif

void pike_enable_stack_profiling(void)
{
#ifdef PROFILING
  add_to_callback(&evaluator_callbacks, sample_stack, 0, 0);
#endif
}

static struct callback_list exit_callbacks;

PMOD_EXPORT struct callback *add_exit_callback(callback_func call,
				   void *arg,
				   callback_func free_func)
{
  return add_to_callback(&exit_callbacks, call, arg, free_func);
}

void pike_do_exit(int num)
{
  /* Add a frame so that backtraces generated in this stage
   * (from eg an atexit() function) can be easily identified.
   */
  struct pike_frame *new_frame = alloc_pike_frame();
#ifdef PROFILING
  new_frame->children_base = Pike_interpreter.accounted_time;
  new_frame->start_time = get_cpu_time() - Pike_interpreter.unlocked_time;
  W_PROFILING_DEBUG("%p{: Push at %" PRINT_CPU_TIME
		    " %" PRINT_CPU_TIME "\n",
		    Pike_interpreter.thread_state, new_frame->start_time,
		    new_frame->children_base);
#endif
  new_frame->next = Pike_fp;
  new_frame->pc = (PIKE_OPCODE_T *)pike_do_exit;
  Pike_fp = new_frame;

  call_callback(&exit_callbacks, NULL);
  free_callback_list(&exit_callbacks);

  POP_PIKE_FRAME();

  exit_las();

  exit_modules();

  exit_pike_memory();

#ifdef PROFILING
  {
    int q;
    for(q=0;q<(long)NELEM(samples);q++)
      if(samples[q])
	fprintf(stderr,"STACK WAS %4d Kb %12u times\n",q-1,samples[q]);
  }
#endif

#ifdef PIKE_DEBUG
  /* For profiling */
  exit_opcodes();
#endif

#ifdef INTERNAL_PROFILING
  fprintf (stderr, "Evaluator callback calls: %lu\n", evaluator_callback_calls);
#ifdef PIKE_THREADS
  fprintf (stderr, "Thread yields: %lu\n", thread_yields);
#endif
  fprintf (stderr, "Main thread summary:\n");
  debug_print_rusage (stderr);
#endif

#ifdef PIKE_DEBUG
  if (TYPEOF(svalue_int_zero) != T_INT ||
      SUBTYPEOF(svalue_int_zero) != NUMBER_NUMBER ||
      svalue_int_zero.u.integer != 0)
    Pike_fatal ("svalue_int_zero has been changed.\n");
  if (TYPEOF(svalue_undefined) != T_INT ||
      SUBTYPEOF(svalue_undefined) != NUMBER_UNDEFINED ||
      svalue_undefined.u.integer != 0)
    Pike_fatal ("svalue_undefined has been changed.\n");
  if (TYPEOF(svalue_int_one) != T_INT ||
      SUBTYPEOF(svalue_int_one) != NUMBER_NUMBER ||
      svalue_int_one.u.integer != 1)
    Pike_fatal ("svalue_int_one has been changed.\n");
#endif

  pike_exit_cb(num);
}

/* Some helper functions
 */

void pike_push_argv(int argc, char **argv)
{
  struct array *a;
  int num;
  a=allocate_array_no_init(argc,0);
  for(num=0;num<argc;num++)
  {
    SET_SVAL(ITEM(a)[num], T_STRING, 0, string, make_shared_string(argv[num]));
  }
  a->type_field = BIT_STRING;
  push_array(a);
}
