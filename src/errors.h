
#ifdef ERR_DECLARE
#define DECLARE_ERROR(NAME, INHERIT, DECL) \
static struct program *PIKE_CONCAT(NAME,_error_program); \
static int PIKE_CONCAT(NAME,_error_offset); \
struct PIKE_CONCAT(NAME,_error_struct) { \
  DECL \
};

#define ERR_VAR(TYPE,CTYPE,RUNTYPE,NAME) TYPE NAME ;
#endif


#ifdef ERR_SETUP
#define DECLARE_ERROR(NAME, INHERIT, DECL) do{ \
  int current_offset=0; \
  struct PIKE_CONCAT(NAME,_error_struct) foo; \
  start_new_program(); \
  INHERIT \
  current_offset = PIKE_CONCAT(NAME,_error_offset)=ADD_STORAGE(struct PIKE_CONCAT(NAME,_error_struct));\
  add_string_constant("error_type",#NAME "_error",0); \
  add_integer_constant("is_" #NAME "_error",1,0); \
  DECL \
  PIKE_CONCAT(NAME,_error_program)=end_program(); \
}while(0);

#define ERR_VAR(TYPE,CTYPE,RUNTYPE,NAME2) \
  map_variable("__" #NAME2, #CTYPE, 0, \
	       current_offset + (((char *)&(foo.NAME2))-((char *)&foo)), RUNTYPE);

#define ERR_INHERIT(NAME) \
  low_inherit(PIKE_CONCAT(NAME,_error_program),0,0,0,0,0);

#define ERR_FUNC(NAME,FUNC,TYPE,FLAGS) \
  ADD_FUNCTION(NAME,FUNC,TYPE,FLAGS);

#endif

#ifdef ERR_CLEANUP
#define DECLARE_ERROR(NAME, INHERIT, DECL) \
  if(PIKE_CONCAT(NAME,_error_program)) {\
    free_program(PIKE_CONCAT(NAME,_error_program)); \
    PIKE_CONCAT(NAME,_error_program)=0;\
  }
#endif


#ifndef ERR_INHERIT
#define ERR_INHERIT(NAME)
#endif

#ifndef ERR_VAR
#define ERR_VAR(TYPE,CTYPE,RUNTYPE,NAME)
#endif

#ifndef ERR_FUNC
#define ERR_FUNC(NAME,FUNC,TYPE,FLAGS)
#endif

DECLARE_ERROR(generic, /* no inherit */ ,
  ERR_VAR(struct pike_string *,string,T_STRING,desc)
  ERR_VAR(struct array *,array,T_ARRAY,backtrace)
  ERR_FUNC("cast",f_error_cast,tFunc(tString,tMixed),0)
  ERR_FUNC("`[]",f_error_index,tFunc(tString,tMixed),0)
  ERR_FUNC("describe",f_error_describe,tFunc(tVoid,tString),0)
  ERR_FUNC("backtrace",f_error_backtrace,tFunc(tVoid,tArr(tMixed)),0)
)

#define GENERIC_ERROR_THIS ((struct generic_error_struct *)fp->current_storage)

DECLARE_ERROR(index,
	      ERR_INHERIT(generic),
  ERR_VAR(struct svalue, mixed, T_MIXED, val)
  ERR_VAR(struct svalue, mixed, T_MIXED, ind)
)

DECLARE_ERROR(bad_arg,
	      ERR_INHERIT(generic),
  ERR_VAR(INT_TYPE, int, T_INT, which_arg)
  ERR_VAR(struct pike_string *,string,T_STRING,expected_type)
  ERR_VAR(struct svalue, mixed, T_MIXED, got)
)

#undef DECLARE_ERROR
#undef ERR_INHERIT
#undef ERR_VAR
#undef ERR_FUNC

#undef ERR_DECLARE
#undef ERR_SETUP
#undef ERR_CLEANUP
