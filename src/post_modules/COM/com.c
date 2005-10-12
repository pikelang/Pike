/*
 * $Id: com.c,v 1.2 2005/10/12 13:41:32 grubba Exp $
 *
 * Pike interface to Common Object Model (COM)
 *
 * Tomas Nilsson
 */

/*
 * Includes
 */

#define COM_DEBUG
#define NO_PIKE_SHORTHAND

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "global.h"
RCSID("$Id: com.c,v 1.2 2005/10/12 13:41:32 grubba Exp $");
#include "program.h"
#include "interpret.h"
#include "stralloc.h"
#include "object.h"
#include "mapping.h"
#include "builtin_functions.h"
#include "pike_error.h"
#include "module_support.h"
#include "pike_memory.h"
#include "gc.h"
#include "threads.h"
#include "operators.h"

#ifdef HAVE_COM

#ifdef HAVE_OBJBASE_H
#include <objbase.h>
#endif /* HAVE_OBJBASE_H */

#ifdef HAVE_WINBASE_H
#include <winbase.h>
#endif /* HAVE_WINBASE_H */

#ifdef USE_COM_PROG
static struct program *com_program = NULL;
#endif /* USE_COM_PROG */
static struct program *cobj_program = NULL;
static struct program *cval_program = NULL;

#ifdef USE_COM_PROG
struct com_storage {
  int dummy;
};
#endif /* USE_COM_PROG */

struct cobj_storage {
  IDispatch      *pIDispatch;
  struct mapping *method_map;
};

struct cval_storage {
  IDispatch *pIDispatch;
  struct pike_string *method;
  DISPID     dispid;
};


#ifdef USE_COM_PROG
#define THIS_COM  ((struct com_storage *)(Pike_fp->current_storage))
#endif /* USE_COM_PROG */

#define THIS_COBJ ((struct cobj_storage *)(Pike_fp->current_storage))
#define THIS_CVAL ((struct cval_storage *)(Pike_fp->current_storage))


#define TYPEINFO_FUNC_ALL        0x00FF
#define TYPEINFO_FUNC_FUNC       0x0001
#define TYPEINFO_FUNC_GET        0x0002
#define TYPEINFO_FUNC_PUT        0x0004
#define TYPEINFO_FUNC_PUTREF     0x0008
#define TYPEINFO_VAR_ALL         0xFF00
#define TYPEINFO_VAR_PERINSTANCE 0x0100
#define TYPEINFO_VAR_STATIC      0x0200
#define TYPEINFO_VAR_CONST       0x0400
#define TYPEINFO_VAR_DISPATCH    0x0800


/*
TODO:
*/

/********************/
/* support routines */
/********************/

void DisplayTypeInfo( LPTYPEINFO pITypeInfo );
static void stack_swap2(int args);
static void com_throw_error(HRESULT hr);
static void com_throw_error2(HRESULT hr, EXCEPINFO excep);
static void set_variant_arg(VARIANT *v, struct svalue *sv);
static void create_variant_arg(int args, DISPPARAMS **dpar);
static void free_variant_arg(DISPPARAMS **dpar);
static void low_push_safearray(SAFEARRAY *psa, UINT dims,
                               long *indices, UINT curdim);
static void push_safearray(SAFEARRAY *psa);
static void push_varg(VARIANT *varg);
static int push_typeinfo_members(ITypeInfo *pITypeInfo,
                                 TYPEATTR *pTypeAttr, int flags);
static void cval_push_result(INT32 args, int flags);



static void stack_swap2(int args)
{
  int i;
  struct svalue tmp;

  if (args < 2)
    return;

  tmp = Pike_sp[-1];
  for (i=1; i<args; i++)
    Pike_sp[-i] = Pike_sp[-i-1];

  Pike_sp[-args] = tmp;
}

static void com_throw_error(HRESULT hr)
{
  LPVOID lpMsgBuf;
  char buf[1024];
  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM | 
                FORMAT_MESSAGE_IGNORE_INSERTS, NULL, hr,
                MAKELANGID(LANG_NEUTRAL, 
                           SUBLANG_DEFAULT),(LPTSTR) &lpMsgBuf,
                0, NULL);
  _snprintf(buf, sizeof(buf), "%s", lpMsgBuf);
  LocalFree( lpMsgBuf );
  Pike_error("Com Error: %s\n", buf);
}

static void com_throw_error2(HRESULT hr, EXCEPINFO excep)
{
    if (hr==DISP_E_EXCEPTION && excep.bstrDescription)
    {
        char errDesc[512];
        char errMsg[512];
        wcstombs(errDesc, excep.bstrDescription, 512);
        sprintf(errMsg, "Run-time error %d:\n\n %s", 
                excep.scode & 0x0000FFFF,  //Lower 16-bits of SCODE
                errDesc);                  //Text error description
        Pike_error("Server Error:%s\n", errMsg);
    }
    else
      com_throw_error(hr);
}

static void set_variant_arg(VARIANT *v, struct svalue *sv)
{
  OLECHAR *sz;
  PCHARP pchar;

  VariantInit(v);
      
  switch(sv->type)
  {
    case PIKE_T_INT:
      v->vt = VT_I4;
      v->lVal = sv->u.integer;
      break;

    case PIKE_T_FLOAT:
#ifndef WITH_DOUBLE_PRECISION_SVALUE
      v->vt = VT_R4;
      v->fltVal = sv->u.float_number;
#else
#ifdef WITH_LONG_DOUBLE_PRECISION_SVALUE
      v->vt = VT_R8;
      v->dblVal = (DOUBLE)sv->u.float_number;
#else
      v->vt = VT_R8;
      v->dblVal = sv->u.float_number;
#endif /* long double */
#endif /* double */
      break;

    case PIKE_T_STRING:
      pchar = MKPCHARP(malloc(sv->u.string->len * 2 + 2), 1);
      pike_string_cpy(pchar, sv->u.string);
      SET_INDEX_PCHARP(pchar, sv->u.string->len, 0);
      v->vt = VT_BSTR;
      v->bstrVal = SysAllocString((OLECHAR *)pchar.ptr);
      free(pchar.ptr);
      break;

    case PIKE_T_OBJECT:
      if (sv->u.object->prog == cobj_program)
      {
        struct cobj_storage *co =
          (struct cobj_storage *)sv->u.object->storage;
        v->pdispVal = co->pIDispatch;
        v->pdispVal->lpVtbl->AddRef(v->pdispVal);
        v->vt = VT_DISPATCH;
      }
      else if (sv->u.object->prog == cval_program)
      {
        apply(sv->u.object, "_value", 0);
        set_variant_arg(v, &Pike_sp[-1]);
        pop_stack();
      }
      else
        Pike_error("Com: create_variant: Pike object can't be converted!\n");
      break;

      /* TODO: Convert array to SAFEARRAY ? */
    case PIKE_T_ARRAY:
      Pike_error("Com: create_variant: Pike array can't be converted!\n",
                 sv->type);
      break;

    case PIKE_T_MAPPING:
      Pike_error("Com: create_variant: Pike mapping can't be converted!\n",
                 sv->type);
      break;

    case PIKE_T_MULTISET:
      Pike_error("Com: create_variant: Pike multiset can't be converted!\n",
                 sv->type);
      break;

    default:
      Pike_error("Com: create_variant: Pike type %d can't be converted!\n",
                 sv->type);
      break;
  }
}

static void create_variant_arg(int args, DISPPARAMS **dpar)
{
  VARIANTARG *varg=NULL;

  if (args > 0)
  {
    int i;
    varg=malloc(args * sizeof(VARIANTARG));
    for (i=0; i<args; i++)
      set_variant_arg(&varg[args-i-1], &Pike_sp[i-args]);
  }
  
  *dpar = malloc(sizeof(DISPPARAMS));
  (*dpar)->cNamedArgs = 0;
  (*dpar)->rgdispidNamedArgs = NULL;
  (*dpar)->cArgs = args;
  (*dpar)->rgvarg = varg;
}

static void free_variant_arg(DISPPARAMS **dpar)
{
  int i;

  for (i=0; i<(*dpar)->cArgs; i++)
  {
    VariantClear(&(*dpar)->rgvarg[i]);
  }
  free((*dpar)->rgvarg);
  free(*dpar);
  *dpar = NULL;
}

