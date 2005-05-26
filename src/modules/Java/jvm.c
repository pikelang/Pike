/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: jvm.c,v 1.77 2005/05/26 12:41:27 grubba Exp $
*/

/*
 * Pike interface to Java Virtual Machine
 *
 * Marcus Comstedt
 */

/*
 * Includes
 */

#define NO_PIKE_SHORTHAND

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "global.h"
#include "program.h"
#include "interpret.h"
#include "stralloc.h"
#include "object.h"
#include "builtin_functions.h"
#include "pike_error.h"
#include "module_support.h"
#include "pike_memory.h"
#include "gc.h"
#include "threads.h"
#include "operators.h"

#ifdef HAVE_JAVA

#include <stdarg.h>
#include <locale.h>

#ifdef HAVE_JNI_H
#include <jni.h>
#endif /* HAVE_JNI_H */

#ifdef HAVE_WINBASE_H
#include <winbase.h>
#endif /* HAVE_WINBASE_H */

#ifdef _REENTRANT
#if defined(HAVE_SPARC_CPU) || defined(HAVE_X86_CPU) || defined(HAVE_PPC_CPU) \
 || defined(HAVE_ALPHA_CPU)
#define SUPPORT_NATIVE_METHODS
#endif /* HAVE_SPARC_CPU || HAVE_X86_CPU || HAVE_PPC_CPU || HAVE_ALPHA_CPU */
#endif /* _REENTRANT */

#ifdef __NT__
#include "ntdl.c"
#endif /* __NT___ */

static struct program *jvm_program = NULL;
static struct program *jobj_program = NULL, *jclass_program = NULL;
static struct program *jthrowable_program = NULL, *jarray_program = NULL;
static struct program *method_program = NULL, *static_method_program = NULL;
static struct program *field_program = NULL, *static_field_program = NULL;
static struct program *natives_program = NULL, *attachment_program = NULL;
static struct program *monitor_program = NULL;
static SIZE_T jarray_stor_offs = 0;

struct jvm_storage {
  JavaVM *jvm;			/* Denotes a Java VM */
  JNIEnv *env;			/* pointer to native method interface */
  JavaVMInitArgs vm_args;       /* JDK 1.2 VM initialization arguments */
  JavaVMOption vm_options[4];	/* Should be large enough to hold all opts. */
  struct pike_string *classpath_string;
  jclass class_object, class_class, class_string, class_throwable;
  jclass class_runtimex, class_system;
  jmethodID method_hash, method_tostring, method_arraycopy;
  jmethodID method_getcomponenttype, method_isarray;
  jmethodID method_getname, method_charat;
#ifdef _REENTRANT
  struct object *tl_env;
#endif /* _REENTRANT */
};

struct jobj_storage {
  struct object *jvm;
  jobject jobj;
};

struct jarray_storage {
  int ty;
};

struct method_storage {
  struct object *class;
  struct pike_string *name, *sig;
  jmethodID method;
  INT32 nargs;
  char rettype, subtype;
};

struct field_storage {
  struct object *class;
  struct pike_string *name, *sig;
  jfieldID field;
  char type, subtype;
};

struct monitor_storage {
  struct object *obj;
#ifdef _REENTRANT
  THREAD_T tid;
#endif /* _REENTRANT */
};

#ifdef _REENTRANT

struct att_storage {
  struct object *jvm;
  struct svalue thr;
  JavaVMAttachArgs args;
  JNIEnv *env;
  THREAD_T tid;
};

#endif /* _REENTRANT */


#define THIS_JVM ((struct jvm_storage *)(Pike_fp->current_storage))
#define THAT_JOBJ(o) ((struct jobj_storage *)get_storage((o),jobj_program))
#define THIS_JOBJ ((struct jobj_storage *)(Pike_fp->current_storage))
#define THIS_JARRAY ((struct jarray_storage *)(Pike_fp->current_storage+jarray_stor_offs))
#define THIS_METHOD ((struct method_storage *)(Pike_fp->current_storage))
#define THIS_FIELD ((struct field_storage *)(Pike_fp->current_storage))
#define THIS_MONITOR ((struct monitor_storage *)(Pike_fp->current_storage))

#define THIS_NATIVES ((struct natives_storage *)(Pike_fp->current_storage))
#ifdef _REENTRANT
#define THIS_ATT ((struct att_storage *)(Pike_fp->current_storage))
#endif /* _REENTRANT */

/*

TODO(?):

*Reflected*

Array stuff

array input to make_jargs

 */



/* Attach foo */

static JNIEnv *jvm_procure_env(struct object *jvm)
{
  struct jvm_storage *j =
    (struct jvm_storage *)get_storage(jvm, jvm_program);
  if(j) {

#ifdef _REENTRANT
    void *env;

    if(JNI_OK == (*j->jvm)->GetEnv(j->jvm, &env, JNI_VERSION_1_2)) {
      return (JNIEnv *)env;
    }

    if(j->tl_env != NULL && j->tl_env->prog != NULL) {
      safe_apply(j->tl_env, "get", 0);
      if(Pike_sp[-1].type != PIKE_T_OBJECT)
	pop_n_elems(1);
      else {
	env = ((struct att_storage *)((Pike_sp[-1].u.object)->storage))->env;
	pop_n_elems(1);
	return (JNIEnv *)env;
      }
    }

    ref_push_object(jvm);
    push_object(clone_object(attachment_program, 1));
    if(Pike_sp[-1].type != PIKE_T_OBJECT || Pike_sp[-1].u.object == NULL) {
      pop_n_elems(1);
      return NULL;
    }

    env = ((struct att_storage *)((Pike_sp[-1].u.object)->storage))->env;

    if(j->tl_env != NULL && j->tl_env->prog != NULL)
      safe_apply(j->tl_env, "set", 1);

    pop_n_elems(1);
    return (JNIEnv *)env;
#else
    return j->env;
#endif /* _REENTRANT */

  } else
    return NULL;
}

static void jvm_vacate_env(struct object *jvm, JNIEnv *env)
{
}


/* Global object references */

static void push_java_class(jclass c, struct object *jvm, JNIEnv *env)
{
  struct object *oo;
  struct jobj_storage *jo;
  jobject c2;

  if(!c) {
    push_int(0);
    return;
  }
  c2 = (*env)->NewGlobalRef(env, c);
  (*env)->DeleteLocalRef(env, c);
  push_object(oo=clone_object(jclass_program, 0));
  jo = (struct jobj_storage *)(oo->storage);
  jo->jvm = jvm;
  jo->jobj = c2;
  add_ref(jvm);
}

static void push_java_throwable(jthrowable t, struct object *jvm, JNIEnv *env)
{
  struct object *oo;
  struct jobj_storage *jo;
  jobject t2;

  if(!t) {
    push_int(0);
    return;
  }
  t2 = (*env)->NewGlobalRef(env, t);
  (*env)->DeleteLocalRef(env, t);
  push_object(oo=clone_object(jthrowable_program, 0));
  jo = (struct jobj_storage *)(oo->storage);
  jo->jvm = jvm;
  jo->jobj = t2;
  add_ref(jvm);
}

static void push_java_array(jarray t, struct object *jvm, JNIEnv *env, int ty)
{
  struct object *oo;
  struct jobj_storage *jo;
  struct jarray_storage *a;
  jobject t2;

  if(!t) {
    push_int(0);
    return;
  }
  t2 = (*env)->NewGlobalRef(env, t);
  (*env)->DeleteLocalRef(env, t);
  push_object(oo=clone_object(jarray_program, 0));
  jo = (struct jobj_storage *)(oo->storage);
  jo->jvm = jvm;
  jo->jobj = t2;
  a = (struct jarray_storage *)(oo->storage+jarray_stor_offs);
  a->ty = ty;
  add_ref(jvm);
}

static void push_java_anyobj(jobject o, struct object *jvm, JNIEnv *env)
{
  struct object *oo;
  struct jobj_storage *jo;
  struct jvm_storage *j =
    (struct jvm_storage *)get_storage(jvm, jvm_program);
  jobject o2;

  if((!j)||(!o)) {
    push_int(0);
    return;
  }
  o2 = (*env)->NewGlobalRef(env, o);
  (*env)->DeleteLocalRef(env, o);
  if((*env)->IsInstanceOf(env, o2, j->class_class))
    push_object(oo=clone_object(jclass_program, 0));
  else if((*env)->IsInstanceOf(env, o2, j->class_throwable))
    push_object(oo=clone_object(jthrowable_program, 0));
  else {
    jclass cc = (*env)->GetObjectClass(env, o2);
    if((*env)->CallBooleanMethod(env, cc, j->method_isarray)) {
      jstring ets = (jstring)(*env)->CallObjectMethod(env, cc,
						      j->method_getname);
      push_object(oo=clone_object(jarray_program, 0));
      ((struct jarray_storage *)(oo->storage+jarray_stor_offs))->ty =
	(*env)->CallCharMethod(env, ets, j->method_charat, 1);
      (*env)->DeleteLocalRef(env, ets);
    } else
      push_object(oo=clone_object(jobj_program, 0));
    (*env)->DeleteLocalRef(env, cc);
  }
  jo = (struct jobj_storage *)(oo->storage);
  jo->jvm = jvm;
  jo->jobj = o2;
  add_ref(jvm);
}

static void init_jobj_struct(struct object *o)
{
  struct jobj_storage *j = THIS_JOBJ;
  j->jvm = NULL;
  j->jobj = 0;
}

static void exit_jobj_struct(struct object *o)
{
  JNIEnv *env;
  struct jobj_storage *j = THIS_JOBJ;
  if(j->jvm) {
    if(j->jobj && (env = jvm_procure_env(j->jvm)) != NULL) {
      (*env)->DeleteGlobalRef(env, j->jobj);
      jvm_vacate_env(j->jvm, env);
    }
    free_object(j->jvm);
  }
}

static void jobj_gc_check(struct object *o)
{
  struct jobj_storage *j = THIS_JOBJ;

  if(j->jvm)
    gc_check(j->jvm);
}

static void jobj_gc_recurse(struct object *o)
{
  struct jobj_storage *j = THIS_JOBJ;

  if(j->jvm)
    gc_recurse_object(j->jvm);
}

static void f_jobj_cast(INT32 args)
{
  struct jobj_storage *jo = THIS_JOBJ;
  struct jvm_storage *j =
    (struct jvm_storage *)get_storage(jo->jvm, jvm_program);
  JNIEnv *env;
  jstring jstr;

  if(args < 1)
    Pike_error("cast() called without arguments.\n");
  if(Pike_sp[-args].type != PIKE_T_STRING)
    Pike_error("Bad argument 1 to cast().\n");

  if(!strcmp(Pike_sp[-args].u.string->str, "object")) {
    pop_n_elems(args);
    push_object(this_object());
  }

  if(strcmp(Pike_sp[-args].u.string->str, "string"))
    Pike_error("cast() to other type than string.\n");

  pop_n_elems(args);
  if((env=jvm_procure_env(jo->jvm))) {
    jstr = (*env)->CallObjectMethod(env, jo->jobj, j->method_tostring);
    if(jstr) {
      jsize l;
      const jchar *wstr;

      wstr = (*env)->GetStringChars(env, jstr, NULL);
      l = (*env)->GetStringLength(env, jstr);
      push_string(make_shared_binary_string1((p_wchar1 *)wstr, l));
      (*env)->ReleaseStringChars(env, jstr, wstr);
      (*env)->DeleteLocalRef(env, jstr);
    } else
      Pike_error("cast() to string failed.\n");
    jvm_vacate_env(jo->jvm, env);
  } else
    Pike_error("cast() to string failed (no JNIEnv).\n");
}

static void f_jobj_eq(INT32 args)
{
  struct jobj_storage *jo2, *jo = THIS_JOBJ;
  JNIEnv *env;
  jboolean res;

  if(args<1 || Pike_sp[-args].type != PIKE_T_OBJECT || 
     (jo2 = (struct jobj_storage *)get_storage(Pike_sp[-args].u.object,
					       jobj_program))==NULL) {
    pop_n_elems(args);
    push_int(0);
    return;
  }

  if((env=jvm_procure_env(jo->jvm))) {
    res = (*env)->IsSameObject(env, jo->jobj, jo2->jobj);
    jvm_vacate_env(jo->jvm, env);
  } else
    res = 0;

  pop_n_elems(args);
  push_int((res? 1:0));
}

static void f_jobj_hash(INT32 args)
{
  struct jobj_storage *jo = THIS_JOBJ;
  JNIEnv *env;
  struct jvm_storage *j =
    (struct jvm_storage *)get_storage(jo->jvm, jvm_program);

  pop_n_elems(args);
  if((env=jvm_procure_env(jo->jvm))) {
    push_int((*env)->CallIntMethod(env, jo->jobj, j->method_hash));
    jvm_vacate_env(jo->jvm, env);
  }
  else push_int(0);
}

static void f_jobj_instance(INT32 args)
{
  struct jobj_storage *c, *jo = THIS_JOBJ;
  JNIEnv *env;
  struct jvm_storage *j =
    (struct jvm_storage *)get_storage(jo->jvm, jvm_program);
  struct object *cls;
  int n=0;

  get_all_args("Java.obj->is_instance_of()", args, "%o", &cls);

  if((c = (struct jobj_storage *)get_storage(cls, jclass_program)) == NULL)
    Pike_error("Bad argument 1 to is_instance_of().\n");

  if((env=jvm_procure_env(jo->jvm))) {
    if((*env)->IsInstanceOf(env, jo->jobj, c->jobj))
      n = 1;
    jvm_vacate_env(jo->jvm, env);
  }

  pop_n_elems(args);
  push_int(n);
}

static void f_monitor_enter(INT32 args)
{
  struct jobj_storage *jo = THIS_JOBJ;
  JNIEnv *env;
  struct jvm_storage *j =
    (struct jvm_storage *)get_storage(jo->jvm, jvm_program);

  pop_n_elems(args);
  if((env=jvm_procure_env(jo->jvm))) {
    jint res = (*env)->MonitorEnter(env, jo->jobj);
    jvm_vacate_env(jo->jvm, env);
    if(res)
      push_int(0);
    else {
      push_object(this_object());
      push_object(clone_object(monitor_program, 1));
    }
  }
  else push_int(0);
}

static void f_jobj_get_class(INT32 args)
{
  struct jobj_storage *jo = THIS_JOBJ;
  JNIEnv *env;
  struct jvm_storage *j =
    (struct jvm_storage *)get_storage(jo->jvm, jvm_program);

  pop_n_elems(args);
  if((env=jvm_procure_env(jo->jvm))) {
    push_java_class((*env)->GetObjectClass(env, jo->jobj), jo->jvm, env);
    jvm_vacate_env(jo->jvm, env);
  }
  else push_int(0);
}


/* Methods */

static void init_method_struct(struct object *o)
{
  struct method_storage *m=THIS_METHOD;

  m->class = NULL;
  m->name = NULL;
  m->sig = NULL;
}

static void exit_method_struct(struct object *o)
{
  struct method_storage *m=THIS_METHOD;

  if(m->sig != NULL)
    free_string(m->sig);
  if(m->name != NULL)
    free_string(m->name);
  if(m->class != NULL)
    free_object(m->class);
}

static void method_gc_check(struct object *o)
{
  struct method_storage *m = THIS_METHOD;

  if(m->class)
    gc_check(m->class);
}

static void method_gc_recurse(struct object *o)
{
  struct method_storage *m = THIS_METHOD;

  if(m->class)
    gc_recurse_object(m->class);
}

