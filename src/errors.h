#ifdef ERR_DECLARE
#define DECLARE_ERROR(NAME, INHERIT, DECL) \
PMOD_EXPORT struct program *PIKE_CONCAT(NAME,_error_program); \
ptrdiff_t PIKE_CONCAT(NAME,_error_offset); 

#define ERR_VAR(TYPE,CTYPE,RUNTYPE,NAME) TYPE NAME ;
#endif

#ifdef ERR_EXT_DECLARE
#define DECLARE_ERROR(NAME, INHERIT, DECL) \
PMOD_EXPORT extern struct program *PIKE_CONCAT(NAME,_error_program); \
extern ptrdiff_t PIKE_CONCAT(NAME,_error_offset); \
struct PIKE_CONCAT(NAME,_error_struct) { \
  DECL \
};

#define ERR_VAR(TYPE,CTYPE,RUNTYPE,NAME) TYPE NAME ;

/* Some compilers (eg cl) don't like empty structs... */
#define EMPTY ERR_VAR(INT_TYPE, int, PIKE_T_INT, ignored__)

#endif /* ERR_EXT_DECLARE */

#ifdef ERR_SETUP
#define DECLARE_ERROR(NAME, INHERIT, DECL) do{ \
  ptrdiff_t current_offset=0; \
  struct PIKE_CONCAT(NAME,_error_struct) foo; \
  start_new_program(); \
  INHERIT \
  current_offset = PIKE_CONCAT(NAME,_error_offset) = \
    ADD_STORAGE(struct PIKE_CONCAT(NAME,_error_struct));\
  add_string_constant("error_type",#NAME "_error",0); \
  add_integer_constant("is_" #NAME "_error",1,0); \
  DECL \
  PIKE_CONCAT(NAME,_error_program)=end_program(); \
  add_program_constant( #NAME "_error",PIKE_CONCAT(NAME,_error_program),0); \
}while(0);

#define ERR_VAR(TYPE,CTYPE,RUNTYPE,NAME2) \
  map_variable("__" #NAME2, #CTYPE, 0, \
	       current_offset + (((char *)&(foo.NAME2))-((char *)&foo)), RUNTYPE);

#define EMPTY

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

DECLARE_ERROR(generic, EMPTY ,
  ERR_VAR(struct pike_string *,string,PIKE_T_STRING,desc)
  ERR_VAR(struct array *,array,PIKE_T_ARRAY,backtrace)
  ERR_FUNC("cast",f_error_cast,tFunc(tString,tMixed),0)
  ERR_FUNC("`[]",f_error_index,tFunc(tString,tMixed),0)
  ERR_FUNC("describe",f_error_describe,tFunc(tVoid,tString),0)
  ERR_FUNC("backtrace",f_error_backtrace,tFunc(tVoid,tArr(tMixed)),0)
  ERR_FUNC("_sprintf",f_error__sprintf,tFunc(tNone,tString),0)
  ERR_FUNC("create",f_error_create,tFunc(tStr,tVoid),ID_STATIC)
)

#define GENERIC_ERROR_THIS ((struct generic_error_struct *)CURRENT_STORAGE)

DECLARE_ERROR(index,
	      ERR_INHERIT(generic),
  ERR_VAR(struct svalue, mixed, PIKE_T_MIXED, val)
  ERR_VAR(struct svalue, mixed, PIKE_T_MIXED, ind)
)

DECLARE_ERROR(bad_arg,
	      ERR_INHERIT(generic),
  ERR_VAR(INT_TYPE, int, PIKE_T_INT, which_arg)
  ERR_VAR(struct pike_string *,string,PIKE_T_STRING,expected_type)
  ERR_VAR(struct svalue, mixed, PIKE_T_MIXED, got)
)

DECLARE_ERROR(math,
	      ERR_INHERIT(generic),
  ERR_VAR(struct svalue, mixed, PIKE_T_MIXED, number)
)

DECLARE_ERROR(resource,
	      ERR_INHERIT(generic),
  ERR_VAR(struct pike_string *,string,PIKE_T_STRING,resource_type)
  ERR_VAR(INT_TYPE, int, PIKE_T_INT, howmuch)
)

DECLARE_ERROR(permission,
	      ERR_INHERIT(generic),
  ERR_VAR(struct pike_string *,string,PIKE_T_STRING,permission_type)
)

DECLARE_ERROR(cpp, ERR_INHERIT(generic), EMPTY)

DECLARE_ERROR(compilation, ERR_INHERIT(generic), EMPTY)

#undef DECLARE_ERROR
#undef ERR_INHERIT
#undef ERR_VAR
#undef EMPTY
#undef ERR_FUNC

#undef ERR_DECLARE
#undef ERR_EXT_DECLARE
#undef ERR_SETUP
#undef ERR_CLEANUP