static HRESULT invoke(struct cobj_storage *cobj,
                      OLECHAR *method,
                      WORD flags,
                      DISPPARAMS *dpar,
                      VARIANT *pRes,
                      EXCEPINFO *pExc,
                      unsigned *pArgErr)
{
  DISPID dispid;
  HRESULT hr;
  DISPID dispidNamed = DISPID_PROPERTYPUT;

  if (flags == DISPATCH_PROPERTYPUT)
  {
    dpar->cNamedArgs = 1;
    dpar->rgdispidNamedArgs = &dispidNamed;
  }

  hr = cobj->pIDispatch->lpVtbl->GetIDsOfNames(cobj->pIDispatch, &IID_NULL,
                                          &method, 1,
                                          GetUserDefaultLCID(), &dispid);

  hr = cobj->pIDispatch->lpVtbl->Invoke(cobj->pIDispatch,
                                        dispid,
                                        &IID_NULL,
                                        GetUserDefaultLCID(),
                                        flags,
                                        dpar,
                                        pRes,
                                        pExc,
                                        pArgErr);

  return hr;
}


static void low_push_safearray(SAFEARRAY *psa, UINT dims,
                               long *indices, UINT curdim)
{
  VARTYPE vtype;
  HRESULT hr;
  long i;
  long lbound, ubound;
  int push_count = 0;

  SafeArrayLock(psa);
  SafeArrayGetVartype(psa, &vtype);
  
  if (vtype != VT_VARIANT)
  {
    /* TODO: Handle more array types */
    Pike_error("Unknown vartype: %d\n", vtype);
    /* Not reached! */
    push_int(0);
    Pike_sp[-1].subtype = NUMBER_UNDEFINED;
    return;
  }

  SafeArrayGetLBound(psa, curdim, &lbound);
  SafeArrayGetUBound(psa, curdim, &ubound);
  for (i=lbound; i<=ubound; i++)
  {
    indices[dims-curdim] = i;
    if (curdim < dims)
    {
      low_push_safearray(psa, dims, indices, curdim+1);
      push_count++;
    }
    else
    {
      if (vtype == VT_VARIANT)
      {
        VARIANT vt;
        VariantInit(&vt);
        SafeArrayGetElement(psa, indices, &vt);
        push_varg(&vt);
        push_count++;
        VariantClear(&vt);
      }
      else
      {
        /* TODO: Handle more array types */
      }
    }
  }
  f_aggregate(push_count);
}

#define MAX_DIMENSIONS 100
static void push_safearray(SAFEARRAY *psa)
{
  UINT dim;
  long indices[MAX_DIMENSIONS]; /* Maximum number of dimensions handled */

  SafeArrayLock(psa);
  dim = SafeArrayGetDim(psa);
  if (dim > MAX_DIMENSIONS)
    Pike_error("Com: push_safearray: Number of SafeArray dimensions %d > %d\n",
               dim, MAX_DIMENSIONS);

  low_push_safearray(psa, dim, indices, 1);

  SafeArrayUnlock(psa);
}

static void push_varg(VARIANT *varg)
{
  VARIANTARG cv;
  HRESULT hr;

  if (varg->vt & VT_ARRAY)
  {
    push_safearray(varg->parray);
    return;
  }
  
  VariantInit(&cv);
  switch (varg->vt & VT_TYPEMASK)
  {
    /*
	VT_ARRAY	= 0x2000,
	VT_BYREF	= 0x4000,
	VT_ILLEGAL	= 0xffff,
	VT_ILLEGALMASKED	= 0xfff,
	VT_TYPEMASK	= 0xfff
    */
    case VT_EMPTY:
      break;

    case VT_NULL:
      push_int(0);
      break;

    case VT_I2:
    case VT_I4:
    case VT_I1:
    case VT_UI1:
    case VT_UI2:
    case VT_UI4:
    case VT_INT:
    case VT_UINT:
    case VT_BOOL:
      hr = VariantChangeType(&cv, varg, 0, VT_INT);
      if (SUCCEEDED(hr))
        push_int(cv.intVal);
      else
        Pike_error("Com.obj: push_varg: Failed to convert result to int\n");
      break;

    case VT_R4:
    case VT_R8:
    case VT_DECIMAL:
      hr = VariantChangeType(&cv, varg, 0, VT_R8);
      if (SUCCEEDED(hr))
        push_float(cv.dblVal);
      else
        Pike_error("Com.obj: push_varg: Failed to convert result to float\n");
      break;

    case VT_BSTR:
    case VT_CY:
    case VT_DATE:
    case VT_VARIANT:
    case VT_UNKNOWN:
      hr = VariantChangeType(&cv, varg, 0, VT_BSTR);
      if (SUCCEEDED(hr))
        push_string(make_shared_string1(cv.bstrVal));
      else
        Pike_error("Com.obj: push_varg: Failed to convert result to string\n");
      break;

    case VT_DISPATCH:
      {
        struct object *oo;
        struct cobj_storage *co;

        hr = VariantChangeType(&cv, varg, 0, VT_DISPATCH);
        if (SUCCEEDED(hr))
        {
          if (cv.pdispVal)
          {
            oo=clone_object(cobj_program, 0);
            co = (struct cobj_storage *)(oo->storage);
            co->pIDispatch = cv.pdispVal;
            co->pIDispatch->lpVtbl->AddRef(co->pIDispatch);
            push_object(oo);
          }
          else
          {
            push_int(0);
            Pike_sp[-1].subtype = NUMBER_UNDEFINED;
          }
        }
        else
          Pike_error("Com.obj: push_varg: Failed to convert result to IDispatch\n");
      }
      break;

    case VT_ERROR:
      Pike_error("Com.obj: push_varg: Failed to convert (VT_ERROR)\n");
      break;

    default:
      Pike_error("Com.obj: push_varg: Failed to convert (Unknown result)\n");
      break;
  }
  VariantClear(&cv);
}


static int push_typeinfo_members(ITypeInfo *pITypeInfo,
                                 TYPEATTR *pTypeAttr, int flags)
{
  unsigned i;
  int count = 0;

  if (flags & TYPEINFO_FUNC_ALL && pTypeAttr->cFuncs )
  {
    int iStart = 0;
    if (pTypeAttr->wTypeFlags & TYPEFLAG_FDUAL &&
        pTypeAttr->typekind == TKIND_DISPATCH)
      iStart = 7;

    for (i=iStart; i<pTypeAttr->cFuncs; i++)
    {
      FUNCDESC * pFuncDesc;
      BSTR pszFuncName;

      pITypeInfo->lpVtbl->GetFuncDesc(pITypeInfo, i, &pFuncDesc);

      pITypeInfo->lpVtbl->GetDocumentation(pITypeInfo, pFuncDesc->memid,
                                           &pszFuncName, 0, 0, 0);

      if ( ( flags & TYPEINFO_FUNC_FUNC &&
             pFuncDesc->invkind == INVOKE_FUNC) ||

           ( flags & TYPEINFO_FUNC_GET &&
             pFuncDesc->invkind == INVOKE_PROPERTYGET) ||

           ( flags & TYPEINFO_FUNC_PUT &&
             pFuncDesc->invkind == INVOKE_PROPERTYPUT) ||

           ( flags & TYPEINFO_FUNC_PUTREF &&
             pFuncDesc->invkind == INVOKE_PROPERTYPUTREF) )
      {
        push_string(make_shared_string1(pszFuncName));
        count++;
      }

      pITypeInfo->lpVtbl->ReleaseFuncDesc(pITypeInfo, pFuncDesc);
      SysFreeString( pszFuncName );
    }
  }

  if (flags & TYPEINFO_VAR_ALL && pTypeAttr->cVars )
  {
    for (i=0; i<pTypeAttr->cVars; i++)
    {
      VARDESC * pVarDesc;
      BSTR pszVarName;

      pITypeInfo->lpVtbl->GetVarDesc(pITypeInfo, i, &pVarDesc);

      pITypeInfo->lpVtbl->GetDocumentation(pITypeInfo, pVarDesc->memid,
                                           &pszVarName, 0, 0, 0);
      if (flags & TYPEINFO_VAR_CONST && pVarDesc->varkind == VAR_CONST)
      {
          push_string(make_shared_string1(pszVarName));
          push_varg(pVarDesc->lpvarValue);
          count++;
      }
      /* TODO: more types? */
      /* if (flags & TYPEINFO_VAR_... */

      pITypeInfo->lpVtbl->ReleaseVarDesc(pITypeInfo, pVarDesc);
      SysFreeString( pszVarName );
    }
  }
  return count;
}