static void f_method_create(INT32 args)
{
  struct method_storage *m=THIS_METHOD;
  struct jobj_storage *c;
  struct object *class;
  struct pike_string *name, *sig;
  JNIEnv *env;
  char *p;

  get_all_args("Java.method->create()", args, "%S%S%o", &name, &sig, &class);

  if((c = (struct jobj_storage *)get_storage(class, jclass_program)) == NULL)
    Pike_error("Bad argument 3 to create().\n");

  if((env = jvm_procure_env(c->jvm))==NULL) {
    pop_n_elems(args);
    destruct(Pike_fp->current_object);
    return;
  }

  m->method = (Pike_fp->current_object->prog==static_method_program?
	       (*env)->GetStaticMethodID(env, c->jobj, name->str, sig->str):
	       (*env)->GetMethodID(env, c->jobj, name->str, sig->str));

  jvm_vacate_env(c->jvm, env);

  if(m->method == 0) {
    pop_n_elems(args);
    destruct(Pike_fp->current_object);
    return;
  }

  m->class = class;
  copy_shared_string(m->name, name);
  copy_shared_string(m->sig, sig);
  add_ref(class);
  pop_n_elems(args);
  push_int(0);

  m->nargs = 0;
  m->rettype = 0;

  p = sig->str;
  if(*p++ != '(')
    return;

  while(*p && *p != ')') {
    if(*p != '[')
      m->nargs ++;
    if(*p++ == 'L')
      while(*p && *p++ != ';');
  }
  if(*p)
    if((m->rettype = *++p)=='[')
      m->subtype = *++p;
}

static void jargs_error(struct object *jvm, JNIEnv *env)
{
  jvm_vacate_env(jvm, env);
  Pike_error("incompatible types passed to method.\n");
}

static void make_jargs(jvalue *jargs, INT32 args, char *dorelease, char *sig,
		       struct object *jvm, JNIEnv *env)
{
  INT32 i;
  struct jobj_storage *jo;

  if(args==-1)
      args=1;
  else
    if(*sig=='(')
      sig++;
    else
      return;
  for(i=0; i<args; i++) {
    struct svalue *sv = &Pike_sp[i-args];
    dorelease && (*dorelease = 0);
    switch(sv->type) {
    case PIKE_T_INT:
      switch(*sig++) {
      case 'L':
	if(i+1<args)
	  while(*sig && *sig++!=';');
	if(sv->u.integer!=0)
	  jargs_error(jvm, env);
	jargs->l = 0;
	break;
      case '[':
	if(i+1<args) {
	  while(*sig=='[') sig++;
	  if(*sig && *sig++=='L')
	    while(*sig && *sig++!=';');
	}
	if(sv->u.integer!=0)
	  jargs_error(jvm, env);
	jargs->l = 0;
	break;
      case 'Z':
	jargs->z = sv->u.integer!=0;
	break;
      case 'B':
	jargs->b = sv->u.integer;
	break;
      case 'C':
	jargs->c = sv->u.integer;
	break;
      case 'S':
	jargs->s = sv->u.integer;
	break;
      case 'I':
	jargs->i = sv->u.integer;
	break;
      case 'J':
	jargs->j = sv->u.integer;
	break;
      case 'F':
	jargs->f = DO_NOT_WARN((float)sv->u.integer);
	break;
      case 'D':
	jargs->d = sv->u.integer;
	break;
      default:
	jargs_error(jvm, env);
      }
      break;
    case PIKE_T_FLOAT:
      switch(*sig++) {
      case 'Z':
	jargs->z = sv->u.float_number!=0;
	break;
      case 'B':
	jargs->b = DO_NOT_WARN((char)sv->u.float_number);
	break;
      case 'C':
	jargs->c = DO_NOT_WARN((unsigned short)sv->u.float_number);
	break;
      case 'S':
	jargs->s = DO_NOT_WARN((short)sv->u.float_number);
	break;
      case 'I':
	jargs->i = DO_NOT_WARN((long)sv->u.float_number);
	break;
      case 'J':
	jargs->j = sv->u.float_number;
	break;
      case 'F':
	jargs->f = sv->u.float_number;
	break;
      case 'D':
	jargs->d = sv->u.float_number;
	break;
      default:
	jargs_error(jvm, env);
      }
      break;
    case PIKE_T_STRING:
      if(*sig++!='L')
	jargs_error(jvm, env);
      if(i+1<args)
	while(*sig && *sig++!=';');
      switch(sv->u.string->size_shift) {
      case 0:
	{
	  /* Extra byte added to avoid zero length allocation */
	  jchar *newstr = (jchar *) xalloc(2 * sv->u.string->len + 1);
	  INT32 i;
	  p_wchar0 *p = STR0(sv->u.string);
	  for(i=sv->u.string->len; --i>=0; )
	    newstr[i]=(jchar)(unsigned char)p[i];
	  jargs->l = (*env)->NewString(env, newstr, sv->u.string->len);
          dorelease && (*dorelease = 1);
	  free(newstr);
	}
	break;
      case 1:
	jargs->l = (*env)->NewString(env, (jchar*)STR1(sv->u.string),
				     sv->u.string->len);
        dorelease && (*dorelease = 1);
	break;
      case 2:
	{
	  /* FIXME?: Does not make surrogates for plane 1-16 in group 0... */
	  /* Extra byte added to avoid zero length allocation */
	  jchar *newstr = (jchar *) xalloc(2 * sv->u.string->len + 1);
	  INT32 i;
	  p_wchar2 *p = STR2(sv->u.string);
	  for(i=sv->u.string->len; --i>=0; )
	    newstr[i]=(jchar)(p[i]>0xffff? 0xfffd : p[i]);
	  jargs->l = (*env)->NewString(env, newstr, sv->u.string->len);
          dorelease && (*dorelease = 1);
	  free(newstr);
	}
	break;
      }
      break;
    case PIKE_T_OBJECT:
      if(*sig=='[') {
	if(!(jo=(struct jobj_storage *)get_storage(sv->u.object,jobj_program)))
	  jargs_error(jvm, env);
	else {
	  if(i+1<args) {
	    while(*sig=='[') sig++;
	    if(*sig && *sig++=='L')
	      while(*sig && *sig++!=';');
	  }
	  jargs->l = jo->jobj;
	}
      } else {
	if(*sig++!='L' ||
	   !(jo=(struct jobj_storage *)get_storage(sv->u.object,jobj_program)))
	  jargs_error(jvm, env);
	else {
	  if(i+1<args)
	    while(*sig && *sig++!=';');
	  jargs->l = jo->jobj;
	}
      }
      break;
    default:
      jargs_error(jvm, env);
    }
    jargs++;
    dorelease && dorelease++;
  }
}

static void free_jargs(jvalue *jargs, INT32 args, char *dorelease, char *sig,
		       struct object *jvm, JNIEnv *env)
{
  INT32 i;
  int do_free_jargs = 1;
  if(jargs == NULL)
    return;

  if(args==-1)
  {
      args=1;
      do_free_jargs = 0;
  }
  if (dorelease)
    for (i=0; i<args; i++)
    {
      if (dorelease[i])
        (*env)->DeleteLocalRef(env, jargs[i].l);
    }

  if (do_free_jargs)
  {
    free(jargs);
    free(dorelease);
  }
}

static void f_call_static(INT32 args)
{
  struct method_storage *m=THIS_METHOD;
  jvalue *jargs = (m->nargs>0?(jvalue *)xalloc(m->nargs*sizeof(jvalue)):NULL);
  char *dorelease = (m->nargs>0?(char *)xalloc(m->nargs*sizeof(char)):NULL);

  JNIEnv *env;
  struct jobj_storage *co = THAT_JOBJ(m->class);
  jclass class = co->jobj;
  jobject jjo; FLOAT_TYPE jjf; INT32 jji;

  if(args != m->nargs)
    Pike_error("wrong number of arguments for method.\n");

  if((env = jvm_procure_env(co->jvm))==NULL) {
    pop_n_elems(args);
    push_int(0);
    return;
  }

  make_jargs(jargs, args, dorelease, m->sig->str, co->jvm, env);

  switch(m->rettype) {
  case 'Z':
    THREADS_ALLOW();
    jji = (*env)->CallStaticBooleanMethodA(env, class, m->method, jargs);
    THREADS_DISALLOW();
    pop_n_elems(args);
    push_int(jji);
    break;
  case 'B':
    THREADS_ALLOW();
    jji = (*env)->CallStaticByteMethodA(env, class, m->method, jargs);
    THREADS_DISALLOW();
    pop_n_elems(args);
    push_int(jji);
    break;
  case 'C':
    THREADS_ALLOW();
    jji = (*env)->CallStaticCharMethodA(env, class, m->method, jargs);
    THREADS_DISALLOW();
    pop_n_elems(args);
    push_int(jji);
    break;
  case 'S':
    THREADS_ALLOW();
    jji = (*env)->CallStaticShortMethodA(env, class, m->method, jargs);
    THREADS_DISALLOW();
    pop_n_elems(args);
    push_int(jji);
    break;
  case 'I':
    THREADS_ALLOW();
    jji = (*env)->CallStaticIntMethodA(env, class, m->method, jargs);
    THREADS_DISALLOW();
    pop_n_elems(args);
    push_int(jji);
    break;
  case 'J':
    THREADS_ALLOW();
    jji = (*env)->CallStaticLongMethodA(env, class, m->method, jargs);
    THREADS_DISALLOW();
    pop_n_elems(args);
    push_int(jji);
    break;
  case 'F':
    THREADS_ALLOW();
    jjf = (*env)->CallStaticFloatMethodA(env, class, m->method, jargs);
    THREADS_DISALLOW();
    pop_n_elems(args);
    push_float(jjf);
    break;
  case 'D':
    THREADS_ALLOW();
    jjf = (*env)->CallStaticDoubleMethodA(env, class, m->method, jargs);
    THREADS_DISALLOW();
    pop_n_elems(args);
    push_float(jjf);
    break;
  case 'L':
  case '[':
    THREADS_ALLOW();
    jjo = (*env)->CallStaticObjectMethodA(env, class, m->method, jargs);
    THREADS_DISALLOW();
    pop_n_elems(args);
    if(m->rettype=='[')
      push_java_array(jjo, co->jvm, env, m->subtype);
    else
      push_java_anyobj(jjo, co->jvm, env);
    break;
  case 'V':
  default:
    THREADS_ALLOW();
    (*env)->CallStaticVoidMethodA(env, class, m->method, jargs);
    THREADS_DISALLOW();
    pop_n_elems(args);
    push_int(0);
    break;
  }

  free_jargs(jargs, args, dorelease, m->sig->str, co->jvm, env);

  jvm_vacate_env(co->jvm, env);
}

static void f_call_virtual(INT32 args)
{
  struct method_storage *m=THIS_METHOD;
  jvalue *jargs = (m->nargs>0?(jvalue *)xalloc(m->nargs*sizeof(jvalue)):NULL);
  char *dorelease = (m->nargs>0?(char *)xalloc(m->nargs*sizeof(char)):NULL);

  JNIEnv *env;
  struct jobj_storage *co = THAT_JOBJ(m->class);
  jclass class = co->jobj;
  jobject jjo; FLOAT_TYPE jjf; INT32 jji;
  struct jobj_storage *jo;

  if(args != 1+m->nargs)
    Pike_error("wrong number of arguments for method.\n");

  if(Pike_sp[-args].type != PIKE_T_OBJECT || 
     (jo = (struct jobj_storage *)get_storage(Pike_sp[-args].u.object,
					      jobj_program))==NULL)
    Pike_error("Bad argument 1 to `().\n");

  if((env = jvm_procure_env(co->jvm))==NULL) {
    pop_n_elems(args);
    push_int(0);
    return;
  }

  make_jargs(jargs, args-1, dorelease, m->sig->str, co->jvm, env);

  switch(m->rettype) {
  case 'Z':
    THREADS_ALLOW();
    jji = (*env)->CallBooleanMethodA(env, jo->jobj, m->method, jargs);
    THREADS_DISALLOW();
    pop_n_elems(args);
    push_int(jji);
    break;
  case 'B':
    THREADS_ALLOW();
    jji = (*env)->CallByteMethodA(env, jo->jobj, m->method, jargs);
    THREADS_DISALLOW();
    pop_n_elems(args);
    push_int(jji);
    break;
  case 'C':
    THREADS_ALLOW();
    jji = (*env)->CallCharMethodA(env, jo->jobj, m->method, jargs);
    THREADS_DISALLOW();
    pop_n_elems(args);
    push_int(jji);
    break;
  case 'S':
    THREADS_ALLOW();
    jji = (*env)->CallShortMethodA(env, jo->jobj, m->method, jargs);
    THREADS_DISALLOW();
    pop_n_elems(args);
    push_int(jji);
    break;
  case 'I':
    THREADS_ALLOW();
    jji = (*env)->CallIntMethodA(env, jo->jobj, m->method, jargs);
    THREADS_DISALLOW();
    pop_n_elems(args);
    push_int(jji);
    break;
  case 'J':
    THREADS_ALLOW();
    jji = (*env)->CallLongMethodA(env, jo->jobj, m->method, jargs);
    THREADS_DISALLOW();
    pop_n_elems(args);
    push_int(jji);
    break;
  case 'F':
    THREADS_ALLOW();
    jjf = (*env)->CallFloatMethodA(env, jo->jobj, m->method, jargs);
    THREADS_DISALLOW();
    pop_n_elems(args);
    push_float(jjf);
    break;
  case 'D':
    THREADS_ALLOW();
    jjf = (*env)->CallDoubleMethodA(env, jo->jobj, m->method, jargs);
    THREADS_DISALLOW();
    pop_n_elems(args);
    push_float(jjf);
    break;
  case 'L':
  case '[':
    THREADS_ALLOW();
    jjo = (*env)->CallObjectMethodA(env, jo->jobj, m->method, jargs);
    THREADS_DISALLOW();
    pop_n_elems(args);
    if(m->rettype=='[')
      push_java_array(jjo, co->jvm, env, m->subtype);
    else
      push_java_anyobj(jjo, co->jvm, env);
    break;
  case 'V':
  default:
    THREADS_ALLOW();
    (*env)->CallVoidMethodA(env, jo->jobj, m->method, jargs);
    THREADS_DISALLOW();
    pop_n_elems(args);
    push_int(0);
    break;
  }

  free_jargs(jargs, args-1, dorelease, m->sig->str, co->jvm, env);

  jvm_vacate_env(co->jvm, env);
}

