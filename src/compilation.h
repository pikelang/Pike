/*
 * $Id: compilation.h,v 1.12 1998/11/09 07:21:30 hubbe Exp $
 *
 * Compilator state push / pop operator construction file
 *
 * (Can you tell I like macros?)
 */

/*
 * IMEMBER: do not reset this member when popping
 * DMEMBER: This member should be the same when popping as when pushing.
 * ZMEMBER: reset this member to zero when pushing
 * STACKMEMBER: Like IMEMBER, but is not allowed to become more when popping
 *
 * defining STRUCT defines the structures
 * defining DECLARE creates global vars for saving linked list
 *                  of these lists...
 * defining PUSH pushes the selected state(s) on the stack(s)
 * defining POP pops the selected state(s) from the stack(s)
 *
 * define PROGRAM_STATE to select the program state
 */

#ifdef DEBUG
#define DO_DEBUG_CODE(X) X
#else
#define DO_DEBUG_CODE(X)
#endif

#ifdef STRUCT
#define IMEMBER(X,Y) X Y ;
#define DMEMBER(X,Y) X Y ;
#define STACKMEMBER(X,Y) X Y ;
#define IMEMBER2(X,Y,Z) X Y Z ;
#define ZMEMBER(X,Y) X Y ;
#define ZMEMBER2(X,Y,Z) X Y Z ;
#define SNAME(X,Y) struct X { struct X *previous;
#define SEND };
#endif

#ifdef DECLARE
#define IMEMBER(X,Y)
#define DMEMBER(X,Y)
#define STACKMEMBER(X,Y)
#define IMEMBER2(X,Y,Z)
#define ZMEMBER(X,Y)
#define ZMEMBER2(X,Y,Z)
#define SNAME(X,Y) static struct X * Y = 0;
#define SEND
#endif

#ifdef PUSH
#define IMEMBER(X,Y) MEMCPY((char *)&(oLd->Y), (char *)&(Y), sizeof(Y));
#define DMEMBER(X,Y) IMEMBER(X,Y)
#define STACKMEMBER(X,Y) (oLd->Y=Y);
#define IMEMBER2(X,Y,Z) IMEMBER(X,Y)
#define ZMEMBER(X,Y) MEMCPY((char *)&(oLd->Y), (char *)&(Y), sizeof(Y)); \
                     MEMSET((char *)&(Y), 0, sizeof(Y));
#define ZMEMBER2(X,Y,Z) ZMEMBER(X,Y)
#define SNAME(X,Y) { \
      struct X *oLd; \
      oLd=ALLOC_STRUCT(X); \
      oLd->previous=Y; Y=oLd;
#define SEND }

#endif


#ifdef POP
#define IMEMBER(X,Y) MEMCPY((char *)&(Y), (char *)&(oLd->Y), sizeof(Y));
#define IMEMBER2(X,Y,Z) IMEMBER(X,Y)
#define ZMEMBER(X,Y) MEMCPY((char *)&(Y), (char *)&(oLd->Y), sizeof(Y));
#define ZMEMBER2(X,Y,Z) ZMEMBER(X,Y)

#define DMEMBER(X,Y) DO_DEBUG_CODE( \
    if(MEMCMP((char *)&(Y), (char *)&(oLd->Y), sizeof(Y))) \
      fatal("Variable " #Y " became whacked during compilation.\n"); ) \
  IMEMBER(X,Y)

#define STACKMEMBER(X,Y) DO_DEBUG_CODE( \
    if(Y < oLd->Y) \
      fatal("Stack " #Y " shrunk %d steps compilation, currently: %p.\n",oLd->Y - Y,Y); ) \
  Y=oLd->Y;

#define SNAME(X,Y) { \
      struct X *oLd; \
      oLd=Y; Y=oLd->previous;
#define SEND free((char *)oLd); \
    }
#define PCODE(X) X
#else
#define PCODE(X)
#endif


#ifdef DEBUG
#define STRMEMBER(X,Y) \
  PCODE(if(X) fatal("Variable %s not deallocated properly.\n",Y);) \
  ZMEMBER(struct pike_string *,X)
#else
#define STRMEMBER(X,Y) \
  ZMEMBER(struct pike_string *,X)
#endif

  SNAME(program_state,previous_program_state)
  ZMEMBER(INT32,last_line)
  STRMEMBER(last_file,"last_file")
  ZMEMBER(struct object *,fake_object)
  ZMEMBER(struct program *,new_program)
  ZMEMBER(struct program *,malloc_size_program)
  ZMEMBER(node *,init_node)
  ZMEMBER(INT32,last_pc)
  ZMEMBER(int,num_parse_error)
  ZMEMBER(struct compiler_frame *,compiler_frame)
  ZMEMBER(INT32,num_used_modules)
  IMEMBER(int,compiler_pass)
  ZMEMBER(int,local_class_counter)
  ZMEMBER(int,catch_level)
  ZMEMBER(struct mapping *,module_index_cache)
  STACKMEMBER(unsigned char *,type_stackp)
  STACKMEMBER(unsigned char **,pike_type_mark_stackp)
  SEND

#undef PCODE
#undef STRMEMBER
#undef IMEMBER
#undef DMEMBER
#undef ZMEMBER
#undef IMEMBER2
#undef ZMEMBER2
#undef SNAME
#undef SEND
#undef STACKMEMBER

#undef STRUCT
#undef PUSH
#undef POP
#undef DECLARE
#undef DO_DEBUG_CODE