/********/
/* cval */
/********/

static void cval_push_result(INT32 args, int flags)
{
  struct cval_storage *cval = THIS_CVAL;
  DISPPARAMS *dpar;
  VARIANT res;
  HRESULT hr;
  EXCEPINFO exc;
  UINT argErr;
  DISPID dispidNamed = DISPID_PROPERTYPUT;

  create_variant_arg(args, &dpar);
  VariantInit(&res);

  if (flags == DISPATCH_PROPERTYPUT)
  {
    dpar->cNamedArgs = 1;
    dpar->rgdispidNamedArgs = &dispidNamed;
  }

  hr = cval->pIDispatch->lpVtbl->Invoke(cval->pIDispatch,
                                        cval->dispid,
                                        &IID_NULL,
                                        GetUserDefaultLCID(),
                                        flags,
                                        dpar,
                                        &res,
                                        &exc,
                                        &argErr);


  free_variant_arg(&dpar);

  if (FAILED(hr))
  {
    com_throw_error2(hr, exc);
  
    /* NOT reached! */
    pop_n_elems(args);
    push_int(0);
    return;
  }

  pop_n_elems(args);

  push_varg(&res);

  VariantClear(&res);

  return;
}

static void init_cval_struct(struct object *o)
{
  struct cval_storage *cval = THIS_CVAL;

  cval->pIDispatch = NULL;
  cval->method = NULL;
}

static void exit_cval_struct(struct object *o)
{
  struct cval_storage *cval = THIS_CVAL;

  if (cval->pIDispatch)
  {
    cval->pIDispatch->lpVtbl->Release(cval->pIDispatch);
    cval->pIDispatch = NULL;
  }
  if (cval->method)
  {
    free_string(cval->method);
    cval->method = NULL;
  }
}

/*   pike_add_function("create", f_cval_create, "function(void:void)", 0); */
/*
static void f_cval_create(INT32 args)
{
}
*/

/*   pike_add_function("destroy", f_cval_destroy, "function(void:void)", 0); */
/*
static void f_cval_destroy(INT32 args)
{
}
*/

/*   pike_add_function("_value", f_cval_value, "function(:mixed)", 0); */
static void f_cval__value(INT32 args)
{
  cval_push_result(0, DISPATCH_PROPERTYGET);
}

#define OPERATOR(op) static void f_cval_##op(INT32 args) \
{                                            \
  cval_push_result(0, DISPATCH_PROPERTYGET); \
  args++;                                    \
  stack_swap2(args);                         \
  f_##op(args);                              \
}

OPERATOR(add)
OPERATOR(minus)
OPERATOR(and)
OPERATOR(or)
OPERATOR(xor)
OPERATOR(lsh)
OPERATOR(rsh)
OPERATOR(multiply)
OPERATOR(divide)
OPERATOR(mod)
OPERATOR(compl)
OPERATOR(eq)
OPERATOR(lt)
OPERATOR(gt)
OPERATOR(not)

#undef OPERATOR


#define ROPERATOR(op) static void f_cval_r##op(INT32 args) \
{                                            \
  cval_push_result(0, DISPATCH_PROPERTYGET); \
  args++;                                    \
  f_##op(args);                              \
}

ROPERATOR(add)
ROPERATOR(minus)
ROPERATOR(and)
ROPERATOR(or)
ROPERATOR(xor)
ROPERATOR(lsh)
ROPERATOR(rsh)
ROPERATOR(multiply)
ROPERATOR(divide)
ROPERATOR(mod)

#undef ROPERATOR


/*   pike_add_function("__hash", f_cval___hash, "function(mixed:mixed)", 0); */
static void f_cval___hash(INT32 args)
{
  cval_push_result(0, DISPATCH_PROPERTYGET);
  args++;
  stack_swap2(args);
  f_hash(args);
}

/*   pike_add_function("cast", f_cval_cast, "function(mixed:mixed)", 0); */
static void f_cval_cast(INT32 args)
{
  struct cval_storage *cval = THIS_CVAL;

  if (args < 1)
    Pike_error("cast() called without arguments.\n");
  if (Pike_sp[-args].type != PIKE_T_STRING)
    Pike_error("Bad argument 1 to cast().\n");

  if (!strcmp(Pike_sp[-args].u.string->str, "object")) {
    pop_n_elems(args);
    push_object(this_object());
  }

  
/*   if (strcmp(Pike_sp[-args].u.string->str, "string")) */
/*     Pike_error("cast() to other type than string.\n"); */

  pop_n_elems(args);
  cval_push_result(0, DISPATCH_PROPERTYGET);
}

/*   pike_add_function("`[]", f_cval_ind, "function(mixed:mixed)", 0); */
/* static void f_cval_ind(INT32 args) */
/* { */
/*   f_index(args); */
/* } */

/*   pike_add_function("`[]=", f_cval_indset, "function(mixed:mixed)", 0); */
/* static void f_cval_indset(INT32 args) */
/* { */
/*   f_index_assign(args); */
/* } */

/*   pike_add_function("`->", f_cval_aind, "function(mixed:mixed)", 0); */
static void f_cval_arrow(INT32 args)
{
  struct cval_storage *cval = THIS_CVAL;

  if (args != 1)
    Pike_error("Bad argument to cval::`->\n");

  if (Pike_sp[-1].type == PIKE_T_STRING &&
      !strncmp(Pike_sp[-1].u.string->str, "_value", Pike_sp[-1].u.string->len))
  {
    pop_n_elems(args);
    cval_push_result(0, DISPATCH_PROPERTYGET);
    return;
  }

  cval_push_result(0, DISPATCH_PROPERTYGET);
  if (Pike_sp[-1].type != PIKE_T_OBJECT ||
      Pike_sp[-1].u.object->prog != cobj_program)
  {
    pop_n_elems(args+1);
    push_int(0);
    Pike_sp[-1].subtype=NUMBER_UNDEFINED;
    return;
  }
  stack_swap();
  f_arrow(2);
}

/*   pike_add_function("`->=", f_cval_aindset, "function(mixed:mixed)", 0); */
static void f_cval_arrow_assign(INT32 args)
{
  struct cval_storage *cval = THIS_CVAL;

  if (args != 2)
    Pike_error("Bad argument to cval::`->=\n");

  cval_push_result(0, DISPATCH_PROPERTYGET);
  if (Pike_sp[-1].type != PIKE_T_OBJECT ||
      Pike_sp[-1].u.object->prog != cobj_program)
  {
    pop_n_elems(args+1);
    push_int(0);
  }
  stack_swap2(3);
  f_arrow_assign(3);
}

/*   pike_add_function("_sizeof", f_cval__sizeof, "function(mixed:mixed)", 0); */
/* static void f_cval__sizeof(INT32 args) */
/* { */
/* } */

/*   pike_add_function("_indices", f_cval__indices, "function(mixed:mixed)", 0); */
/* static void f_cval__indices(INT32 args) */
/* { */
/* } */

/*   pike_add_function("_values", f_cval__values, "function(mixed:mixed)", 0); */
/* static void f_cval__values(INT32 args) */
/* { */
/* } */

/*   pike_add_function("`()", f_cval_func, "function(mixed:mixed)", 0); */
static void f_cval_func(INT32 args)
{
  cval_push_result(args, DISPATCH_METHOD);
}

/*   pike_add_function("`+=", f_cval_inc, "function(mixed:mixed)", 0); */
/* static void f_cval_inc(INT32 args) */
/* { */
/* } */

/*   pike_add_function("_is_type", f_cval__is_type, "function(mixed:mixed)", 0); */
/* static void f_cval__is_type(INT32 args) */
/* { */
/* } */