static void f_call_nonvirtual(INT32 args)
{
  struct method_storage *m=THIS_METHOD;
  jvalue *jargs = (m->nargs>0?(jvalue *)xalloc(m->nargs*sizeof(jvalue)):NULL);
  char *dorelease = (m->nargs>0?(char *)xalloc(m->nargs*sizeof(char)):NULL);

  JNIEnv *env;
  struct jobj_storage *co = THAT_JOBJ(m->class);
  jclass class = co->jobj;
  jobject jjo; FLOAT_TYPE jjf; INT32 jji;
  struct jobj_storage *jo;

  if(args != 1+m->nargs)
    Pike_error("wrong number of arguments for method.\n");

  if(Pike_sp[-args].type != PIKE_T_OBJECT || 
     (jo = (struct jobj_storage *)get_storage(Pike_sp[-args].u.object,
					      jobj_program))==NULL)
    Pike_error("Bad argument 1 to call_nonvirtual.\n");

  if((env = jvm_procure_env(co->jvm))==NULL) {
    pop_n_elems(args);
    push_int(0);
    return;
  }

  make_jargs(jargs, args-1, dorelease, m->sig->str, co->jvm, env);

  switch(m->rettype) {
  case 'Z':
    THREADS_ALLOW();
    jji = (*env)->CallNonvirtualBooleanMethodA(env, jo->jobj, class, m->method, jargs);
    THREADS_DISALLOW();
    pop_n_elems(args);
    push_int(jji);
    break;
  case 'B':
    THREADS_ALLOW();
    jji = (*env)->CallNonvirtualByteMethodA(env, jo->jobj, class, m->method, jargs);
    THREADS_DISALLOW();
    pop_n_elems(args);
    push_int(jji);
    break;
  case 'C':
    THREADS_ALLOW();
    jji = (*env)->CallNonvirtualCharMethodA(env, jo->jobj, class, m->method, jargs);
    THREADS_DISALLOW();
    pop_n_elems(args);
    push_int(jji);
    break;
  case 'S':
    THREADS_ALLOW();
    jji = (*env)->CallNonvirtualShortMethodA(env, jo->jobj, class, m->method, jargs);
    THREADS_DISALLOW();
    pop_n_elems(args);
    push_int(jji);
    break;
  case 'I':
    THREADS_ALLOW();
    jji = (*env)->CallNonvirtualIntMethodA(env, jo->jobj, class, m->method, jargs);
    THREADS_DISALLOW();
    pop_n_elems(args);
    push_int(jji);
    break;
  case 'J':
    THREADS_ALLOW();
    jji = (*env)->CallNonvirtualLongMethodA(env, jo->jobj, class, m->method, jargs);
    THREADS_DISALLOW();
    pop_n_elems(args);
    push_int(jji);
    break;
  case 'F':
    THREADS_ALLOW();
    jjf = (*env)->CallNonvirtualFloatMethodA(env, jo->jobj, class, m->method, jargs);
    THREADS_DISALLOW();
    pop_n_elems(args);
    push_float(jjf);
    break;
  case 'D':
    THREADS_ALLOW();
    jjf = (*env)->CallNonvirtualDoubleMethodA(env, jo->jobj, class, m->method, jargs);
    THREADS_DISALLOW();
    pop_n_elems(args);
    push_float(jjf);
    break;
  case 'L':
  case '[':
    THREADS_ALLOW();
    jjo = (*env)->CallNonvirtualObjectMethodA(env, jo->jobj, class, m->method, jargs);
    THREADS_DISALLOW();
    pop_n_elems(args);
    if(m->rettype=='[')
      push_java_array(jjo, co->jvm, env, m->subtype);
    else
      push_java_anyobj(jjo, co->jvm, env);
    break;
  case 'V':
  default:
    THREADS_ALLOW();
    (*env)->CallNonvirtualVoidMethodA(env, jo->jobj, class, m->method, jargs);
    THREADS_DISALLOW();
    pop_n_elems(args);
    push_int(0);
    break;
  }

  free_jargs(jargs, args-1, dorelease, m->sig->str, co->jvm, env);

  jvm_vacate_env(co->jvm, env);
}


/* Fields */

static void init_field_struct(struct object *o)
{
  struct field_storage *f=THIS_FIELD;

  f->class = NULL;
  f->name = NULL;
  f->sig = NULL;
}

static void exit_field_struct(struct object *o)
{
  struct field_storage *f=THIS_FIELD;

  if(f->sig != NULL)
    free_string(f->sig);
  if(f->name != NULL)
    free_string(f->name);
  if(f->class != NULL)
    free_object(f->class);
}

static void field_gc_check(struct object *o)
{
  struct field_storage *f = THIS_FIELD;

  if(f->class)
    gc_check(f->class);
}

static void field_gc_recurse(struct object *o)
{
  struct field_storage *f = THIS_FIELD;

  if(f->class)
    gc_recurse_object(f->class);
}

static void f_field_create(INT32 args)
{
  struct field_storage *f=THIS_FIELD;
  struct jobj_storage *c;
  struct object *class;
  struct pike_string *name, *sig;
  JNIEnv *env;

  if(args==1) {
    get_all_args("Java.field->create()", args, "%o", &class);
    name = NULL;
    sig = NULL;
  } else
    get_all_args("Java.field->create()", args, "%S%S%o", &name, &sig, &class);

  if((c = (struct jobj_storage *)get_storage(class, jclass_program)) == NULL)
    Pike_error("Bad argument 3 to create().\n");

  f->field = 0;

  if(name == NULL || sig == NULL) {
    f->class = class;
    add_ref(class);
    pop_n_elems(args);
    f->type = 0;
    return;
  }

  if((env = jvm_procure_env(c->jvm))) {

    f->field = (Pike_fp->current_object->prog==static_field_program?
		(*env)->GetStaticFieldID(env, c->jobj, name->str, sig->str):
		(*env)->GetFieldID(env, c->jobj, name->str, sig->str));

    jvm_vacate_env(c->jvm, env);
  }

  if(f->field == 0) {
    pop_n_elems(args);
    destruct(Pike_fp->current_object);
    return;
  }

  f->class = class;
  copy_shared_string(f->name, name);
  copy_shared_string(f->sig, sig);
  add_ref(class);
  pop_n_elems(args);
  push_int(0);

  if((f->type = sig->str[0])=='[')
    f->subtype = sig->str[1];
}

static void f_field_set(INT32 args)
{
  struct field_storage *f=THIS_FIELD;
  JNIEnv *env;
  struct jobj_storage *co = THAT_JOBJ(f->class);
  struct jobj_storage *jo;
  jvalue v;
  char dorelease;

  if(args!=2)
    Pike_error("Incorrect number of arguments to set.\n");

  if(Pike_sp[-args].type != PIKE_T_OBJECT || 
     (jo = (struct jobj_storage *)get_storage(Pike_sp[-args].u.object,
					      jobj_program))==NULL)
    Pike_error("Bad argument 1 to set.\n");

  if((env = jvm_procure_env(co->jvm))==NULL) {
    pop_n_elems(args);
    push_int(0);
    return;
  }

  make_jargs(&v, -1, &dorelease, f->sig->str, co->jvm, env);
  switch(f->type) {
  case 'Z':
    (*env)->SetBooleanField(env, jo->jobj, f->field, v.z);
    break;
  case 'B':
    (*env)->SetByteField(env, jo->jobj, f->field, v.b);
    break;
  case 'C':
    (*env)->SetCharField(env, jo->jobj, f->field, v.c);
    break;
  case 'S':
    (*env)->SetShortField(env, jo->jobj, f->field, v.s);
    break;
  case 'I':
    (*env)->SetIntField(env, jo->jobj, f->field, v.i);
    break;
  case 'J':
    (*env)->SetLongField(env, jo->jobj, f->field, v.j);
    break;
  case 'F':
    (*env)->SetFloatField(env, jo->jobj, f->field, v.f);
    break;
  case 'D':
    (*env)->SetDoubleField(env, jo->jobj, f->field, v.d);
    break;
  case 'L':
  case '[':
    (*env)->SetObjectField(env, jo->jobj, f->field, v.l);
    break;
  }

  free_jargs(&v, -1, &dorelease, f->sig->str, co->jvm, env);

  jvm_vacate_env(co->jvm, env);

  pop_n_elems(args);
  push_int(0);
}

static void f_field_get(INT32 args)
{
  struct field_storage *f=THIS_FIELD;
  JNIEnv *env;
  struct jobj_storage *co = THAT_JOBJ(f->class);
  jclass class = co->jobj;
  jobject jjo; FLOAT_TYPE jjf; INT32 jji;
  struct jobj_storage *jo;

  if(Pike_sp[-args].type != PIKE_T_OBJECT || 
     (jo = (struct jobj_storage *)get_storage(Pike_sp[-args].u.object,
					      jobj_program))==NULL)
    Pike_error("Bad argument 1 to get.\n");

  if((env = jvm_procure_env(co->jvm))==NULL) {
    pop_n_elems(args);
    push_int(0);
    return;
  }

  switch(f->type) {
  case 'Z':
    jji = (*env)->GetBooleanField(env, jo->jobj, f->field);
    pop_n_elems(args);
    push_int(jji);
    break;
  case 'B':
    jji = (*env)->GetByteField(env, jo->jobj, f->field);
    pop_n_elems(args);
    push_int(jji);
    break;
  case 'C':
    jji = (*env)->GetCharField(env, jo->jobj, f->field);
    pop_n_elems(args);
    push_int(jji);
    break;
  case 'S':
    jji = (*env)->GetShortField(env, jo->jobj, f->field);
    pop_n_elems(args);
    push_int(jji);
    break;
  case 'I':
    jji = (*env)->GetIntField(env, jo->jobj, f->field);
    pop_n_elems(args);
    push_int(jji);
    break;
  case 'J':
    jji = (*env)->GetLongField(env, jo->jobj, f->field);
    pop_n_elems(args);
    push_int(jji);
    break;
  case 'F':
    jjf = (*env)->GetFloatField(env, jo->jobj, f->field);
    pop_n_elems(args);
    push_float(jjf);
    break;
  case 'D':
    jjf = (*env)->GetDoubleField(env, jo->jobj, f->field);
    pop_n_elems(args);
    push_float(jjf);
    break;
  case 'L':
  case '[':
    jjo = (*env)->GetObjectField(env, jo->jobj, f->field);
    pop_n_elems(args);
    if(f->type=='[')
      push_java_array(jjo, co->jvm, env, f->subtype);
    else
      push_java_anyobj(jjo, co->jvm, env);
    break;
  default:
    pop_n_elems(args);
    push_int(0);
    break;
  }

  jvm_vacate_env(co->jvm, env);
}

static void f_static_field_set(INT32 args)
{
  struct field_storage *f=THIS_FIELD;
  JNIEnv *env;
  struct jobj_storage *co = THAT_JOBJ(f->class);
  jclass class = co->jobj;
  jvalue v;
  char dorelease;

  if(args!=1)
    Pike_error("Incorrect number of arguments to set.\n");

  if((env = jvm_procure_env(co->jvm))==NULL) {
    pop_n_elems(args);
    push_int(0);
    return;
  }

  make_jargs(&v, -1, &dorelease, f->sig->str, co->jvm, env);
  switch(f->type) {
  case 'Z':
    (*env)->SetStaticBooleanField(env, class, f->field, v.z);
    break;
  case 'B':
    (*env)->SetStaticByteField(env, class, f->field, v.b);
    break;
  case 'C':
    (*env)->SetStaticCharField(env, class, f->field, v.c);
    break;
  case 'S':
    (*env)->SetStaticShortField(env, class, f->field, v.s);
    break;
  case 'I':
    (*env)->SetStaticIntField(env, class, f->field, v.i);
    break;
  case 'J':
    (*env)->SetStaticLongField(env, class, f->field, v.j);
    break;
  case 'F':
    (*env)->SetStaticFloatField(env, class, f->field, v.f);
    break;
  case 'D':
    (*env)->SetStaticDoubleField(env, class, f->field, v.d);
    break;
  case 'L':
  case '[':
    (*env)->SetStaticObjectField(env, class, f->field, v.l);
    break;
  }

  free_jargs(&v, -1, &dorelease, f->sig->str, co->jvm, env);

  jvm_vacate_env(co->jvm, env);

  pop_n_elems(args);
  push_int(0);
}

static void f_static_field_get(INT32 args)
{
  struct field_storage *f=THIS_FIELD;
  JNIEnv *env;
  struct jobj_storage *co = THAT_JOBJ(f->class);
  jclass class = co->jobj;
  jobject jjo; FLOAT_TYPE jjf; INT32 jji;

  if((env = jvm_procure_env(co->jvm))==NULL) {
    pop_n_elems(args);
    push_int(0);
    return;
  }

  pop_n_elems(args);
  switch(f->type) {
  case 'Z':
    jji = (*env)->GetStaticBooleanField(env, class, f->field);
    push_int(jji);
    break;
  case 'B':
    jji = (*env)->GetStaticByteField(env, class, f->field);
    push_int(jji);
    break;
  case 'C':
    jji = (*env)->GetStaticCharField(env, class, f->field);
    push_int(jji);
    break;
  case 'S':
    jji = (*env)->GetStaticShortField(env, class, f->field);
    push_int(jji);
    break;
  case 'I':
    jji = (*env)->GetStaticIntField(env, class, f->field);
    push_int(jji);
    break;
  case 'J':
    jji = (*env)->GetStaticLongField(env, class, f->field);
    push_int(jji);
    break;
  case 'F':
    jjf = (*env)->GetStaticFloatField(env, class, f->field);
    push_float(jjf);
    break;
  case 'D':
    jjf = (*env)->GetStaticDoubleField(env, class, f->field);
    push_float(jjf);
    break;
  case 'L':
  case '[':
    jjo = (*env)->GetStaticObjectField(env, class, f->field);
    if(f->type=='[')
      push_java_array(jjo, co->jvm, env, f->subtype);
    else
      push_java_anyobj(jjo, co->jvm, env);
    break;
  default:
    push_int(0);
    break;
  }

  jvm_vacate_env(co->jvm, env);
}


/* Classes */


#ifdef SUPPORT_NATIVE_METHODS

struct native_method_context;

/* low_make_stub() returns the address of a function in ctx that
 * prepends data to the list of arguments, and then calls dispatch()
 * with the resulting argument list.
 *
 * Arguments:
 *   ctx	Context, usually just containing space for the machine code.
 *   data	Value to prepend in the argument list.
 *   statc	dispatch is a static method.
 *   dispatch	Function to call.
 *   args	Number of integer equvivalents to pass along.
 *   flt_args	bitfield: There are float arguments at these positions.
 *   dbl_args	bitfield: There are double arguments at these positions.
 */

#ifdef HAVE_SPARC_CPU

struct cpu_context {
  unsigned INT32 code[19];
};

static void *low_make_stub(struct cpu_context *ctx, void *data, int statc,
			   void (*dispatch)(), int args,
			   int flt_args, int dbl_args)
{
  unsigned INT32 *p = ctx->code;

  if(!statc)
    *p++ = 0xd223a048;  /* st  %o1, [ %sp + 0x48 ] */
  *p++ = 0xd423a04c;  /* st  %o2, [ %sp + 0x4c ] */
  *p++ = 0xd623a050;  /* st  %o3, [ %sp + 0x50 ] */
  *p++ = 0xd823a054;  /* st  %o4, [ %sp + 0x54 ] */
  *p++ = 0xda23a058;  /* st  %o5, [ %sp + 0x58 ] */
  *p++ = 0x9de3bf90;  /* save  %sp, -112, %sp    */

  *p++ = 0x11000000|(((unsigned INT32)data)>>10);
                      /* sethi  %hi(data), %o0   */
  *p++ = 0x90122000|(((unsigned INT32)data)&0x3ff);
                      /* or  %o0, %lo(data), %o0 */

  *p++ = 0x92162000;  /* mov  %i0, %o1           */
  if(statc) {
    *p++ = 0x94100019;  /* mov  %i1, %o2           */
    *p++ = 0x9607a04c;  /* add  %fp, 0x4c, %o3     */
  } else {
    *p++ = 0x94100000;  /* mov  %g0, %o2           */
    *p++ = 0x9607a048;  /* add  %fp, 0x48, %o3     */
  }

  *p++ = 0x19000000|(((unsigned INT32)(void *)dispatch)>>10);
                      /* sethi  %hi(dispatch), %o4   */
  *p++ = 0x98132000|(((unsigned INT32)(void *)dispatch)&0x3ff);
                      /* or  %o4, %lo(dispatch), %o4 */

  *p++ = 0x9fc30000;  /* call  %o4               */
  *p++ = 0x01000000;  /* nop                     */
  *p++ = 0xb0100008;  /* mov %o0,%i0             */
  *p++ = 0xb2100009;  /* mov %o1,%i1             */
  *p++ = 0x81c7e008;  /* ret                     */
  *p++ = 0x81e80000;  /* restore                 */

  return ctx->code;
}

#else
#ifdef HAVE_X86_CPU