/*   pike_add_function("_sprintf", f_cval__sprintf, "function(mixed:mixed)", 0); */
static void f_cval__sprintf(INT32 args)
{
  struct cval_storage *cval = THIS_CVAL;
  int n;
  char buf[80];
  char *p;
  INT_TYPE precision, precision_undecided, width, width_undecided;
/*   INT_TYPE base = 0, mask_shift = 0; */
/*   struct pike_string *s = 0; */
  INT_TYPE flag_left, method;
  
/*   get_all_args("_sprintf",args,"%i",&x); */
  if (args < 1 || Pike_sp[-args].type != PIKE_T_INT)
    Pike_error("Bad argument 1 for Com.cval->_sprintf().\n");
  if (args < 2 || Pike_sp[1-args].type != PIKE_T_MAPPING)
    Pike_error("Bad argument 2 for Com.cval->_sprintf().\n");
  
  push_svalue(&Pike_sp[1-args]);
  push_constant_text("precision");
  f_index(2);
  if (Pike_sp[-1].type != PIKE_T_INT)
    Pike_error("\"precision\" argument to Com->_sprintf() is not an integer.\n");
  precision_undecided = ((Pike_sp-1)->subtype != NUMBER_NUMBER);
  precision = (--Pike_sp)->u.integer;
  
  push_svalue(&Pike_sp[1-args]);
  push_constant_text("width");
  f_index(2);
  if (Pike_sp[-1].type != PIKE_T_INT)
    Pike_error("\"width\" argument to Com->_sprintf() is not an integer.\n");
  width_undecided = ((Pike_sp-1)->subtype != NUMBER_NUMBER);
  width = (--Pike_sp)->u.integer;

  push_svalue(&Pike_sp[1-args]);
  push_constant_text("flag_left");
  f_index(2);
  if (Pike_sp[-1].type != PIKE_T_INT)
    Pike_error("\"flag_left\" argument to Com->_sprintf() is not an integer.\n");
  flag_left=Pike_sp[-1].u.integer;
  pop_stack();

  switch (method = Pike_sp[-args].u.integer)
  {
    case 'O':
      n=0;
      n++;push_constant_text("Com.cval(\"");
      n++;ref_push_string(cval->method);
      n++;push_constant_text("\" ");
      n++;push_int(cval->dispid);
      n++;push_constant_text(" ");
      sprintf(buf, "%x", cval->pIDispatch);
      n++;push_text(buf);
      n++;push_constant_text(")");
      f_add(n);
      stack_pop_n_elems_keep_top(args);
      return;

    case 'b':
    case 'd':
    case 'u':
    case 'o':
    case 'x':
    case 'c':
    case 'f':
    case 'g':
    case 'G':
    case 'e':
    case 'E':
    case 's':
/*     case 't': */
      p = buf;
      p += sprintf(p, "%%%s", flag_left?"-":"");
      if (!width_undecided)
        p += sprintf(p, "%d", width);
      if (!precision_undecided)
        p += sprintf(p, ".%d", precision);
      p += sprintf(p, "%c", method);
      push_text(buf);
        
      cval_push_result(0, DISPATCH_PROPERTYGET);
      f_sprintf(2);
      return;

    default:
      pop_n_elems(args);
      push_int(0);
      return;
  }
  
}

/*   pike_add_function("_equal", f_cval__equal, "function(mixed:mixed)", 0); */
/* static void f_cval__equal(INT32 args) */
/* { */
/* } */

/*   pike_add_function("_m_delete", f_cval__m_delete, "function(mixed:mixed)", 0); */
/* static void f_cval__m_delete(INT32 args) */
/* { */
/* } */



/********/
/* cobj */
/********/

static void f_cobj_create(INT32 args)
{
  struct cobj_storage *cobj = THIS_COBJ;
  HRESULT hr;
  CLSID clsid;
  struct pike_string *progID;
  PCHARP progID2;

  if (args > 0)
  {
    get_all_args("Com.obj->create()", args, "%W", &progID);
    progID2 = MKPCHARP(malloc(progID->len * 2 + 2), 1);
    pike_string_cpy(progID2, progID);
    SET_INDEX_PCHARP(progID2, progID->len, 0);
    
    hr = CLSIDFromProgID((OLECHAR *)progID2.ptr, &clsid);
    if (!SUCCEEDED(hr))
    {
      pop_n_elems(args);
      destruct(Pike_fp->current_object);
      return;
    }
    
    hr = CoCreateInstance(&clsid, NULL, CLSCTX_SERVER,
                          &IID_IDispatch, (void**)&cobj->pIDispatch);
    if (!SUCCEEDED(hr))
    {
      pop_n_elems(args);
      destruct(Pike_fp->current_object);
      return;
    }
  }

  cobj->method_map = allocate_mapping(10);
  mapping_set_flags(cobj->method_map, MAPPING_WEAK);
  
  pop_n_elems(args);
  push_int(0);

}

static void init_cobj_struct(struct object *o)
{
  struct cobj_storage *cobj = THIS_COBJ;

  cobj->pIDispatch = NULL;
  cobj->method_map = NULL;
}

static void exit_cobj_struct(struct object *o)
{
  struct cobj_storage *cobj = THIS_COBJ;


  if (cobj->pIDispatch)
  {
    cobj->pIDispatch->lpVtbl->Release(cobj->pIDispatch);
    cobj->pIDispatch = NULL;
  }
  if (cobj->method_map)
  {
    free_mapping(cobj->method_map);
  }
}

static void f_cobj_getprop(INT32 args)
{
  struct cobj_storage *cobj = THIS_COBJ;
  DISPPARAMS *dpar;
  VARIANT res;
  EXCEPINFO exc;
  UINT argErr;
  HRESULT hr;
  struct pike_string *prop;
  PCHARP propU;

  get_all_args("Com.obj->get_prop()", args, "%W", &prop);
  propU = MKPCHARP(malloc(prop->len * 2 + 2), 1);
  pike_string_cpy(propU, prop);
  SET_INDEX_PCHARP(propU, prop->len, 0);

  create_variant_arg(0, &dpar);
  VariantInit(&res);
  hr = invoke(cobj, (OLECHAR *)propU.ptr, DISPATCH_PROPERTYGET, dpar,
              &res, &exc, &argErr);
  free_variant_arg(&dpar);
  free(propU.ptr);

  if (FAILED(hr))
  {
    com_throw_error2(hr, exc);
  
    pop_n_elems(args);
    push_int(0);
    return;
  }

  pop_n_elems(args);

  push_varg(&res);

  VariantClear(&res);

  return;
}

static void f_cobj_setprop(INT32 args)
{
  struct cobj_storage *cobj = THIS_COBJ;
  DISPPARAMS *dpar;
  VARIANT res;
  EXCEPINFO exc;
  UINT argErr;
  HRESULT hr;
  struct pike_string *prop;
  PCHARP propU;

  if (args!=2)
    Pike_error("Incorrect number of arguments to set_prop.\n");

  if (Pike_sp[-args].type != PIKE_T_STRING)
    Pike_error("Bad argument 1 to set_prop.\n");

  prop = Pike_sp[-args].u.string;
  propU = MKPCHARP(malloc(prop->len * 2 + 2), 1);
  pike_string_cpy(propU, prop);
  SET_INDEX_PCHARP(propU, prop->len, 0);

  create_variant_arg(args-1, &dpar);
  VariantInit(&res);
  hr = invoke(cobj, (OLECHAR *)propU.ptr, DISPATCH_PROPERTYPUT, dpar,
              &res, &exc, &argErr);
  free_variant_arg(&dpar);
  free(propU.ptr);

  if (FAILED(hr))
  {
    com_throw_error2(hr, exc);
  
    /* Not reached */
    pop_n_elems(args);
    push_int(0);
    return;
  }

  pop_n_elems(args);

  push_varg(&res);

  VariantClear(&res);

  return;
}

static void f_cobj_call_method(INT32 args)
{
  struct cobj_storage *cobj = THIS_COBJ;
  DISPPARAMS *dpar;
  VARIANT res;
  EXCEPINFO exc;
  UINT argErr;
  HRESULT hr;
  struct pike_string *prop;
  PCHARP propU;

  if (args<1)
    Pike_error("Incorrect number of arguments to call_method.\n");

  if (Pike_sp[-args].type != PIKE_T_STRING)
    Pike_error("Bad argument 1 to call_method.\n");

  prop = Pike_sp[-args].u.string;
  propU = MKPCHARP(malloc(prop->len * 2 + 2), 1);
  pike_string_cpy(propU, prop);
  SET_INDEX_PCHARP(propU, prop->len, 0);

  create_variant_arg(args-1, &dpar);
  VariantInit(&res);
  hr = invoke(cobj, (OLECHAR *)propU.ptr, DISPATCH_METHOD, dpar,
              &res, &exc, &argErr);
  free_variant_arg(&dpar);
  free(propU.ptr);

  if (FAILED(hr))
  {
    com_throw_error2(hr, exc);

    /* Not reached */
    pop_n_elems(args);
    push_int(0);
    return;
  }

  pop_n_elems(args);

  push_varg(&res);

  VariantClear(&res);

  return;
}

static void f_cobj_arrow(INT32 args)
{
  struct cobj_storage *cobj = THIS_COBJ;
  struct svalue *cval_prog;

  if (args<1 || Pike_sp[-args].type != PIKE_T_STRING ||
     args>1)
    Pike_error("Bad args to `->.");

  cval_prog = low_mapping_lookup(cobj->method_map, &Pike_sp[-args]);

  if (cval_prog)
  {
    push_svalue(cval_prog);
  }
  else
  {
    struct object      *oo;
    DISPID dispid;
    HRESULT hr;
    struct pike_string *method;
    OLECHAR method2[MAX_PATH+1];
    PCHARP  tmp;
    
    method = Pike_sp[-args].u.string;

    if (method->len > MAX_PATH)
      Pike_error("Bad argument to `->: String too long\n");
    tmp = MKPCHARP((char *)method2, 1);
    pike_string_cpy(tmp, method);
    SET_INDEX_PCHARP(tmp, method->len, 0);

    hr = cobj->pIDispatch->lpVtbl->
      GetIDsOfNames(cobj->pIDispatch, &IID_NULL, (OLECHAR **)&tmp.ptr, 1,
                    GetUserDefaultLCID(), &dispid);

    if (FAILED(hr))
      com_throw_error(hr);

    oo=clone_object(cval_program, 0);
    if (oo->prog != NULL)
    {
      struct cval_storage *cv = (struct cval_storage *)(oo->storage);

      cv->pIDispatch = cobj->pIDispatch;
      cv->pIDispatch->lpVtbl->AddRef(cv->pIDispatch);
      cv->dispid = dispid;
      copy_shared_string(cv->method, method);
      push_object(oo);
      mapping_string_insert(cobj->method_map, method, &Pike_sp[-1]);
    }
    else {
      free_object(oo);
      push_int(0);
    }

  }

  stack_pop_n_elems_keep_top(args);

  return;
}

static void f_cobj_arrow_assign(INT32 args)
{
  f_cobj_setprop(args);
}


static void f_cobj__sprintf(INT32 args)
{
  struct cobj_storage *cobj = THIS_COBJ;
  int n;
  char buf[80];
  
  if (args < 1 || Pike_sp[-args].type != PIKE_T_INT)
    Pike_error("Bad argument 1 for Com.cobj->_sprintf().\n");
  if (args < 2 || Pike_sp[1-args].type != PIKE_T_MAPPING)
    Pike_error("Bad argument 2 for Com.cobj->_sprintf().\n");
  
  switch (Pike_sp[-args].u.integer)
  {
    case 'O':
      n=0;
      n++;push_constant_text("Com.cobj(");
      sprintf(buf, "%x", cobj->pIDispatch);
      n++;push_text(buf);
      n++;push_constant_text(")");
      f_add(n);
      stack_pop_n_elems_keep_top(args);
      return;
  }
  
  pop_n_elems(args);
  push_int(0);
}


static void f_cobj__indices(INT32 args)
{
  struct cobj_storage *cobj = THIS_COBJ;
  ITypeInfo *ptinfo;
  TYPEATTR * pTypeAttr;
  int count = 0;
  HRESULT hr;
  
  if (args != 0 )
    Pike_error("Bad arguments for Com.cobj->_indices().\n");
  
  hr = cobj->pIDispatch->lpVtbl->GetTypeInfo(cobj->pIDispatch, 0,
                                             GetUserDefaultLCID(), &ptinfo);
  
  if (FAILED(hr))
  {
    f_aggregate(0);
    return;
  }

#ifdef COM_DEBUG
  DisplayTypeInfo(ptinfo);
#endif

  hr = ptinfo->lpVtbl->GetTypeAttr(ptinfo, &pTypeAttr);
  if ( S_OK == hr )
  {
    if (pTypeAttr->typekind == TKIND_DISPATCH)
    {
      count += push_typeinfo_members(ptinfo, pTypeAttr,
                                     TYPEINFO_FUNC_ALL);
    }
    else if (pTypeAttr->wTypeFlags & TYPEFLAG_FDUAL)
    {
      HREFTYPE refType;
      ITypeInfo *ptinforef;
      TYPEATTR * pTypeAttrRef;

      ptinfo->lpVtbl->GetRefTypeOfImplType(ptinfo, -1, &refType);
      ptinfo->lpVtbl->GetRefTypeInfo(ptinfo, refType, &ptinforef);

#ifdef COM_DEBUG
      DisplayTypeInfo(ptinforef);
#endif

      hr = ptinforef->lpVtbl->GetTypeAttr(ptinforef, &pTypeAttrRef);
      if ( S_OK == hr )
      {
        if (pTypeAttrRef->typekind == TKIND_DISPATCH)
        {
          count += push_typeinfo_members(ptinforef, pTypeAttrRef,
                                         TYPEINFO_FUNC_ALL);
        }
        ptinforef->lpVtbl->ReleaseTypeAttr(ptinforef, pTypeAttrRef);
      }
      ptinforef->lpVtbl->Release(ptinforef);
    }
    
    ptinfo->lpVtbl->ReleaseTypeAttr(ptinfo, pTypeAttr);
  }

/*   if (count > 0) */
/*   { */
    f_aggregate(count);
/*   } */
/*   else */
/*     push_int(0); */

}


/*************************/
/* COM                   */


#ifdef USE_COM_PROG
static void f_create(INT32 args)
{
  struct com_storage *c = THIS_COM;

  pop_n_elems(args);

  push_int(0);
}

static void init_com_struct(struct object *o)
{
  struct com_storage *c = THIS_COM;

}

static void exit_com_struct(struct object *o)
{
  struct com_storage *c = THIS_COM;

}

/* #ifdef _REENTRANT */
/* static void com_gc_check(struct object *o) */
/* { */
/*   struct com_storage *c = THIS_COM; */

/* } */

/* static void com_gc_recurse(struct object *o) */
/* { */
/*   struct com_storage *c = THIS_COM; */

/* } */
/* #endif /\* _REENTRANT *\/ */

#endif /* USE_COM_PROG */


/* static void f_get_version(INT32 args) */
/* { */
/*   pop_n_elems(args); */
/*   push_int(314); */
/* } */

/* #include <Iads.h> */
/* static void f_get_nt_systeminfo(INT32 args) */
/* { */
/*   HRESULT hr; */
/*   IADsWinNTSystemInfo *pNTsys = NULL; */
/*   BSTR bstr; */

/*   /\* */
/*   hr = CoCreateInstance(&CLSID_WinNTSystemInfo, */
/*                         NULL, */
/*                         CLSCTX_INPROC_SERVER, */
/*                         &IID_IADsWinNTSystemInfo, */
/*                         (void**)&pNTsys); */

/*   hr = pNTsys->lpVtbl->get_UserName(pNTsys, &bstr); */
/*   *\/ */
/*   CLSID clsid; */
/*   wchar_t progid[] = L"WinNTSystemInfo"; */
/*   IDispatch *pIDispatch = NULL; */
/*   DISPID dispid; */
/*   OLECHAR *name = L"UserName"; */
/*   DISPPARAMS dispparamsNoArgs = { */
/*     NULL, */
/*     NULL, */
/*     0, /\* Zero arguments *\/ */
/*     0  /\* Zero named arguments *\/ */
/*   }; */
/*   VARIANT varResult; */
  
/*   hr = CLSIDFromProgID(progid, &clsid); */
/*   hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, */
/*                         &IID_IDispatch, (void**)&pIDispatch); */
/*   pIDispatch->lpVtbl->GetIDsOfNames(pIDispatch, &IID_NULL, &name, 1, */
/*                                     GetUserDefaultLCID(), &dispid); */
  