struct cpu_context {
  unsigned char code[32];
};

static void *low_make_stub(struct cpu_context *ctx, void *data, int statc,
			   void (*dispatch)(), int args,
			   int flt_args, int dbl_args)
{
  unsigned char *p = ctx->code;

  *p++ = 0x55;               /* pushl  %ebp       */
  *p++ = 0x8b; *p++ = 0xec;  /* movl  %esp, %ebp  */
  *p++ = 0x8d; *p++ = 0x45;  /* lea  n(%ebp),%eax */
  if(statc)
    *p++ = 16;
  else
    *p++ = 12;
  *p++ = 0x50;               /* pushl  %eax       */
  if(statc) {
    *p++ = 0xff; *p++ = 0x75; *p++ = 0x0c;  /* pushl  12(%ebp) */
  } else {
    *p++ = 0x6a; *p++ = 0x00;               /* pushl  $0x0     */
  }
  *p++ = 0xff; *p++ = 0x75; *p++ = 0x08;  /* pushl  8(%ebp)  */
  *p++ = 0x68;               /* pushl  $data          */
  *((unsigned INT32 *)p) = (unsigned INT32)data; p+=4;
  *p++ = 0xb8;               /* movl  $dispatch, %eax */
  *((unsigned INT32 *)p) = (unsigned INT32)dispatch; p+=4;
  *p++ = 0xff; *p++ = 0xd0;  /* call  *%eax          */
  *p++ = 0x8b; *p++ = 0xe5;  /* movl  %ebp, %esp     */
  *p++ = 0x5d;               /* popl  %ebp           */
#ifdef __NT__
  *p++ = 0xc2;               /* ret   n              */
  *((unsigned INT16 *)p) = (unsigned INT16)(args<<2); p+=2;
#else /* !__NT__ */
  *p++ = 0xc3;               /* ret                  */
#endif /* __NT__ */

  return ctx->code;
}

#else
#ifdef HAVE_X86_64_CPU

#error Support for x86_64 not implemented yet!

struct cpu_context {
  unsigned char code[32];
};

static void *low_make_stub(struct cpu_context *ctx, void *data, int statc,
			   void (*dispatch)(), int args,
			   int flt_args, int dbl_args)
{
  unsigned char *p = ctx->code;

  *p++ = 0x55;               /* pushl  %ebp       */
  *p++ = 0x8b; *p++ = 0xec;  /* movl  %esp, %ebp  */
  *p++ = 0x8d; *p++ = 0x45;  /* lea  n(%ebp),%eax */
  if(statc)
    *p++ = 16;
  else
    *p++ = 12;
  *p++ = 0x50;               /* pushl  %eax       */
  if(statc) {
    *p++ = 0xff; *p++ = 0x75; *p++ = 0x0c;  /* pushl  12(%ebp) */
  } else {
    *p++ = 0x6a; *p++ = 0x00;               /* pushl  $0x0     */
  }
  *p++ = 0xff; *p++ = 0x75; *p++ = 0x08;  /* pushl  8(%ebp)  */
  *p++ = 0x68;               /* pushl  $data          */
  *((unsigned INT32 *)p) = (unsigned INT32)data; p+=4;
  *p++ = 0xb8;               /* movl  $dispatch, %eax */
  *((unsigned INT32 *)p) = (unsigned INT32)dispatch; p+=4;
  *p++ = 0xff; *p++ = 0xd0;  /* call  *%eax          */
  *p++ = 0x8b; *p++ = 0xe5;  /* movl  %ebp, %esp     */
  *p++ = 0x5d;               /* popl  %ebp           */
#ifdef __NT__
  *p++ = 0xc2;               /* ret   n              */
  *((unsigned INT16 *)p) = (unsigned INT16)(args<<2); p+=2;
#else /* !__NT__ */
  *p++ = 0xc3;               /* ret                  */
#endif /* __NT__ */

  return ctx->code;
}

#else
#ifdef HAVE_PPC_CPU

#ifdef __linux

/* SVR4 ABI */

#define VARARG_NATIVE_DISPATCH

#define NUM_FP_SAVE 8
#define REG_SAVE_AREA_SIZE (8*NUM_FP_SAVE+4*8+8)
#define STACK_FRAME_SIZE (8+REG_SAVE_AREA_SIZE+12+4)
#define VAOFFS0 (8+REG_SAVE_AREA_SIZE)
#define VAOFFS(x) ((((char*)&((*(va_list*)NULL))[0].x)-(char*)NULL)+VAOFFS0)

struct cpu_context {
  unsigned INT32 code[32+NUM_FP_SAVE];
};

static void *low_make_stub(struct cpu_context *ctx, void *data, int statc,
			   void (*dispatch)(), int args,
			   int flt_args, int dbl_args)
{
  unsigned INT32 *p = ctx->code;
  int i;

  *p++ = 0x7c0802a6;  /* mflr r0         */
  *p++ = 0x90010004;  /* stw r0,4(r1)    */
  *p++ = 0x94210000|(0xffff&-STACK_FRAME_SIZE);
		      /* stwu r1,-STACK_FRAME_SIZE(r1) */
  if(!statc)
    *p++ = 0x9081000c;  /* stw r4,12(r1)   */
  *p++ = 0x90a10010;  /* stw r5,16(r1)   */
  *p++ = 0x90c10014;  /* stw r6,20(r1)   */
  *p++ = 0x90e10018;  /* stw r7,24(r1)   */
  *p++ = 0x9101001c;  /* stw r8,28(r1)   */
  *p++ = 0x91210020;  /* stw r9,32(r1)   */
  *p++ = 0x91410024;  /* stw r10,36(r1)  */

  *p++ = 0x40a60000|(4*NUM_FP_SAVE+4);
		      /* bne+ cr1,.nofpsave */
  for(i=0; i<NUM_FP_SAVE; i++)
    *p++ = 0xd8010000|((i+1)<<21)|(8+4*8+8*i);
		      /* stfd fN,M(r1)   */

		      /* .nofpsave:      */
  if(statc) {
    *p++ = 0x7c852378;  /* mr r5,r4        */
    *p++ = 0x38000002;  /* li r0,2	   */
  } else {
    *p++ = 0x38a00000;  /* li r5,0         */
    *p++ = 0x38000001;  /* li r0,1	   */
  }
  
  *p++ = 0x7c641b78;  /* mr r4,r3        */
  *p++ = 0x98010000|VAOFFS(gpr);
		      /* stb r0,gpr      */
  *p++ = 0x38000000;  /* li r0,0         */
  *p++ = 0x98010000|VAOFFS(fpr);
		      /* stb r0,fpr      */
  *p++ = 0x38010000|(STACK_FRAME_SIZE+8);
		      /* addi r0,r1,STACK_FRAME_SIZE+8   */
  *p++ = 0x90010000|VAOFFS(overflow_arg_area);
		      /* stw r0,overflow_arg_area        */
  *p++ = 0x38010008;  /* addi r0,r1,8                    */
  *p++ = 0x90010000|VAOFFS(reg_save_area);
		      /* stw r0,reg_save_area            */

  *p++ = 0x38c10000|VAOFFS0;
		      /* addi r6,r1,va_list              */

  *p++ = 0x3c600000|(((unsigned INT32)(void *)data)>>16);
                      /* lis r3,hi16(data)          */
  *p++ = 0x60630000|(((unsigned INT32)(void *)data)&0xffff);
                      /* ori r3,r3,lo16(data)       */
 
  *p++ = 0x3d800000|(((unsigned INT32)(void *)dispatch)>>16);
                      /* lis r12,hi16(dispatch)     */
  *p++ = 0x618c0000|(((unsigned INT32)(void *)dispatch)&0xffff);
                      /* ori r12,r12,lo16(dispatch) */

  *p++ = 0x7d8803a6;  /* mtlr r12        */
  *p++ = 0x4e800021;  /* blrl            */

  *p++ = 0x80210000;  /* lwz r1,0(r1)    */
  *p++ = 0x80010004;  /* lwz r0,4(r1)    */
  *p++ = 0x7c0803a6;  /* mtlr r0         */
  *p++ = 0x4e800020;  /* blr             */

  return ctx->code;
}

#else /* __linux */

/* PowerOpen ABI */

struct cpu_context {
  unsigned INT32 code[23];
};

#define FLT_ARG_OFFS (args+wargs)

static void *low_make_stub(struct cpu_context *ctx, void *data, int statc,
			   void (*dispatch)(), int args,
			   int flt_args, int dbl_args)
{
  unsigned INT32 *p = ctx->code;

  *p++ = 0x7c0802a6;  /* mflr r0         */
  *p++ = 0x90010008;  /* stw r0,8(r1)    */
  *p++ = 0x9421ffc8;  /* stwu r1,-56(r1) */
  if(!statc)
    *p++ = 0x90810054;  /* stw r4,84(r1)   */
#ifdef __APPLE__
  {
    int i, fp=1;
    for(i=0; i<6; i++)
      if(flt_args&(1<<i))
	*p++ = 0xd0010000|((fp++)<<21)|(4*i+88);  /* stfs fN,X(r1)   */	
      else if(i<5 && dbl_args&(1<<i)) {
	*p++ = 0xd8010000|((fp++)<<21)|(4*i+88);  /* stfd fN,X(r1)   */	
	i++;
      } else
	*p++ = 0x90010000|((i+5)<<21)|(4*i+88);  /* stw rN,X(r1)   */
  }
#else
  *p++ = 0x90a10058;  /* stw r5,88(r1)   */
  *p++ = 0x90c1005c;  /* stw r6,92(r1)   */
  *p++ = 0x90e10060;  /* stw r7,96(r1)   */
  *p++ = 0x91010064;  /* stw r8,100(r1)  */
  *p++ = 0x91210068;  /* stw r9,104(r1)  */
  *p++ = 0x9141006c;  /* stw r10,108(r1) */
#endif

  if(statc) {
    *p++ = 0x7c852378;  /* mr r5,r4        */
    *p++ = 0x38c10058;  /* addi r6,r1,88   */
  } else {
    *p++ = 0x38a00000;  /* li r5,0         */
    *p++ = 0x38c10054;  /* addi r6,r1,84   */
  }
  
  *p++ = 0x7c641b78;  /* mr r4,r3        */

  *p++ = 0x3c600000|(((unsigned INT32)(void *)data)>>16);
                      /* lis r3,hi16(data)          */
  *p++ = 0x60630000|(((unsigned INT32)(void *)data)&0xffff);
                      /* ori r3,r3,lo16(data)       */
 
  *p++ = 0x3d800000|(((unsigned INT32)(void *)dispatch)>>16);
                      /* lis r12,hi16(dispatch)     */
  *p++ = 0x618c0000|(((unsigned INT32)(void *)dispatch)&0xffff);
                      /* ori r12,r12,lo16(dispatch) */

  *p++ = 0x7d8803a6;  /* mtlr r12        */
  *p++ = 0x4e800021;  /* blrl            */

  *p++ = 0x80210000;  /* lwz r1,0(r1)    */
  *p++ = 0x80010008;  /* lwz r0,8(r1)    */
  *p++ = 0x7c0803a6;  /* mtlr r0         */
  *p++ = 0x4e800020;  /* blr             */

  return ctx->code;
}

#endif /* __linux */

#else
#ifdef HAVE_ALPHA_CPU

/* NB: Assumes that pointers are 64bit! */

#define GET_NATIVE_ARG(ty) (((args)=((void**)(args))+1),*(ty *)(((void**)(args))-1))

struct cpu_context {
  void *code[10];
};

static void *low_make_stub(struct cpu_context *ctx, void *data, int statc,
			   void (*dispatch)(), int args,
			   int flt_args, int dbl_args)
{
  unsigned INT32 *p = (unsigned INT32 *)ctx->code;

  /* lda sp,-48(sp) */
  *p++ = 0x23deffd0;
  /* stq ra,0(sp) */
  *p++ = 0xb75e0000;
  if(!statc)
    /* stq a1,8(sp) */
    *p++ = 0xb63e0008;
  if(dbl_args & (1<<0))
    /* stt $f18,16(sp) */
    *p++ = 0x9e5e0010;
  else if(flt_args & (1<<0))
    /* sts $f18,16(sp) */
    *p++ = 0x9a5e0010;
  else
    /* stq a2,16(sp) */
    *p++ = 0xb65e0010;
  if(dbl_args & (1<<1))
    /* stt $f19,24(sp) */
    *p++ = 0x9e7e0018;
  else if(flt_args & (1<<1))
    /* sts $f19,24(sp) */
    *p++ = 0x9a7e0018;
  else
    /* stq a3,24(sp) */
    *p++ = 0xb67e0018;
  if(dbl_args & (1<<2))
    /* stt $f20,32(sp) */
    *p++ = 0x9e9e0020;
  else if(flt_args & (1<<2))
    /* sts $f20,32(sp) */
    *p++ = 0x9a9e0020;
  else
    /* stq a4,32(sp) */
    *p++ = 0xb69e0020;
  if(dbl_args & (1<<3))
    /* stt $f21,40(sp) */
    *p++ = 0x9ebe0028;
  else if(flt_args & (1<<3))
    /* sts $f21,40(sp) */
    *p++ = 0x9abe0028;
  else
    /* stq a5,40(sp) */
    *p++ = 0xb6be0028;
  if(statc) { 
    /* mov a1,a2 */
    *p++ = 0x46310412;
    /* lda a3,16(sp) */
    *p++ = 0x227e0010;
  } else { 
    /* clr a2 */
    *p++ = 0x47ff0412;
    /* lda a3,8(sp) */
    *p++ = 0x227e0008;
  } 
  /* mov a0,a1 */
  *p++ = 0x46100411;
  /* ldq a0,64(t12) */
  *p++ = 0xa61b0040;
  /* ldq t12,72(t12) */
  *p++ = 0xa77b0048;
  /* jsr ra,(t12) */
  *p++ = 0x6b5b4000;
  /* ldq ra,0(sp) */
  *p++ = 0xa75e0000;
  /* lda sp,48(sp) */
  *p++ = 0x23de0030;
  /* ret zero,(ra) */
  *p++ = 0x6bfa8001;

  ctx->code[8] = data;
  ctx->code[9] = dispatch;

  return ctx->code;
}

#else
#error How did you get here?  It should never happen.
#endif /* HAVE_ALPHA_CPU */
#endif /* HAVE_PPC_CPU */
#endif /* HAVE_X86_64_CPU */
#endif /* HAVE_X86_CPU */
#endif /* HAVE_SPARC_CPU */

struct natives_storage;

struct native_method_context {
  struct svalue callback;
  struct pike_string *name, *sig;
  struct natives_storage *nat;
  struct cpu_context cpu;
};

struct natives_storage {

  struct object *jvm, *cls;
  int num_methods;
  struct native_method_context *cons;
  JNINativeMethod *jnms;

};

static void make_java_exception(struct object *jvm, JNIEnv *env,
				struct svalue *v)
{
  union anything *a;
  struct generic_error_struct *gen_err;
  struct jvm_storage *j =
    (struct jvm_storage *)get_storage(jvm, jvm_program);

  if(!j)
    return;

  push_svalue (v);
  SAFE_APPLY_MASTER ("describe_error", 1);
#ifdef PIKE_DEBUG
  if (Pike_sp[-1].type != PIKE_T_STRING)
    Pike_fatal ("Unexpected return value from destribe_error\n");
#endif
  (*env)->ThrowNew(env, j->class_runtimex, Pike_sp[-1].u.string->str);
  pop_stack();
}

#ifdef VARARG_NATIVE_DISPATCH

#define ARGS_TYPE va_list*
#define GET_NATIVE_ARG(ty) va_arg(*args,ty)
#define NATIVE_ARG_JFLOAT_TYPE jdouble

#else

#ifndef ARGS_TYPE
#define ARGS_TYPE void*
#endif

#ifndef GET_NATIVE_ARG
#define GET_NATIVE_ARG(ty) (((args)=((ty *)(args))+1),(((ty *)(args))[-1]))
#endif

#ifndef NATIVE_ARG_JFLOAT_TYPE
#define NATIVE_ARG_JFLOAT_TYPE jfloat
#endif

#endif

static void do_native_dispatch(struct native_method_context *ctx,
			       JNIEnv *env, jclass cls, void *args_,
			       jvalue *rc)
{
  JMP_BUF recovery;
  struct svalue *osp = Pike_sp;
  char *p;

  if (SETJMP(recovery)) {
    make_java_exception(ctx->nat->jvm, env, &throw_value);
    pop_n_elems(Pike_sp-osp);
    UNSETJMP(recovery);
    free_svalue(&throw_value);
    throw_value.type = PIKE_T_INT;
    return;
  }

  {
    int nargs = 0;
    ARGS_TYPE args = args_;

    if(!cls) {
      push_java_anyobj(GET_NATIVE_ARG(jobject), ctx->nat->jvm, env);
      nargs++;
    }

    p = ctx->sig->str;

    if(*p == '(')
      p++;

    while(*p && *p!=')') {
      switch(*p++) {
      case 'Z':
      case 'B':
      case 'C':
      case 'S':
      case 'I':
      default:
	push_int(GET_NATIVE_ARG(jint));
	break;
      
      case 'J':
	push_int(GET_NATIVE_ARG(jlong));
	break;
      
      case 'F':
	push_float(GET_NATIVE_ARG(NATIVE_ARG_JFLOAT_TYPE));
	break;
      
      case 'D':
	push_float(GET_NATIVE_ARG(jdouble));
	break;
      
      case 'L':
	push_java_anyobj(GET_NATIVE_ARG(jobject), ctx->nat->jvm, env);
	while(*p && *p++!=';') ;
	break;
      
      case '[':
	push_java_array(GET_NATIVE_ARG(jarray), ctx->nat->jvm, env, *p);
	while(*p == '[')
	  p++;
	if(*p++ == 'L')
	  while(*p && *p++!=';') ;
	break;
      }
      nargs ++;
    }

    if(*p == ')')
      p++;

    apply_svalue(&ctx->callback, nargs);

    memset(rc, 0, sizeof(*rc));

    if(*p != 'V') {
      /* The Local Referens that is created here will be
         released automatically when we return to java */
      make_jargs(rc, -1, NULL, p, ctx->nat->jvm, env);
      if((*p == 'L' || *p == '[') && rc->l != NULL)
	rc->l = (*env)->NewLocalRef(env, rc->l);
    }
  }

  pop_n_elems(Pike_sp-osp);
  UNSETJMP(recovery);
}

static void native_dispatch(struct native_method_context *ctx,
			    JNIEnv *env, jclass cls, void *args,
			    jvalue *rc)
{
  struct thread_state *state;

  if((state = thread_state_for_id(th_self()))!=NULL) {
    /* This is a pike thread.  Do we have the interpreter lock? */
    if(!state->swapped) {
      /* Yes.  Go for it... */
      do_native_dispatch(ctx, env, cls, args, rc);
    } else {
      /* Nope, let's get it... */
      mt_lock_interpreter();
      SWAP_IN_THREAD(state);

      do_native_dispatch(ctx, env, cls, args, rc);

      /* Restore */
      SWAP_OUT_THREAD(state);
      mt_unlock_interpreter();
    }
  } else {
    /* Not a pike thread.  Create a temporary thread_id... */
    struct object *thread_obj;

    mt_lock_interpreter();
    init_interpreter();
    Pike_interpreter.stack_top=((char *)&state)+ (thread_stack_size-16384) * STACK_DIRECTION;
    Pike_interpreter.recoveries = NULL;
    thread_obj = fast_clone_object(thread_id_prog);
    INIT_THREAD_STATE((struct thread_state *)(thread_obj->storage +
					      thread_storage_offset));
    num_threads++;
    thread_table_insert(Pike_interpreter.thread_state);

    do_native_dispatch(ctx, env, cls, args, rc);

    cleanup_interpret();	/* Must be done before EXIT_THREAD_STATE */
    Pike_interpreter.thread_state->status=THREAD_EXITED;
    co_signal(&Pike_interpreter.thread_state->status_change);
    thread_table_delete(Pike_interpreter.thread_state);
    EXIT_THREAD_STATE(Pike_interpreter.thread_state);
    Pike_interpreter.thread_state=NULL;
    free_object(thread_obj);
    thread_obj = NULL;
    num_threads--;
    mt_unlock_interpreter();
  }
}

static jboolean JNICALL native_dispatch_z(struct native_method_context *ctx,
					  JNIEnv *env, jobject obj, void *args)
{
  jvalue v;
  native_dispatch(ctx, env, obj, args, &v);
  return v.z;
}

static jbyte JNICALL native_dispatch_b(struct native_method_context *ctx,
				       JNIEnv *env, jobject obj, void *args)
{
  jvalue v;
  native_dispatch(ctx, env, obj, args, &v);
  return v.b;
}

static jchar JNICALL native_dispatch_c(struct native_method_context *ctx,
				       JNIEnv *env, jobject obj, void *args)
{
  jvalue v;
  native_dispatch(ctx, env, obj, args, &v);
  return v.c;
}

static jshort JNICALL native_dispatch_s(struct native_method_context *ctx,
					JNIEnv *env, jobject obj, void *args)
{
  jvalue v;
  native_dispatch(ctx, env, obj, args, &v);
  return v.s;
}

static jint JNICALL native_dispatch_i(struct native_method_context *ctx,
				      JNIEnv *env, jobject obj, void *args)
{
  jvalue v;
  native_dispatch(ctx, env, obj, args, &v);
  return v.i;
}

static jlong JNICALL native_dispatch_j(struct native_method_context *ctx,
				       JNIEnv *env, jobject obj, void *args)
{
  jvalue v;
  native_dispatch(ctx, env, obj, args, &v);
  return v.j;
}

static jfloat JNICALL native_dispatch_f(struct native_method_context *ctx,
					JNIEnv *env, jobject obj, void *args)
{
  jvalue v;
  native_dispatch(ctx, env, obj, args, &v);
  return v.f;
}

static jdouble JNICALL native_dispatch_d(struct native_method_context *ctx,
					 JNIEnv *env, jobject obj, void *args)
{
  jvalue v;
  native_dispatch(ctx, env, obj, args, &v);
  return v.d;
}

static jobject JNICALL native_dispatch_l(struct native_method_context *ctx,
					 JNIEnv *env, jobject obj, void *args)
{
  jvalue v;
  native_dispatch(ctx, env, obj, args, &v);
  return v.l;
}

static void JNICALL native_dispatch_v(struct native_method_context *ctx,
				      JNIEnv *env, jobject obj, void *args)
{
  jvalue v;
  native_dispatch(ctx, env, obj, args, &v);
}

static void *make_stub(struct cpu_context *ctx, void *data, int statc, int rt,
		       int args, int flt_args, int dbl_args)
{
  void (*disp)() = (void (*)())native_dispatch_v;

  switch(rt) {
  case 'Z': disp = (void (*)())native_dispatch_z; break;
  case 'B': disp = (void (*)())native_dispatch_b; break;
  case 'C': disp = (void (*)())native_dispatch_c; break;
  case 'S': disp = (void (*)())native_dispatch_s; break;
  case 'I': disp = (void (*)())native_dispatch_i; break;
  case 'J': disp = (void (*)())native_dispatch_j; break;
  case 'F': disp = (void (*)())native_dispatch_f; break;
  case 'D': disp = (void (*)())native_dispatch_d; break;
  case '[':
  case 'L': disp = (void (*)())native_dispatch_l; break;
  default:
    disp = (void (*)())native_dispatch_v;
  }

  return low_make_stub(ctx, data, statc, disp, args, flt_args, dbl_args);
}

#ifndef FLT_ARG_OFFS
#define FLT_ARG_OFFS args
#endif

static void build_native_entry(JNIEnv *env, jclass cls,
			       struct native_method_context *con,
			       JNINativeMethod *jnm,
			       struct pike_string *name,
			       struct pike_string *sig)
{
  int statc, args=0, wargs=0, flt_args=0, dbl_args=0;
  char *p = sig->str;

  copy_shared_string(con->name, name);
  copy_shared_string(con->sig, sig);

  if((*env)->GetMethodID(env, cls, name->str, sig->str))
    statc = 0;
  else {
    (*env)->ExceptionClear(env);
    if((*env)->GetStaticMethodID(env, cls, name->str, sig->str))
      statc = 1;
    else {
      (*env)->ExceptionClear(env);
      Pike_error("trying to register nonexistent function\n");
    }
  }

  jnm->name = name->str;
  jnm->signature = sig->str;
  while(*p && *p != ')')
    switch(*p++) {
    case '(':
      break;
    case 'D':
      dbl_args |= 1<<FLT_ARG_OFFS;
    case 'J':
      args ++;
      wargs ++;
      break;
    case 'F':
      flt_args |= 1<<FLT_ARG_OFFS;
      args ++;
      break;
    case '[':
      if(!*p)
	break;
      if(*p++ != 'L') { args++; break; }
    case 'L':
      while(*p && *p++!=';');
    default:
      args++;
    }
  if(*p) p++;
  jnm->fnPtr = make_stub(&con->cpu, con, statc, *p, args+wargs+2,
			 flt_args, dbl_args);
}

static void init_natives_struct(struct object *o)
{
  struct natives_storage *n = THIS_NATIVES;

  n->jvm = NULL;
  n->cls = NULL;
  n->num_methods = 0;
  n->cons = NULL;
  n->jnms = NULL;
}

static void exit_natives_struct(struct object *o)
{
  JNIEnv *env;
  struct natives_storage *n = THIS_NATIVES;
  
  if(n->jvm) {
    if(n->cls) {
      if((env = jvm_procure_env(n->jvm)) != NULL) {
	(*env)->UnregisterNatives(env, THAT_JOBJ(n->cls)->jobj);
	jvm_vacate_env(n->jvm, env);
      }
      free_object(n->cls);
    }
    free_object(n->jvm);
  }
  if(n->jnms)
    free(n->jnms);
  if(n->cons) {
    int i;
    for(i=0; i<n->num_methods; i++) {
      free_svalue(&n->cons[i].callback);
      if(n->cons[i].name)
	free_string(n->cons[i].name);
      if(n->cons[i].sig)
	free_string(n->cons[i].sig);
    }
    mexec_free(n->cons);
  }
}

static void natives_gc_check(struct object *o)
{
  struct natives_storage *n = THIS_NATIVES;

  if(n->jvm)
    gc_check(n->jvm);
  if(n->cls)
    gc_check(n->cls);
  if(n->cons) {
    int i;
    for(i=0; i<n->num_methods; i++)
      gc_check_svalues(&n->cons[i].callback, 1);
  }
}

static void natives_gc_recurse(struct object *o)
{
  struct natives_storage *n = THIS_NATIVES;

  if(n->jvm)
    gc_recurse_object(n->jvm);
  if(n->cls)
    gc_recurse_object(n->cls);
  if(n->cons) {
    int i;
    for(i=0; i<n->num_methods; i++)
      gc_recurse_svalues(&n->cons[i].callback, 1);
  }
}

static void f_natives_create(INT32 args)
{
  struct natives_storage *n = THIS_NATIVES;
  struct jobj_storage *c;
  struct object *cls;
  struct array *arr;
  int i, rc=-1;
  JNIEnv *env;

  get_all_args("Java.natives->create()", args, "%a%o", &arr, &cls);

  if((c = (struct jobj_storage *)get_storage(cls, jclass_program)) == NULL)
    Pike_error("Bad argument 2 to create().\n");

  if(n->num_methods)
    Pike_error("create() called twice in Java.natives object.\n");

  if(!arr->size) {
    pop_n_elems(args);
    return;
  }

  if((env = jvm_procure_env(c->jvm))) {
    if (n->jnms) free(n->jnms);
    n->jnms = (JNINativeMethod *)
      xalloc(arr->size * sizeof(JNINativeMethod));

    if (n->cons) {
      mexec_free(n->cons);
    }

    if (!(n->cons = (struct native_method_context *)
	  mexec_alloc(arr->size * sizeof(struct native_method_context)))) {
      Pike_error("Out of memory.\n");
    }

    for(i=0; i<arr->size; i++) {
      struct array *nm;
      if(ITEM(arr)[i].type != PIKE_T_ARRAY || ITEM(arr)[i].u.array->size != 3)
	Pike_error("Bad argument 1 to create().\n");
      nm = ITEM(arr)[i].u.array;
      if(ITEM(nm)[0].type != PIKE_T_STRING || ITEM(nm)[1].type != PIKE_T_STRING)
	Pike_error("Bad argument 1 to create().\n");
      assign_svalue_no_free(&n->cons[i].callback, &ITEM(nm)[2]);
      n->cons[i].nat = n;
      n->num_methods++;
      
      build_native_entry(env, c->jobj, &n->cons[i], &n->jnms[i],
			 ITEM(nm)[0].u.string, ITEM(nm)[1].u.string);
    }
    
    n->jvm = c->jvm;
    n->cls = cls;
    add_ref(n->jvm);
    add_ref(n->cls);

    rc = (*env)->RegisterNatives(env, c->jobj, n->jnms, n->num_methods);
    jvm_vacate_env(c->jvm, env);
  }

  pop_n_elems(args);

  if(rc<0)
    destruct(Pike_fp->current_object);
}

#endif /* SUPPORT_NATIVE_METHODS */

static void f_super_class(INT32 args)
{
  struct jobj_storage *jo = THIS_JOBJ;
  JNIEnv *env;
  
  pop_n_elems(args);
  if((env = jvm_procure_env(jo->jvm))) {
    push_java_class((*env)->GetSuperclass(env, jo->jobj), jo->jvm, env);
    jvm_vacate_env(jo->jvm, env);
  } else
    push_int(0);
}

static void f_is_assignable_from(INT32 args)
{
  struct jobj_storage *jc, *jo = THIS_JOBJ;
  JNIEnv *env;
  jboolean iaf;

  if(args<1 || Pike_sp[-args].type != PIKE_T_OBJECT ||
     (jc = (struct jobj_storage *)get_storage(Pike_sp[-args].u.object,
					      jclass_program))==NULL)
    Pike_error("illegal argument 1 to is_assignable_from\n");

  if((env = jvm_procure_env(jo->jvm))) {
    iaf = (*env)->IsAssignableFrom(env, jo->jobj, jc->jobj);
    jvm_vacate_env(jo->jvm, env);
  } else
    iaf = 0;

  pop_n_elems(args);
  push_int((iaf? 1:0));
}

static void f_throw_new(INT32 args)
{
  struct jobj_storage *jo = THIS_JOBJ;
  struct jvm_storage *jj =
    (struct jvm_storage *)get_storage(jo->jvm, jvm_program);
  JNIEnv *env;
  char *cn;

  get_all_args("throw_new", args, "%s", &cn);

  if((env = jvm_procure_env(jo->jvm))) {

    if(!(*env)->IsAssignableFrom(env, jo->jobj, jj->class_throwable)) {
      jvm_vacate_env(jo->jvm, env);
      Pike_error("throw_new called in a class that doesn't inherit java.lang.Throwable!\n");
    }

    if((*env)->ThrowNew(env, jo->jobj, cn)<0) {
      jvm_vacate_env(jo->jvm, env);
      Pike_error("throw_new failed!\n");
    }

    jvm_vacate_env(jo->jvm, env);
  }

  pop_n_elems(args);
  push_int(0);
}