/*   hr = pIDispatch->lpVtbl->Invoke(pIDispatch, */
/*                                   dispid, */
/*                                   &IID_NULL, */
/*                                   GetUserDefaultLCID(), */
/*                                   DISPATCH_PROPERTYGET, */
/*                                   &dispparamsNoArgs, */
/*                                   &varResult, */
/*                                   NULL, */
/*                                   NULL); */

/*   pop_n_elems(args); */
/*   if (SUCCEEDED(hr)) { */
/*     printf("User: %S\n", varResult.bstrVal); */
/*     push_text("User"); */
/*     push_string(make_shared_string1(varResult.bstrVal)); */
/*     f_aggregate(2); */
/*     //SysFreeString(bstr); */
/*   } */
/*   else */
/*     push_int(0); */

/*   if (pNTsys) { */
/*     pNTsys->lpVtbl->Release(pNTsys); */
/*     pNTsys = NULL; */
/*   } */
/*   if (pIDispatch) { */
/*     pIDispatch->lpVtbl->Release(pIDispatch); */
/*     pIDispatch = NULL; */
/*   } */
/* } */

static void f_create_object(INT32 args)
{
  struct object *oo;
  HRESULT hr;
  CLSID clsid;
  struct pike_string *progID;
  PCHARP progID2;
  IDispatch *pDispatch;

  get_all_args("Com->create_object()", args, "%W", &progID);
  //check_all_args("create_object", args, BIT_STRING, 0);

  progID2 = MKPCHARP(malloc(progID->len * 2 + 2), 1);
  pike_string_cpy(progID2, progID);
  SET_INDEX_PCHARP(progID2, progID->len, 0);
  
  hr = CLSIDFromProgID((OLECHAR *)progID2.ptr, &clsid);
  free(progID2.ptr);
  
  if (FAILED(hr))
  {
    pop_n_elems(args);
    push_int(0);
    return;
  }
  
  hr = CoCreateInstance(&clsid, NULL, CLSCTX_SERVER,
                        &IID_IDispatch, (void**)&pDispatch);
  if (FAILED(hr))
  {
    pop_n_elems(args);
    push_int(0);
    return;
  }

  oo=clone_object(cobj_program, 0);
  if (oo->prog!=NULL)
  {
    struct cobj_storage *co;
    
    co = (struct cobj_storage *)(oo->storage);
    co->pIDispatch = pDispatch;
    push_object(oo);
  }
  else {
    pDispatch->lpVtbl->Release(pDispatch);
    free_object(oo);
    push_int(0);
  }
}

static void f_get_object(INT32 args)
{
  struct object      *oo;
  struct pike_string *progID;
  struct pike_string *filename;
  HRESULT    hr;
  CLSID      clsid;
  PCHARP     progID2;
  OLECHAR    filename2[MAX_PATH+1];
  int        type = 0;
  IDispatch *pDispatch;

  if (args < 1 || args > 2 || (args == 1 && IS_ZERO(&Pike_sp[-args])))
    Pike_error("Incorrect number of arguments to GetObject.\n");

  if (args >= 1 && !IS_ZERO(&Pike_sp[-args]))
  {
    PCHARP     tmp;
    type += 0x1;

    if (Pike_sp[-args].type != PIKE_T_STRING)
      Pike_error("Bad argument 1 to GetObject\n");
    filename = Pike_sp[-args].u.string;

    if (filename->len > MAX_PATH)
      Pike_error("Bad argument 1 to GetObject: Too large\n");
    tmp = MKPCHARP((char *)filename2, 1);
    pike_string_cpy(tmp, filename);
    SET_INDEX_PCHARP(tmp, filename->len, 0);
  }

  if (args >= 2 && !IS_ZERO(&Pike_sp[1-args]))
  {
    type += 0x2;

    if (Pike_sp[1-args].type != PIKE_T_STRING)
      Pike_error("Bad argument 2 to GetObject\n");
    progID = Pike_sp[1-args].u.string;

    progID2 = MKPCHARP(malloc(progID->len * 2 + 2), 1);
    pike_string_cpy(progID2, progID);
    SET_INDEX_PCHARP(progID2, progID->len, 0);
    
    hr = CLSIDFromProgID((OLECHAR *)progID2.ptr, &clsid);
    free(progID2.ptr);
    
    if (FAILED(hr))
    {
      pop_n_elems(args);
      push_int(0);
      return;
    }
  }

  switch (type)
  {
    case 0x1:
      /* GetObject(filename,) */
      {
        IBindCtx *pbc;
        ULONG cEaten;
        IMoniker *pmk;
        IDispatch *pdisp;
        
        hr = CreateBindCtx(0, &pbc);
        if (FAILED(hr))
          com_throw_error(hr);

        hr = MkParseDisplayName(pbc, filename2, &cEaten, &pmk);
        if (FAILED(hr))
        {
          pbc->lpVtbl->Release(pbc);
          com_throw_error(hr);
        }

        hr = BindMoniker(pmk, 0, &IID_IDispatch, (void *)&pDispatch);
        if (FAILED(hr))
        {
          pmk->lpVtbl->Release(pmk);
          pbc->lpVtbl->Release(pbc);
          com_throw_error(hr);
        }

        pmk->lpVtbl->Release(pmk);
        pbc->lpVtbl->Release(pbc);
      }
      break;

    case 0x2:
      /* GetObject(,progID) */
      {
        IUnknown *punk;

        hr = GetActiveObject(&clsid, NULL, &punk);
        if (FAILED(hr))
          com_throw_error(hr);

        hr = punk->lpVtbl->QueryInterface(punk, &IID_IDispatch,
                                     (LPVOID FAR*)&pDispatch);
        if (FAILED(hr))
        {
          punk->lpVtbl->Release(punk);
          com_throw_error(hr);
        }

        punk->lpVtbl->Release(punk);
      }
      break;

    case 0x3:
      /* GetObject(filename,progID) */
      {
        IPersistFile *pPF;
        hr = CoCreateInstance(&clsid, NULL, CLSCTX_SERVER,
                              &IID_IPersistFile, (void**)&pPF);

        if (FAILED(hr))
        {
          pop_n_elems(args);
          push_int(0);
          return;
        }
        
        if (FAILED(hr = pPF->lpVtbl->Load(pPF, filename2, 0)) ||
            FAILED(hr = pPF->lpVtbl->QueryInterface(pPF, &IID_IDispatch,
                                                    (void **)&pDispatch)) )
        {
          pPF->lpVtbl->Release(pPF);
          pop_n_elems(args);
          push_int(0);
          return;
        }
        
        pPF->lpVtbl->Release(pPF);
      }
      break;

    case 0x0:
    default:
      Pike_error("At least one of filename and progID must be present\n");
      break;
  }

  oo=clone_object(cobj_program, 0);
  if (oo->prog!=NULL)
  {
    struct cobj_storage *co;
    
    co = (struct cobj_storage *)(oo->storage);
    co->pIDispatch = pDispatch;
    push_object(oo);
  }
  else
  {
    pDispatch->lpVtbl->Release(pDispatch);
    free_object(oo);
    push_int(0);
  }
}

/* ------------------ */
/* test begin */
/* VVVVVVVVVVVVVVVVVV */
/* VVVVVVVVVVVVVVVVVV */


#define CASE_STRING( x ) case x: s = #x; break

char * GetTypeKindName( TYPEKIND typekind )
{
  char *s = "<unknown>";
  
  switch( typekind )
  {
    CASE_STRING( TKIND_ENUM );
    CASE_STRING( TKIND_RECORD );
    CASE_STRING( TKIND_MODULE );
    CASE_STRING( TKIND_INTERFACE );
    CASE_STRING( TKIND_DISPATCH );
    CASE_STRING( TKIND_COCLASS );
    CASE_STRING( TKIND_ALIAS );
    CASE_STRING( TKIND_UNION );
  }
  
  return s;
}

char * GetInvokeKindName( INVOKEKIND invkind )
{
  char *s = "<unknown>";
  
  switch( invkind )
  {
    CASE_STRING( INVOKE_FUNC );
    CASE_STRING( INVOKE_PROPERTYGET );
    CASE_STRING( INVOKE_PROPERTYPUT );
    CASE_STRING( INVOKE_PROPERTYPUTREF );
  }
  
  return s;
}