static void f_alloc(INT32 args)
{
  struct jobj_storage *jo = THIS_JOBJ;
  JNIEnv *env;
  
  pop_n_elems(args);

  if((env = jvm_procure_env(jo->jvm))) {
    push_java_anyobj((*env)->AllocObject(env, jo->jobj), jo->jvm, env);
    jvm_vacate_env(jo->jvm, env);
  } else push_int(0);
}

static void f_new_array(INT32 args)
{
  struct jobj_storage *jo = THIS_JOBJ;
  struct jvm_storage *j =
    (struct jvm_storage *)get_storage(jo->jvm, jvm_program);
  struct object *o;
  JNIEnv *env;
  jvalue i;
  char dorelease;
  jarray a;
  INT_TYPE n;

  if(args==1) {
    push_int(0);
    args++;
  }

  get_all_args("new_array", args, "%i%O", &n, &o);

  if((env = jvm_procure_env(jo->jvm))) {
    make_jargs(&i, -1, &dorelease, "L", jo->jvm, env);
    a = (*env)->NewObjectArray(env, n, jo->jobj, i.l);
    pop_n_elems(args);
    push_java_array(a, jo->jvm, env,
		    ((*env)->CallBooleanMethod(env, jo->jobj,
					       j->method_isarray)? '[':'L'));
    free_jargs(&i, -1, &dorelease, "L", jo->jvm, env);
    jvm_vacate_env(jo->jvm, env);
  } else {
    pop_n_elems(args);
    push_int(0);
  }
}

static void f_get_method(INT32 args)
{
  struct object *oo;

  check_all_args("get_method", args, BIT_STRING, BIT_STRING, 0);

  push_object(this_object());
  oo=clone_object(method_program, args+1);
  if(oo->prog!=NULL)
    push_object(oo);
  else {
    free_object(oo);
    push_int(0);
  }
}

static void f_get_static_method(INT32 args)
{
  struct object *oo;

  check_all_args("get_static_method", args, BIT_STRING, BIT_STRING, 0);

  push_object(this_object());
  oo=clone_object(static_method_program, args+1);
  if(oo->prog!=NULL)
    push_object(oo);
  else {
    free_object(oo);
    push_int(0);
  }
}

static void f_get_field(INT32 args)
{
  struct object *oo;

  check_all_args("get_field", args, BIT_STRING, BIT_STRING, 0);

  push_object(this_object());
  oo=clone_object(field_program, args+1);
  if(oo->prog!=NULL)
    push_object(oo);
  else {
    free_object(oo);
    push_int(0);
  }
}

static void f_get_static_field(INT32 args)
{
  struct object *oo;

  check_all_args("get_static_field", args, BIT_STRING, BIT_STRING, 0);

  push_object(this_object());
  oo=clone_object(static_field_program, args+1);
  if(oo->prog!=NULL)
    push_object(oo);
  else {
    free_object(oo);
    push_int(0);
  }
}

#ifdef SUPPORT_NATIVE_METHODS
static void f_register_natives(INT32 args)
{
  struct object *oo;
  check_all_args("register_natives", args, BIT_ARRAY, 0);
  push_object(this_object());
  oo=clone_object(natives_program, args+1);
  if(oo->prog!=NULL)
    push_object(oo);
  else {
    free_object(oo);
    push_int(0);
  } 
}
#endif /* SUPPORT_NATIVE_METHODS */


/* Throwables */


static void f_javathrow(INT32 args)
{
  struct jobj_storage *jo = THIS_JOBJ;
  JNIEnv *env;

  pop_n_elems(args);

  if((env = jvm_procure_env(jo->jvm))) {
    if((*env)->Throw(env, jo->jobj)<0) {
      jvm_vacate_env(jo->jvm, env);
      Pike_error("throw failed!\n");
    }
    jvm_vacate_env(jo->jvm, env);
  }
  push_int(0);
}


/* Arrays */

static void f_javaarray_sizeof(INT32 args)
{
  struct jobj_storage *jo = THIS_JOBJ;
  JNIEnv *env;

  pop_n_elems(args);

  if((env = jvm_procure_env(jo->jvm))) {
    push_int((*env)->GetArrayLength(env, jo->jobj));  
    jvm_vacate_env(jo->jvm, env);
  } else
    push_int(0);
}

static void javaarray_subarray(struct object *jvm, struct object *oo,
			       jobject jobj, int ty, INT32 e1, INT32 e2)
{
  JNIEnv *env;
  jobject jobj2;
  jclass jocls, eltcls;
  struct jvm_storage *j;

  if((j = (struct jvm_storage *)get_storage(jvm, jvm_program))==NULL) {
    push_int(0);
    return;
  }

  if((env = jvm_procure_env(jvm))) {
    jsize size = (*env)->GetArrayLength(env, jobj);

    if(e1<0)
      e1=0;
    if(e1>size)
      e1=size;
    if(e2>=size)
      e2=size-1;
    if(e2<e1)
      e2=0;
    else
      e2-=e1-1;

    if(e2==size) {
      /* Entire array selected */
      jvm_vacate_env(jvm, env);
      ref_push_object(oo);
      return;
    }

    switch(ty) {
    case 'Z':
      jobj2 = (*env)->NewBooleanArray(env, e2);
      break;
    case 'B':
      jobj2 = (*env)->NewByteArray(env, e2);
      break;
    case 'C':
      jobj2 = (*env)->NewCharArray(env, e2);
      break;
    case 'S':
      jobj2 = (*env)->NewShortArray(env, e2);
      break;
    case 'I':
      jobj2 = (*env)->NewIntArray(env, e2);
      break;
    case 'J':
      jobj2 = (*env)->NewLongArray(env, e2);
      break;
    case 'F':
      jobj2 = (*env)->NewFloatArray(env, e2);
      break;
    case 'D':
      jobj2 = (*env)->NewDoubleArray(env, e2);
      break;
    case 'L':
    case '[':
    default:
      jocls = (*env)->GetObjectClass(env, jobj);
      eltcls = (jclass)(*env)->CallObjectMethod(env, jocls,
						j->method_getcomponenttype);
      jobj2 = (*env)->NewObjectArray(env, e2, eltcls, 0);
      (*env)->DeleteLocalRef(env, eltcls);
      (*env)->DeleteLocalRef(env, jocls);
      break;
    }

    if(!jobj2) {
      jvm_vacate_env(jvm, env);
      push_int(0);
      return;
    }

    if(e2)
      (*env)->CallStaticVoidMethod(env, j->class_system, j->method_arraycopy,
				   jobj, (jint)e1, jobj2, (jint)0, (jint)e2);
    push_java_array(jobj2, jvm, env, ty);
    jvm_vacate_env(jvm, env);
  } else
    push_int(0);
}

static void f_javaarray_getelt(INT32 args)
{
  struct jobj_storage *jo = THIS_JOBJ;
  struct jarray_storage *ja = THIS_JARRAY;
  JNIEnv *env;
  INT32 n;
  jvalue jjv;

  if(args<1 || Pike_sp[-args].type != PIKE_T_INT ||
     (args>1 && Pike_sp[1-args].type != PIKE_T_INT))
    Pike_error("Bad args to `[].\n");

  n = Pike_sp[-args].u.integer;

  if(args>1) {
    INT32 m = Pike_sp[1-args].u.integer;
    pop_n_elems(args);
    javaarray_subarray(jo->jvm, Pike_fp->current_object, jo->jobj, ja->ty, n, m);
    return;
  }

  pop_n_elems(args);

  if((env = jvm_procure_env(jo->jvm))) {

    if(n<0) {
      /* Count backwards... */
      n += (*env)->GetArrayLength(env, jo->jobj);
    }

    switch(ja->ty) {
    case 'Z':
      (*env)->GetBooleanArrayRegion(env, jo->jobj, n, 1, &jjv.z);
      push_int(jjv.z);
      break;
    case 'B':
      (*env)->GetByteArrayRegion(env, jo->jobj, n, 1, &jjv.b);
      push_int(jjv.b);
      break;
    case 'C':
      (*env)->GetCharArrayRegion(env, jo->jobj, n, 1, &jjv.c);
      push_int(jjv.c);
      break;
    case 'S':
      (*env)->GetShortArrayRegion(env, jo->jobj, n, 1, &jjv.s);
      push_int(jjv.s);
      break;
    case 'I':
      (*env)->GetIntArrayRegion(env, jo->jobj, n, 1, &jjv.i);
      push_int(jjv.i);
      break;
    case 'J':
      (*env)->GetLongArrayRegion(env, jo->jobj, n, 1, &jjv.j);
      push_int(jjv.j);
      break;
    case 'F':
      (*env)->GetFloatArrayRegion(env, jo->jobj, n, 1, &jjv.f);
      push_float(jjv.f);
      break;
    case 'D':
      (*env)->GetDoubleArrayRegion(env, jo->jobj, n, 1, &jjv.d);
      push_float(jjv.d);
      break;
    case 'L':
    case '[':
      push_java_anyobj((*env)->GetObjectArrayElement(env, jo->jobj, n),
		       jo->jvm, env);
      break;
    default:
      push_int(0);
    }
    jvm_vacate_env(jo->jvm, env);
  } else
    push_int(0);
}

static void f_javaarray_setelt(INT32 args)
{
  struct jobj_storage *jo = THIS_JOBJ;
  struct jarray_storage *ja = THIS_JARRAY;
  JNIEnv *env;
  INT32 n;
  jvalue jjv;
  char dorelease;
  char ty2;

  if(args<2 || Pike_sp[-args].type != PIKE_T_INT)
    Pike_error("Bad args to `[]=.\n");

  if(args>2)
    pop_n_elems(args-2);

  n = Pike_sp[-2].u.integer;

  if((env = jvm_procure_env(jo->jvm))==NULL) {
    pop_n_elems(2);
    push_int(0);
    return;
  }

  ty2 = ja->ty;
  make_jargs(&jjv, -1, &dorelease, &ty2, jo->jvm, env);

  assign_svalue(&Pike_sp[-2], &Pike_sp[-1]);
  pop_n_elems(1);

  if(n<0) {
    /* Count backwards... */
    n += (*env)->GetArrayLength(env, jo->jobj);
  }

  switch(ja->ty) {
   case 'Z':
     (*env)->SetBooleanArrayRegion(env, jo->jobj, n, 1, &jjv.z);
     break;
   case 'B':
     (*env)->SetByteArrayRegion(env, jo->jobj, n, 1, &jjv.b);
     break;
   case 'C':
     (*env)->SetCharArrayRegion(env, jo->jobj, n, 1, &jjv.c);
     break;
   case 'S':
     (*env)->SetShortArrayRegion(env, jo->jobj, n, 1, &jjv.s);
     break;
   case 'I':
     (*env)->SetIntArrayRegion(env, jo->jobj, n, 1, &jjv.i);
     break;
   case 'J':
     (*env)->SetLongArrayRegion(env, jo->jobj, n, 1, &jjv.j);
     break;
   case 'F':
     (*env)->SetFloatArrayRegion(env, jo->jobj, n, 1, &jjv.f);
     break;
   case 'D':
     (*env)->SetDoubleArrayRegion(env, jo->jobj, n, 1, &jjv.d);
     break;
   case 'L':
   case '[':
     (*env)->SetObjectArrayElement(env, jo->jobj, n, jjv.l);
     break;
  }

  free_jargs(&jjv, -1, &dorelease, &ty2, jo->jvm, env);

  jvm_vacate_env(jo->jvm, env);
}

static void f_javaarray_indices(INT32 args)
{
  struct jobj_storage *jo = THIS_JOBJ;
  JNIEnv *env;
  INT32 size;
  struct array *a;

  if((env = jvm_procure_env(jo->jvm))) {
    size = (*env)->GetArrayLength(env, jo->jobj);
    jvm_vacate_env(jo->jvm, env);
  } else
    size = 0;
  a = allocate_array_no_init(size,0);
  a->type_field=BIT_INT;
  while(--size>=0) {
    ITEM(a)[size].type=PIKE_T_INT;
    ITEM(a)[size].subtype=NUMBER_NUMBER;
    ITEM(a)[size].u.integer=size;
  }
  pop_n_elems(args);
  push_array(a);
}

static void f_javaarray_values(INT32 args)
{
  struct jobj_storage *jo = THIS_JOBJ;
  struct jarray_storage *ja = THIS_JARRAY;
  JNIEnv *env;

  if((env = jvm_procure_env(jo->jvm))) {
    INT32 i, size = (*env)->GetArrayLength(env, jo->jobj);
    pop_n_elems(args);
    if(ja->ty == 'L' || ja->ty == '[') {
      for(i=0; i<size; i++)
	push_java_anyobj((*env)->GetObjectArrayElement(env, jo->jobj, i),
			 jo->jvm, env);
      f_aggregate(size);
    } else {
      struct array *ar = allocate_array_no_init(size, 0);
      if(ar == NULL)
	push_int(0);
      else {
	void *a = (*env)->GetPrimitiveArrayCritical(env, jo->jobj, 0);
	if(a == NULL) {
	  free_array(ar);
	  push_int(0);
	} else {
	  switch(ja->ty) {
	  case 'Z':
	    ar->type_field=BIT_INT;
	    for(i=0; i<size; i++) {
	      ITEM(ar)[i].type = PIKE_T_INT;
	      ITEM(ar)[i].subtype = NUMBER_NUMBER;
	      ITEM(ar)[i].u.integer = ((jboolean*)a)[i];
	    }
	    break;
	  case 'B':
	    ar->type_field=BIT_INT;
	    for(i=0; i<size; i++) {
	      ITEM(ar)[i].type = PIKE_T_INT;
	      ITEM(ar)[i].subtype = NUMBER_NUMBER;
	      ITEM(ar)[i].u.integer = ((jbyte*)a)[i];
	    }
	    break;
	  case 'C':
	    ar->type_field=BIT_INT;
	    for(i=0; i<size; i++) {
	      ITEM(ar)[i].type = PIKE_T_INT;
	      ITEM(ar)[i].subtype = NUMBER_NUMBER;
	      ITEM(ar)[i].u.integer = ((jchar*)a)[i];
	    }
	    break;
	  case 'S':
	    ar->type_field=BIT_INT;
	    for(i=0; i<size; i++) {
	      ITEM(ar)[i].type = PIKE_T_INT;
	      ITEM(ar)[i].subtype = NUMBER_NUMBER;
	      ITEM(ar)[i].u.integer = ((jshort*)a)[i];
	    }
	    break;
	  case 'I':
	  default:
	    ar->type_field=BIT_INT;
	    for(i=0; i<size; i++) {
	      ITEM(ar)[i].type = PIKE_T_INT;
	      ITEM(ar)[i].subtype = NUMBER_NUMBER;
	      ITEM(ar)[i].u.integer = ((jint*)a)[i];
	    }
	    break;
	  case 'J':
	    ar->type_field=BIT_INT;
	    for(i=0; i<size; i++) {
	      ITEM(ar)[i].type = PIKE_T_INT;
	      ITEM(ar)[i].subtype = NUMBER_NUMBER;
	      ITEM(ar)[i].u.integer = ((jlong*)a)[i];
	    }
	    break;
	  case 'F':
	    ar->type_field=BIT_FLOAT;
	    for(i=0; i<size; i++) {
	      ITEM(ar)[i].type = PIKE_T_FLOAT;
	      ITEM(ar)[i].u.float_number = ((jfloat*)a)[i];
	    }
	    break;
	  case 'D':
	    ar->type_field=BIT_FLOAT;
	    for(i=0; i<size; i++) {
	      ITEM(ar)[i].type = PIKE_T_FLOAT;
	      ITEM(ar)[i].u.float_number = ((jdouble*)a)[i];
	    }
	    break;	    
	  }
	  (*env)->ReleasePrimitiveArrayCritical(env, jo->jobj, a, 0);
	  push_array(ar);
	}
      }
    }
    jvm_vacate_env(jo->jvm, env);
  } else {
    pop_n_elems(args);
    push_int(0);
  }
}