void EnumTypeInfoMembers( LPTYPEINFO pITypeInfo, LPTYPEATTR pTypeAttr  )
{
  unsigned i;
  if ( pTypeAttr->cFuncs )
  {
    printf( "  Functions:\n" );
    
    for ( i = 0; i < pTypeAttr->cFuncs; i++ )
    {
      FUNCDESC * pFuncDesc;
      BSTR pszFuncName;
      
      pITypeInfo->lpVtbl->GetFuncDesc( pITypeInfo, i, &pFuncDesc );
      
      pITypeInfo->lpVtbl->GetDocumentation(pITypeInfo, pFuncDesc->memid, &pszFuncName,0,0,0);
      
      printf( "    %-32ls", pszFuncName );
      
      printf( " (%s)\n", GetInvokeKindName(pFuncDesc->invkind) );
      
      pITypeInfo->lpVtbl->ReleaseFuncDesc( pITypeInfo, pFuncDesc );
      SysFreeString( pszFuncName );
    }
  }
  
  if ( pTypeAttr->cVars )
  {
    printf( "  Variables:\n" );
    
    for ( i = 0; i < pTypeAttr->cVars; i++ )
    {
      VARDESC * pVarDesc;
      BSTR pszVarName;
      
      pITypeInfo->lpVtbl->GetVarDesc( pITypeInfo, i, &pVarDesc );
      
      pITypeInfo->lpVtbl->GetDocumentation(pITypeInfo, pVarDesc->memid, &pszVarName,0,0,0);
      
      printf( "    %ls\n", pszVarName );
      
      pITypeInfo->lpVtbl->ReleaseVarDesc( pITypeInfo, pVarDesc );
      SysFreeString( pszVarName );
    }
  }
  
}

void DisplayTypeInfo( LPTYPEINFO pITypeInfo )
{
  HRESULT hr;
  BSTR pszTypeInfoName;
  TYPEATTR * pTypeAttr;

  hr = pITypeInfo->lpVtbl->GetDocumentation(pITypeInfo, MEMBERID_NIL, &pszTypeInfoName, 0, 0, 0);
  if ( S_OK != hr )
    return;
  
  hr = pITypeInfo->lpVtbl->GetTypeAttr( pITypeInfo, &pTypeAttr );
  if ( S_OK != hr )
  {
    SysFreeString( pszTypeInfoName );
    return;
  }
  
  printf( "%ls - %s\n", pszTypeInfoName,
            GetTypeKindName(pTypeAttr->typekind) );
  
  EnumTypeInfoMembers( pITypeInfo, pTypeAttr );
  
  printf( "\n" );
  
  SysFreeString( pszTypeInfoName );
  
  pITypeInfo->lpVtbl->ReleaseTypeAttr( pITypeInfo, pTypeAttr );
}


/* ^^^^^^^^^^^^^^^^^^ */
/* ^^^^^^^^^^^^^^^^^^ */
/* test end */
/* ------------------ */

static void f_get_typeinfo(INT32 args)
{
#ifdef USE_COM_PROG
  struct com_storage *com = THIS_COM;
#endif /* USE_COM_PROG */
  struct cobj_storage *co;  
  ITypeInfo *ptinfo;
  TYPEATTR *ptattr;

  if (args != 1)
    Pike_error("Bad number of arguments for Com->GetTypeInfo().\n");
  if (Pike_sp[-args].type != PIKE_T_OBJECT ||
      Pike_sp[-args].u.object->prog != cobj_program)
    Pike_error("Bad argument 1 for Com->GetTypeInfo().\n");
 
  co = (struct cobj_storage *)Pike_sp[-args].u.object->storage;
  co->pIDispatch->lpVtbl->GetTypeInfo(co->pIDispatch, 0,
                                      GetUserDefaultLCID(), &ptinfo);

  ptinfo->lpVtbl->GetTypeAttr(ptinfo, &ptattr);

  fprintf(stderr, " cbSizeInstance=%d\n typekind=%ls\n cFuncs=%d\n"
          " cVars=%d\n cImplTypes=%d\n cbSizeVft=%d\n cbAlignment=%d\n"
          " wMajorVerNum=%d\n wMinorVerNum=%d\n",
          ptattr->cbSizeInstance,
          GetTypeKindName(ptattr->typekind),
          ptattr->cFuncs,
          ptattr->cVars, ptattr->cImplTypes,
          ptattr->cbSizeVft, ptattr->cbAlignment,
          ptattr->wMajorVerNum, ptattr->wMinorVerNum);

  ptinfo->lpVtbl->ReleaseTypeAttr(ptinfo, ptattr);

  DisplayTypeInfo(ptinfo);

  ptinfo->lpVtbl->Release(ptinfo);
}


static void f_get_constants(INT32 args)
{
#ifdef USE_COM_PROG
  struct com_storage *com = THIS_COM;
#endif /* USE_COM_PROG */
  struct cobj_storage *co;  
  ITypeInfo *ptinfo;
  ITypeLib *ptlib;
  UINT tiCount;
  TYPEATTR *ptattr;
  unsigned index;
  int count = 0;
  int i;
  HRESULT hr;

  if (args != 1)
    Pike_error("Bad number of arguments for Com->GetConstants().\n");
/*   if (Pike_sp[-args].type != PIKE_T_OBJECT || */
/*       Pike_sp[-args].u.object->prog != cobj_program) */
/*     Pike_error("Bad argument 1 for Com->GetConstants().\n"); */
  if (Pike_sp[-args].type == PIKE_T_STRING)
  {
    /* String */
    char *to_free = NULL;
    OLECHAR *typelib = NULL;

    typelib = require_wstring1(Pike_sp[-args].u.string, &to_free);
    if (!typelib)
      Pike_error("Bad argument 1 for Com->GetConstants().\n");

    hr = LoadTypeLib(typelib, &ptlib);
    if (to_free)
      free(to_free);

    if (FAILED(hr))
    {
      com_throw_error(hr);
      /* Not reached */
      return;
    }
  }
  else if (Pike_sp[-args].type == PIKE_T_OBJECT &&
      Pike_sp[-args].u.object->prog == cobj_program)
  {
    /* cobj */
    co = (struct cobj_storage *)Pike_sp[-args].u.object->storage;
    hr = co->pIDispatch->lpVtbl->GetTypeInfo(co->pIDispatch, 0,
                                             GetUserDefaultLCID(), &ptinfo);

    if (FAILED(hr))
    {
      f_aggregate_mapping(0);
      return;
    }

    hr = ptinfo->lpVtbl->GetContainingTypeLib(ptinfo, &ptlib, &index);
    ptinfo->lpVtbl->Release(ptinfo);
    if (FAILED(hr))
    {
      f_aggregate_mapping(0);
      return;
    }
  }
  else
    Pike_error("Bad argument 1 for Com->GetConstants().\n");
 

  tiCount = ptlib->lpVtbl->GetTypeInfoCount(ptlib);

  for (i=0; i<tiCount; i++)
  {
    ITypeInfo *pITypeInfo;
    
    hr = ptlib->lpVtbl->GetTypeInfo(ptlib, i, &pITypeInfo);
    
    if ( S_OK == hr )
    {
      //DisplayTypeInfo( pITypeInfo );
      TYPEATTR * pTypeAttr;
      hr = pITypeInfo->lpVtbl->GetTypeAttr(pITypeInfo, &pTypeAttr);
      if ( S_OK == hr )
      {
        if (pTypeAttr->typekind == TKIND_ENUM)
        {
          count += push_typeinfo_members(pITypeInfo, pTypeAttr,
                                         TYPEINFO_VAR_CONST);
        }
        
        pITypeInfo->lpVtbl->ReleaseTypeAttr(pITypeInfo, pTypeAttr);
      }

      pITypeInfo->lpVtbl->Release(pITypeInfo);
    }
  }
  
  f_aggregate_mapping(count*2);

  stack_pop_n_elems_keep_top(args);

  ptlib->lpVtbl->Release(ptlib);
}