/* Attachment */

#ifdef _REENTRANT

static void init_att_struct(struct object *o)
{
  struct att_storage *att = THIS_ATT;
  att->jvm = NULL;
  att->env = NULL;
  clear_svalues(&att->thr, 1);
}

static void exit_att_struct(struct object *o)
{
  struct att_storage *att = THIS_ATT;

  if(att->jvm) {
    struct jvm_storage *j =
      (struct jvm_storage *)get_storage(att->jvm, jvm_program);
    if(att->env) {
      THREAD_T me = th_self();
      if(!th_equal(me, att->tid))
	/* Hum hum.  This should (hopefully) only happen at exit time
	   when we're destructing all objects.  In this case it should be
	   safe just to ignore detaching, as the JVM itself will be destroyed
	   within moments... */
	;
      else
	(*j->jvm)->DetachCurrentThread(j->jvm);
    }
    free_object(att->jvm);
  }
  free_svalue(&att->thr);
}

static void att_gc_check(struct object *o)
{
  struct att_storage *att = THIS_ATT;

  if(att->jvm)
    gc_check(att->jvm);
  gc_check_svalues(&att->thr, 1);
}

static void att_gc_recurse(struct object *o)
{
  struct att_storage *att = THIS_ATT;

  if(att->jvm)
    gc_recurse_object(att->jvm);
  gc_recurse_svalues(&att->thr, 1);
}

static void f_att_create(INT32 args)
{
  struct object *j;
  struct jvm_storage *jvm;
  struct att_storage *att = THIS_ATT;

  get_all_args("Java.attachment->create()", args, "%o", &j);

  if((jvm = (struct jvm_storage *)get_storage(j, jvm_program))==NULL)
    Pike_error("Bad argument 1 to create().\n");

  att->jvm = j;
  j->refs++;
  pop_n_elems(args);
  f_this_thread(0);
  assign_svalue(&att->thr, &Pike_sp[-1]);
  pop_n_elems(1);
  att->args.version = JNI_VERSION_1_2;
  att->args.name = NULL;
  att->args.group = NULL;

  att->tid = th_self();
  if((*jvm->jvm)->AttachCurrentThread(jvm->jvm, (void *)&att->env, &att->args)<0)
    destruct(Pike_fp->current_object);
}

#endif /* _REENTRANT */


/* Monitor */

static void init_monitor_struct(struct object *o)
{
  struct monitor_storage *m = THIS_MONITOR;
  m->obj = NULL;
}

static void exit_monitor_struct(struct object *o)
{
  JNIEnv *env;
  struct monitor_storage *m = THIS_MONITOR;
  struct jobj_storage *j;

  if ((m->obj && (j=THAT_JOBJ(m->obj)))) {
#ifdef _REENTRANT
    THREAD_T me = th_self();
    if (!th_equal(me, m->tid))
      ;
    else
#endif /* _REENTRANT */
      if ((env = jvm_procure_env(j->jvm)) != NULL) {
	(*env)->MonitorExit(env, j->jobj);
	jvm_vacate_env(j->jvm, env);
      }
  }
  if (m->obj)
    free_object(m->obj);
}

static void monitor_gc_check(struct object *o)
{
  struct monitor_storage *m = THIS_MONITOR;

  if(m->obj)
    gc_check(m->obj);
}

static void monitor_gc_recurse(struct object *o)
{
  struct monitor_storage *m = THIS_MONITOR;

  if(m->obj)
    gc_recurse_object(m->obj);
}

static void f_monitor_create(INT32 args)
{
  struct monitor_storage *m=THIS_MONITOR;
  struct object *obj;

  get_all_args("Java.monitor->create()", args, "%o", &obj);

  if(get_storage(obj, jobj_program) == NULL)
    Pike_error("Bad argument 1 to create().\n");

#ifdef _REENTRANT
  m->tid = th_self();
#endif /* _REENTRANT */

  m->obj = obj;
  add_ref(obj);
  pop_n_elems(args);
  return;
}


/* JVM */


static void f_create(INT32 args)
{
  struct jvm_storage *j = THIS_JVM;
  char *classpath = NULL;
  jclass cls;

  while(j->jvm) {
    JavaVM *jvm = j->jvm;
    void *env;
    j->jvm = NULL;
    THREADS_ALLOW();
    (*jvm)->AttachCurrentThread(jvm, &env, NULL);
    (*jvm)->DestroyJavaVM(jvm);
    THREADS_DISALLOW();
  }

  j->vm_args.version = 0x00010002; /* Java 1.2. */
  j->vm_args.nOptions = 0;
  j->vm_args.options = j->vm_options;
  j->vm_args.ignoreUnrecognized = JNI_TRUE;

  /* Set classpath */
  if(args>0 && Pike_sp[-args].type == PIKE_T_STRING) {
    classpath = Pike_sp[-args].u.string->str;
    copy_shared_string(j->classpath_string, Pike_sp[-args].u.string);
  } else {
    if(getenv("CLASSPATH"))
      classpath = getenv("CLASSPATH");
#if 0
#ifdef JAVA_HOME
    else
      classpath = ".:"JAVA_HOME"/classes:"JAVA_HOME"/lib/classes.zip:"
	JAVA_HOME"/lib/rt.jar:"JAVA_HOME"/lib/i18n.jar";
#endif /* JAVA_HOME */
#else
    else
      classpath = ".";
#endif
    if(classpath != NULL)
      j->classpath_string = make_shared_string(classpath);
  }
  if(classpath != NULL) {
    push_text("-Djava.class.path=");
    push_string(j->classpath_string);
    j->classpath_string = NULL;
    f_add(2);
    copy_shared_string(j->classpath_string, Pike_sp[-1].u.string);
    pop_n_elems(1);
    j->vm_args.options[j->vm_args.nOptions].optionString =
      j->classpath_string->str;
    j->vm_args.options[j->vm_args.nOptions].extraInfo = NULL;
    j->vm_args.nOptions++;
  }
#ifndef __NT__
#ifdef JAVA_LIBPATH
  j->vm_args.options[j->vm_args.nOptions].optionString =
    "-Djava.library.path="JAVA_LIBPATH;
  j->vm_args.options[j->vm_args.nOptions].extraInfo = NULL;
  j->vm_args.nOptions++;
#endif
#endif

  /* load and initialize a Java VM, return a JNI interface 
   * pointer in env */
  if(JNI_CreateJavaVM(&j->jvm, (void**)&j->env, &j->vm_args))
    Pike_error( "Failed to create virtual machine\n" );

  /* Java tries to be a wiseguy with the locale... */
#ifdef HAVE_SETLOCALE
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
#endif

  cls = (*j->env)->FindClass(j->env, "java/lang/Object");
  j->class_object = (*j->env)->NewGlobalRef(j->env, cls);
  (*j->env)->DeleteLocalRef(j->env, cls);
  cls = (*j->env)->FindClass(j->env, "java/lang/Class");
  j->class_class = (*j->env)->NewGlobalRef(j->env, cls);
  (*j->env)->DeleteLocalRef(j->env, cls);
  cls = (*j->env)->FindClass(j->env, "java/lang/String");
  j->class_string = (*j->env)->NewGlobalRef(j->env, cls);
  (*j->env)->DeleteLocalRef(j->env, cls);
  cls = (*j->env)->FindClass(j->env, "java/lang/Throwable");
  j->class_throwable = (*j->env)->NewGlobalRef(j->env, cls);
  (*j->env)->DeleteLocalRef(j->env, cls);
  cls = (*j->env)->FindClass(j->env, "java/lang/RuntimeException");
  j->class_runtimex = (*j->env)->NewGlobalRef(j->env, cls);
  (*j->env)->DeleteLocalRef(j->env, cls);
  cls = (*j->env)->FindClass(j->env, "java/lang/System");
  j->class_system = (*j->env)->NewGlobalRef(j->env, cls);
  (*j->env)->DeleteLocalRef(j->env, cls);

  j->method_hash =
    (*j->env)->GetMethodID(j->env, j->class_object, "hashCode", "()I");
  j->method_tostring =
    (*j->env)->GetMethodID(j->env, j->class_object, "toString",
			   "()Ljava/lang/String;");
  j->method_arraycopy =
    (*j->env)->GetStaticMethodID(j->env, j->class_system, "arraycopy",
				 "(Ljava/lang/Object;ILjava/lang/Object;II)V");
  j->method_getcomponenttype =
    (*j->env)->GetMethodID(j->env, j->class_class, "getComponentType",
			   "()Ljava/lang/Class;");
  j->method_isarray =
    (*j->env)->GetMethodID(j->env, j->class_class, "isArray", "()Z");
  j->method_getname =
    (*j->env)->GetMethodID(j->env, j->class_class, "getName",
			   "()Ljava/lang/String;");
  j->method_charat =
    (*j->env)->GetMethodID(j->env, j->class_string, "charAt", "(I)C");

#ifdef _REENTRANT
  f_thread_local(0);
  if(Pike_sp[-1].type == PIKE_T_OBJECT) {
    j->tl_env = Pike_sp[-1].u.object;
    add_ref(j->tl_env);
  }
  pop_n_elems(args+1);
#else
  pop_n_elems(args);
#endif /* _REENTRANT */
  push_int(0);
}

static void init_jvm_struct(struct object *o)
{
  struct jvm_storage *j = THIS_JVM;

#ifdef SUPPORT_NATIVE_METHODS
  num_threads++;
#endif /* SUPPORT_NATIVE_METHODS */
  j->jvm = NULL;
  j->classpath_string = NULL;
  j->class_object = 0;
  j->class_class = 0;
  j->class_string = 0;
  j->class_throwable = 0;
  j->class_runtimex = 0;
  j->class_system = 0;
#ifdef _REENTRANT
  j->tl_env = NULL;
#endif /* _REENTRANT */
}

static void exit_jvm_struct(struct object *o)
{
  struct jvm_storage *j = THIS_JVM;
  JNIEnv *env = NULL;
  void *tmpenv = NULL;

  if(j->jvm != NULL && (env = jvm_procure_env(Pike_fp->current_object))) {
    if(j->class_system)
      (*env)->DeleteGlobalRef(env, j->class_system);
    if(j->class_runtimex)
      (*env)->DeleteGlobalRef(env, j->class_runtimex);
    if(j->class_throwable)
      (*env)->DeleteGlobalRef(env, j->class_throwable);
    if(j->class_string)
      (*env)->DeleteGlobalRef(env, j->class_string);
    if(j->class_class)
      (*env)->DeleteGlobalRef(env, j->class_class);
    if(j->class_object)
      (*env)->DeleteGlobalRef(env, j->class_object);
    jvm_vacate_env(Pike_fp->current_object, env);
  }

  tmpenv = (void *)env;
  while(j->jvm) {
    JavaVM *jvm = j->jvm;
    j->jvm = NULL;
    THREADS_ALLOW();
    (*jvm)->AttachCurrentThread(jvm, &tmpenv, NULL);
    (*jvm)->DestroyJavaVM(jvm);
    THREADS_DISALLOW();
  }
  if(j->classpath_string)
    free_string(j->classpath_string);
#ifdef _REENTRANT
  if(j->tl_env != NULL)
    free_object(j->tl_env);
#endif /* _REENTRANT */
#ifdef SUPPORT_NATIVE_METHODS
  num_threads--;
#endif /* SUPPORT_NATIVE_METHODS */
}

#ifdef _REENTRANT
static void jvm_gc_check(struct object *o)
{
  struct jvm_storage *j = THIS_JVM;

  if(j->tl_env)
    gc_check(j->tl_env);
}

static void jvm_gc_recurse(struct object *o)
{
  struct jvm_storage *j = THIS_JVM;

  if(j->tl_env)
    gc_recurse_object(j->tl_env);
}
#endif /* _REENTRANT */


static void f_get_version(INT32 args)
{
  JNIEnv *env;
  pop_n_elems(args);
  if((env = jvm_procure_env(Pike_fp->current_object))) {
    push_int((*env)->GetVersion(env));
    jvm_vacate_env(Pike_fp->current_object, env);
  } else
    push_int(0);
}

static void f_find_class(INT32 args)
{
  JNIEnv *env;
  char *cn;
  jclass c;

  get_all_args("find_class", args, "%s", &cn);
  if((env = jvm_procure_env(Pike_fp->current_object))) {
    c = (*env)->FindClass(env, cn);
    pop_n_elems(args);
    push_java_class(c, Pike_fp->current_object, env);
    jvm_vacate_env(Pike_fp->current_object, env);
  } else {
    pop_n_elems(args);
    push_int(0);
  }
}

static void f_define_class(INT32 args)
{
  JNIEnv *env;
  struct object *obj;
  struct pike_string *str;
  struct jobj_storage *ldr;
  char *name;
  jclass c;

  get_all_args("define_class", args, "%s%o%S", &name, &obj, &str);
  if((ldr = THAT_JOBJ(obj))==NULL)
    Pike_error("Bad argument 2 to define_class().\n");
  if((env = jvm_procure_env(Pike_fp->current_object))) {
    c = (*env)->DefineClass(env, name, ldr->jobj, (jbyte*)str->str, str->len);
    pop_n_elems(args);
    push_java_class(c, Pike_fp->current_object, env);
    jvm_vacate_env(Pike_fp->current_object, env);
  } else {
    pop_n_elems(args);
    push_int(0);
  }
}

static void f_exception_check(INT32 args)
{
  JNIEnv *env;

  pop_n_elems(args);
  if((env = jvm_procure_env(Pike_fp->current_object))) {
    push_int((((*env)->ExceptionCheck(env))==JNI_TRUE? 1 : 0));
    jvm_vacate_env(Pike_fp->current_object, env);
  } else
    push_int(0);
}

static void f_exception_occurred(INT32 args)
{
  JNIEnv *env;

  pop_n_elems(args);
  if((env = jvm_procure_env(Pike_fp->current_object))) {
    push_java_throwable((*env)->ExceptionOccurred(env),
			Pike_fp->current_object, env);
    jvm_vacate_env(Pike_fp->current_object, env);
  } else
    push_int(0);
}

static void f_exception_describe(INT32 args)
{
  JNIEnv *env;

  pop_n_elems(args);
  if((env = jvm_procure_env(Pike_fp->current_object))) {
    (*env)->ExceptionDescribe(env);
    jvm_vacate_env(Pike_fp->current_object, env);
  }
  push_int(0);
}

static void f_exception_clear(INT32 args)
{
  JNIEnv *env;

  pop_n_elems(args);
  if((env = jvm_procure_env(Pike_fp->current_object))) {
    (*env)->ExceptionClear(env);
    jvm_vacate_env(Pike_fp->current_object, env);
  }
  push_int(0);
}

static void f_javafatal(INT32 args)
{
  JNIEnv *env;
  char *msg;

  get_all_args("fatal", args, "%s", &msg);
  if((env = jvm_procure_env(Pike_fp->current_object))) {
    (*env)->FatalError(env, msg);
    jvm_vacate_env(Pike_fp->current_object, env);
  }
  pop_n_elems(args);
  push_int(0);
}

static void f_new_boolean_array(INT32 args)
{
  JNIEnv *env;
  INT_TYPE n;

  get_all_args("new_boolean_array", args, "%i", &n);
  pop_n_elems(args);

  if((env = jvm_procure_env(Pike_fp->current_object))) {
    push_java_array((*env)->NewBooleanArray(env, n), Pike_fp->current_object,
		    env, 'Z');
    jvm_vacate_env(Pike_fp->current_object, env);
  } else
    push_int(0);
}