static void f_com__sprintf(INT32 args)
{
#ifdef USE_COM_PROG
  struct com_storage *com = THIS_COM;
#endif /* USE_COM_PROG */
  
  if (args < 1 || Pike_sp[-args].type != PIKE_T_INT)
    Pike_error("Bad argument 1 for Com->_sprintf().\n");
  if (args < 2 || Pike_sp[1-args].type != PIKE_T_MAPPING)
    Pike_error("Bad argument 2 for Com->_sprintf().\n");
  
  switch (Pike_sp[-args].u.integer)
  {
    case 'O':
      push_constant_text("Com()");
      stack_pop_n_elems_keep_top(args);
      return;
  }
  
  pop_n_elems(args);
  push_int(0);
}

#endif /* HAVE_COM */

PIKE_MODULE_INIT
{
#ifdef HAVE_COM
  struct svalue prog;
  prog.type = PIKE_T_PROGRAM;
  prog.subtype = 0;

  /* load and initialize COM */
  CoInitialize(0);

  start_new_program();
  ADD_STORAGE(struct cval_storage);
/*   pike_add_function("create", f_cval_create, "function(void:void)", 0); */
/*   pike_add_function("destroy", f_cval_destroy, "function(void:void)", 0); */

  pike_add_function("_value", f_cval__value, "function(:mixed)", 0);

  pike_add_function("`+", f_cval_add, "function(mixed:mixed)", 0);
  pike_add_function("`-", f_cval_minus, "function(mixed:mixed)", 0);
  pike_add_function("`&", f_cval_and, "function(mixed:mixed)", 0);
  pike_add_function("`|", f_cval_or, "function(mixed:mixed)", 0);
  pike_add_function("`^", f_cval_xor, "function(mixed:mixed)", 0);
  pike_add_function("`<<", f_cval_lsh, "function(mixed:mixed)", 0);
  pike_add_function("`>>", f_cval_rsh, "function(mixed:mixed)", 0);
  pike_add_function("`*", f_cval_multiply, "function(mixed:mixed)", 0);
  pike_add_function("`/", f_cval_divide, "function(mixed:mixed)", 0);
  pike_add_function("`%", f_cval_mod, "function(mixed:mixed)", 0);
  pike_add_function("`~", f_cval_compl, "function(:mixed)", 0);
  pike_add_function("`==", f_cval_eq, "function(mixed:mixed)", 0);
  pike_add_function("`<", f_cval_lt, "function(mixed:mixed)", 0);
  pike_add_function("`>", f_cval_gt, "function(mixed:mixed)", 0);

  pike_add_function("__hash", f_cval___hash, "function(:mixed)", 0);
  pike_add_function("cast", f_cval_cast, "function(mixed:mixed)", 0);
  pike_add_function("`!", f_cval_not, "function(:mixed)", 0);
/*   pike_add_function("`[]", f_cval_ind, "function(mixed:mixed)", 0); */
/*   pike_add_function("`[]=", f_cval_indset, "function(mixed:mixed)", 0); */
  pike_add_function("`[]", f_cval_arrow, "function(mixed:mixed)", 0);
  pike_add_function("`[]=", f_cval_arrow_assign, "function(mixed:mixed)", 0);
  pike_add_function("`->", f_cval_arrow, "function(mixed:mixed)", 0);
  pike_add_function("`->=", f_cval_arrow_assign, "function(mixed:mixed)", 0);
/*   pike_add_function("_sizeof", f_cval__sizeof, "function(mixed:mixed)", 0); */
/*   pike_add_function("_indices", f_cval__indices, "function(mixed:mixed)", 0); */
/*   pike_add_function("_values", f_cval__values, "function(mixed:mixed)", 0); */
  pike_add_function("`()", f_cval_func, "function(mixed:mixed)", 0);
  pike_add_function("``+", f_cval_radd, "function(mixed:mixed)", 0);
  pike_add_function("``-", f_cval_rminus, "function(mixed:mixed)", 0);
  pike_add_function("``&", f_cval_rand, "function(mixed:mixed)", 0);
  pike_add_function("``|", f_cval_ror, "function(mixed:mixed)", 0);
  pike_add_function("``^", f_cval_rxor, "function(mixed:mixed)", 0);
  pike_add_function("``<<", f_cval_rlsh, "function(mixed:mixed)", 0);
  pike_add_function("``>>", f_cval_rrsh, "function(mixed:mixed)", 0);
  pike_add_function("``*", f_cval_rmultiply, "function(mixed:mixed)", 0);
  pike_add_function("``/", f_cval_rdivide, "function(mixed:mixed)", 0);
  pike_add_function("``%", f_cval_rmod, "function(mixed:mixed)", 0);
/*   pike_add_function("`+=", f_cval_inc, "function(mixed:mixed)", 0); */
/*   pike_add_function("_is_type", f_cval__is_type, "function(mixed:mixed)", 0); */
  pike_add_function("_sprintf", f_cval__sprintf, "function(mixed:mixed)", 0);
/*   pike_add_function("_equal", f_cval__equal, "function(mixed:mixed)", 0); */
/*   pike_add_function("_m_delete", f_cval__m_delete, "function(mixed:mixed)", 0); */

  set_init_callback(init_cval_struct);
  set_exit_callback(exit_cval_struct);
  cval_program = end_program();
  cval_program->flags |= PROGRAM_DESTRUCT_IMMEDIATE;

  start_new_program();
  ADD_STORAGE(struct cobj_storage);
  pike_add_function("create", f_cobj_create, "function(string:void)", 0);
  pike_add_function("get_prop", f_cobj_getprop, "function(string:mixed)", 0);
  pike_add_function("set_prop", f_cobj_setprop,
                    "function(string,mixed:mixed)", 0);
  pike_add_function("call_method", f_cobj_call_method,
                    "function(string:mixed)", 0);
  pike_add_function("`->", f_cobj_arrow, "function(string:mixed)", 0);
  pike_add_function("`->=", f_cobj_arrow_assign,
                    "function(string,mixed:mixed)", 0);
  pike_add_function("_sprintf", f_cobj__sprintf, "function(mixed:mixed)", 0);
  pike_add_function("_indices", f_cobj__indices, "function(:mixed)", 0);
  
  set_init_callback(init_cobj_struct);
  set_exit_callback(exit_cobj_struct);
  cobj_program = end_program();
/*   add_program_constant("cobj", cobj_program = end_program(), 0); */
  cobj_program->flags |= PROGRAM_DESTRUCT_IMMEDIATE;

#ifdef USE_COM_PROG
  start_new_program();
  ADD_STORAGE(struct com_storage);
  pike_add_function("create", f_create, "function(string|void:void)", 0);
#endif /* USE_COM_PROG */

/*   pike_add_function("get_version", f_get_version, "function(:int)", 0); */
/*   pike_add_function("get_nt_systeminfo", f_get_nt_systeminfo, */
/*                     "function(:string|array|mapping)", 0); */
  pike_add_function("CreateObject", f_create_object,
                    "function(string:object)", 0);
  pike_add_function("GetObject", f_get_object,
                    "function(string|void,string|void:object)", 0);
  pike_add_function("GetTypeInfo", f_get_typeinfo,
                    "function(object:mixed)", 0);
  pike_add_function("GetConstants", f_get_constants,
                    "function(object|string:mapping)", 0);
  pike_add_function("_sprintf", f_com__sprintf, "function(mixed:mixed)", 0);

#ifdef USE_COM_PROG
  set_init_callback(init_com_struct);
  set_exit_callback(exit_com_struct);
/* #ifdef _REENTRANT */
/*   set_gc_check_callback(com_gc_check); */
/*   set_gc_recurse_callback(com_gc_recurse); */
/* #endif /\* _REENTRANT *\/ */

  add_program_constant("com", com_program = end_program(), 0);
  com_program->flags |= PROGRAM_DESTRUCT_IMMEDIATE;
#endif /* USE_COM_PROG */

#endif /* HAVE_COM */
}

PIKE_MODULE_EXIT
{
#ifdef HAVE_COM
#ifdef USE_COM_PROG
  if (com_program) {
    free_program(com_program);
    com_program=NULL;
  }
#endif /* USE_COM_PROG */
  if (cobj_program) {
    free_program(cobj_program);
    cobj_program=NULL;
  }
  if (cval_program) {
    free_program(cval_program);
    cval_program=NULL;
  }

  CoUninitialize();
#endif /* HAVE_COM */
}