static void f_new_byte_array(INT32 args)
{
  JNIEnv *env;
  INT_TYPE n;

  get_all_args("new_byte_array", args, "%i", &n);
  pop_n_elems(args);

  if((env = jvm_procure_env(Pike_fp->current_object))) {
    push_java_array((*env)->NewByteArray(env, n), Pike_fp->current_object,
		    env, 'B');
    jvm_vacate_env(Pike_fp->current_object, env);
  } else
    push_int(0);
}

static void f_new_char_array(INT32 args)
{
  JNIEnv *env;
  INT_TYPE n;

  get_all_args("new_char_array", args, "%i", &n);
  pop_n_elems(args);

  if((env = jvm_procure_env(Pike_fp->current_object))) {
    push_java_array((*env)->NewCharArray(env, n), Pike_fp->current_object,
		    env, 'C');
    jvm_vacate_env(Pike_fp->current_object, env);
  } else
    push_int(0);
}

static void f_new_short_array(INT32 args)
{
  JNIEnv *env;
  INT_TYPE n;

  get_all_args("new_short_array", args, "%i", &n);
  pop_n_elems(args);

  if((env = jvm_procure_env(Pike_fp->current_object))) {
    push_java_array((*env)->NewShortArray(env, n), Pike_fp->current_object,
		    env, 'S');
    jvm_vacate_env(Pike_fp->current_object, env);
  } else
    push_int(0);
}

static void f_new_int_array(INT32 args)
{
  JNIEnv *env;
  INT_TYPE n;

  get_all_args("new_int_array", args, "%i", &n);
  pop_n_elems(args);

  if((env = jvm_procure_env(Pike_fp->current_object))) {
    push_java_array((*env)->NewIntArray(env, n), Pike_fp->current_object,
		    env, 'I');
    jvm_vacate_env(Pike_fp->current_object, env);
  } else
    push_int(0);
}

static void f_new_long_array(INT32 args)
{
  JNIEnv *env;
  INT_TYPE n;

  get_all_args("new_long_array", args, "%i", &n);
  pop_n_elems(args);

  if((env = jvm_procure_env(Pike_fp->current_object))) {
    push_java_array((*env)->NewLongArray(env, n), Pike_fp->current_object,
		    env, 'J');
    jvm_vacate_env(Pike_fp->current_object, env);
  } else
    push_int(0);
}

static void f_new_float_array(INT32 args)
{
  JNIEnv *env;
  INT_TYPE n;

  get_all_args("new_float_array", args, "%i", &n);
  pop_n_elems(args);

  if((env = jvm_procure_env(Pike_fp->current_object))) {
    push_java_array((*env)->NewFloatArray(env, n), Pike_fp->current_object,
		    env, 'F');
    jvm_vacate_env(Pike_fp->current_object, env);
  } else
    push_int(0);
}

static void f_new_double_array(INT32 args)
{
  JNIEnv *env;
  INT_TYPE n;

  get_all_args("new_double_array", args, "%i", &n);
  pop_n_elems(args);

  if((env = jvm_procure_env(Pike_fp->current_object))) {
    push_java_array((*env)->NewDoubleArray(env, n), Pike_fp->current_object,
		    env, 'D');
    jvm_vacate_env(Pike_fp->current_object, env);
  } else
    push_int(0);
}

#endif /* HAVE_JAVA */

PIKE_MODULE_INIT
{
#ifdef HAVE_JAVA
  struct svalue prog;
  prog.type = PIKE_T_PROGRAM;
  prog.subtype = 0;

#ifdef __NT__
  if (open_nt_dll()<0)
    return;
#endif /* __NT__ */

#ifdef HAVE_IBMFINDDLL
  {
    /* Debug... */
    extern char *ibmFindDLL(void);
    fprintf(stderr, "ibmFindDLL(): \"%s\"\n", ibmFindDLL());
  }
#endif

  /* Restore any signal handlers that may have been zapped
   * when the jvm was loaded.
   */
  low_init_signals();

  start_new_program();
  ADD_STORAGE(struct jobj_storage);
  ADD_FUNCTION("cast", f_jobj_cast, tFunc(tStr,tMix), 0);
  ADD_FUNCTION("`==", f_jobj_eq, tFunc(tMix,tInt01), 0);
  ADD_FUNCTION("__hash", f_jobj_hash, tFunc(tNone,tInt), 0);
  ADD_FUNCTION("is_instance_of", f_jobj_instance, tFunc(tObj,tInt01), 0);
  ADD_FUNCTION("monitor_enter", f_monitor_enter, tFunc(tNone,tObj), 0);
  ADD_FUNCTION("get_object_class", f_jobj_get_class, tFunc(tNone,tObj), 0);
  set_init_callback(init_jobj_struct);
  set_exit_callback(exit_jobj_struct);
  set_gc_check_callback(jobj_gc_check);
  set_gc_recurse_callback(jobj_gc_recurse);
  jobj_program = end_program();
  jobj_program->flags |= PROGRAM_DESTRUCT_IMMEDIATE;

  start_new_program();
  prog.u.program = jobj_program;
  do_inherit(&prog, 0, NULL);
  ADD_FUNCTION("super_class", f_super_class, tFunc(tNone,tObj), 0);
  ADD_FUNCTION("is_assignable_from", f_is_assignable_from,
	       tFunc(tObj,tInt), 0);
  ADD_FUNCTION("throw_new", f_throw_new, tFunc(tStr,tVoid), 0);
  ADD_FUNCTION("alloc", f_alloc, tFunc(tNone,tObj), 0);
  ADD_FUNCTION("new_array", f_new_array, tFunc(tInt tOr(tObj,tVoid),tObj), 0);
  ADD_FUNCTION("get_method", f_get_method, tFunc(tStr tStr,tObj), 0);
  ADD_FUNCTION("get_static_method", f_get_static_method,
	       tFunc(tStr tStr,tObj), 0);
  ADD_FUNCTION("get_field", f_get_field, tFunc(tStr tStr,tObj), 0);
  ADD_FUNCTION("get_static_field", f_get_static_field,
	       tFunc(tStr tStr,tObj), 0);
#ifdef SUPPORT_NATIVE_METHODS
  ADD_FUNCTION("register_natives", f_register_natives,
	       tFunc(tArr(tArr(tOr(tStr,tFunction))),tObj), 0);
#endif /* SUPPORT_NATIVE_METHODS */
  jclass_program = end_program();
  jclass_program->flags |= PROGRAM_DESTRUCT_IMMEDIATE;

  start_new_program();
  do_inherit(&prog, 0, NULL);
  ADD_FUNCTION("throw", f_javathrow, tFunc(tNone,tVoid), 0);
  jthrowable_program = end_program();
  jthrowable_program->flags |= PROGRAM_DESTRUCT_IMMEDIATE;

  start_new_program();
  do_inherit(&prog, 0, NULL);
  jarray_stor_offs = ADD_STORAGE(struct jarray_storage);
  ADD_FUNCTION("_sizeof", f_javaarray_sizeof, tFunc(tNone,tInt), 0);
  ADD_FUNCTION("`[]", f_javaarray_getelt,
	       tFunc(tInt tOr(tInt,tVoid), tMix), 0);
  ADD_FUNCTION("`[]=", f_javaarray_setelt, tFunc(tInt tMix,tMix), 0);
  ADD_FUNCTION("_indices", f_javaarray_indices, tFunc(tNone,tArr(tInt)), 0);
  ADD_FUNCTION("_values", f_javaarray_values, tFunc(tNone,tArray), 0);
  jarray_program = end_program();
  jarray_program->flags |= PROGRAM_DESTRUCT_IMMEDIATE;

  start_new_program();
  ADD_STORAGE(struct method_storage);
  ADD_FUNCTION("create", f_method_create, tFunc(tStr tStr tObj,tVoid), 0);
  ADD_FUNCTION("`()", f_call_static, tFuncV(tNone,tMix,tMix), 0);
  set_init_callback(init_method_struct);
  set_exit_callback(exit_method_struct);
  set_gc_check_callback(method_gc_check);
  set_gc_recurse_callback(method_gc_recurse);
  static_method_program = end_program();
  static_method_program->flags |= PROGRAM_DESTRUCT_IMMEDIATE;

  start_new_program();
  ADD_STORAGE(struct method_storage);
  ADD_FUNCTION("create", f_method_create, tFunc(tStr tStr tObj,tVoid), 0);
  ADD_FUNCTION("`()", f_call_virtual, tFuncV(tObj,tMix,tMix), 0);
  ADD_FUNCTION("call_nonvirtual", f_call_nonvirtual, tFuncV(tObj,tMix,tMix),0);
  set_init_callback(init_method_struct);
  set_exit_callback(exit_method_struct);
  set_gc_check_callback(method_gc_check);
  set_gc_recurse_callback(method_gc_recurse);
  method_program = end_program();
  method_program->flags |= PROGRAM_DESTRUCT_IMMEDIATE;

  start_new_program();
  ADD_STORAGE(struct field_storage);
  ADD_FUNCTION("create", f_field_create, tFunc(tStr tStr tObj,tVoid), 0);
  ADD_FUNCTION("set", f_field_set, tFunc(tObj tMix,tMix), 0);
  ADD_FUNCTION("get", f_field_get, tFunc(tObj,tMix), 0);
  set_init_callback(init_field_struct);
  set_exit_callback(exit_field_struct);
  set_gc_check_callback(field_gc_check);
  set_gc_recurse_callback(field_gc_recurse);
  field_program = end_program();
  field_program->flags |= PROGRAM_DESTRUCT_IMMEDIATE;

  start_new_program();
  ADD_STORAGE(struct field_storage);
  ADD_FUNCTION("create", f_field_create, tFunc(tStr tStr tObj,tVoid), 0);
  ADD_FUNCTION("set", f_static_field_set, tFunc(tMix,tMix), 0);
  ADD_FUNCTION("get", f_static_field_get, tFunc(tNone,tMix), 0);
  set_init_callback(init_field_struct);
  set_exit_callback(exit_field_struct);
  set_gc_check_callback(field_gc_check);
  set_gc_recurse_callback(field_gc_recurse);
  static_field_program = end_program();
  static_field_program->flags |= PROGRAM_DESTRUCT_IMMEDIATE;

  start_new_program();
  ADD_STORAGE(struct monitor_storage);
  ADD_FUNCTION("create", f_monitor_create, tFunc(tObj,tVoid), 0);
  set_init_callback(init_monitor_struct);
  set_exit_callback(exit_monitor_struct);
  set_gc_check_callback(monitor_gc_check);
  set_gc_recurse_callback(monitor_gc_recurse);
  monitor_program = end_program();
  monitor_program->flags |= PROGRAM_DESTRUCT_IMMEDIATE;

#ifdef SUPPORT_NATIVE_METHODS
  start_new_program();
  ADD_STORAGE(struct natives_storage);
  ADD_FUNCTION("create", f_natives_create,
	       tFunc(tArr(tArr(tOr(tStr,tFunction))) tObj,tVoid), 0);
  set_init_callback(init_natives_struct);
  set_exit_callback(exit_natives_struct);
  set_gc_check_callback(natives_gc_check);
  set_gc_recurse_callback(natives_gc_recurse);
  natives_program = end_program();
  natives_program->flags |= PROGRAM_DESTRUCT_IMMEDIATE;
#endif /* SUPPORT_NATIVE_METHODS */

#ifdef _REENTRANT
  start_new_program();
  ADD_STORAGE(struct att_storage);
  ADD_FUNCTION("create", f_att_create, tFunc(tObj,tVoid), 0);
  set_init_callback(init_att_struct);
  set_exit_callback(exit_att_struct);
  set_gc_check_callback(att_gc_check);
  set_gc_recurse_callback(att_gc_recurse);
  attachment_program = end_program();
  attachment_program->flags |= PROGRAM_DESTRUCT_IMMEDIATE;
#endif /* _REENTRANT */

  start_new_program();
  ADD_STORAGE(struct jvm_storage);
  ADD_FUNCTION("create", f_create, tFunc(tOr(tStr,tVoid),tVoid), 0);
  ADD_FUNCTION("get_version", f_get_version, tFunc(tNone,tInt), 0);
  ADD_FUNCTION("find_class", f_find_class, tFunc(tStr,tObj), 0);
  ADD_FUNCTION("define_class", f_define_class, tFunc(tStr tObj tStr,tObj), 0);
  ADD_FUNCTION("exception_check", f_exception_check, tFunc(tNone,tInt), 0);
  ADD_FUNCTION("exception_occurred", f_exception_occurred,
	       tFunc(tNone,tObj), 0);
  ADD_FUNCTION("exception_describe", f_exception_describe,
	       tFunc(tNone,tVoid), 0);
  ADD_FUNCTION("exception_clear", f_exception_clear, tFunc(tNone,tVoid), 0);
  ADD_FUNCTION("fatal", f_javafatal, tFunc(tStr,tVoid), 0);
  ADD_FUNCTION("new_boolean_array", f_new_boolean_array, tFunc(tInt,tObj), 0);
  ADD_FUNCTION("new_byte_array", f_new_byte_array, tFunc(tInt,tObj), 0);
  ADD_FUNCTION("new_char_array", f_new_char_array, tFunc(tInt,tObj), 0);
  ADD_FUNCTION("new_short_array", f_new_short_array, tFunc(tInt,tObj), 0);
  ADD_FUNCTION("new_int_array", f_new_int_array, tFunc(tInt,tObj), 0);
  ADD_FUNCTION("new_long_array", f_new_long_array, tFunc(tInt,tObj), 0);
  ADD_FUNCTION("new_float_array", f_new_float_array, tFunc(tInt,tObj), 0);
  ADD_FUNCTION("new_double_array", f_new_double_array, tFunc(tInt,tObj), 0);
  set_init_callback(init_jvm_struct);
  set_exit_callback(exit_jvm_struct);
#ifdef _REENTRANT
  set_gc_check_callback(jvm_gc_check);
  set_gc_recurse_callback(jvm_gc_recurse);
#endif /* _REENTRANT */
  add_program_constant("jvm", jvm_program = end_program(), 0);
  jvm_program->flags |= PROGRAM_DESTRUCT_IMMEDIATE;
#endif /* HAVE_JAVA */
}

PIKE_MODULE_EXIT
{
#ifdef HAVE_JAVA
  if(jarray_program) {
    free_program(jarray_program);
    jarray_program=NULL;
  }
  if(jthrowable_program) {
    free_program(jthrowable_program);
    jthrowable_program=NULL;
  }
  if(jclass_program) {
    free_program(jclass_program);
    jclass_program=NULL;
  }
  if(jobj_program) {
    free_program(jobj_program);
    jobj_program=NULL;
  }
  if(static_field_program) {
    free_program(static_field_program);
    static_field_program=NULL;
  }
  if(field_program) {
    free_program(field_program);
    field_program=NULL;
  }
  if(static_method_program) {
    free_program(static_method_program);
    static_method_program=NULL;
  }
  if(method_program) {
    free_program(method_program);
    method_program=NULL;
  }
  if(monitor_program) {
    free_program(monitor_program);
    monitor_program=NULL;
  }
  if(natives_program) {
    free_program(natives_program);
    natives_program=NULL;
  }
  if(attachment_program) {
    free_program(attachment_program);
    attachment_program=NULL;
  }
  if(jvm_program) {
    free_program(jvm_program);
    jvm_program=NULL;
  }
#ifdef __NT__
  close_nt_dll();
#endif /* __NT __ */
#endif /* HAVE_JAVA */
}
