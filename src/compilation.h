/*
 * $Id: compilation.h,v 1.9 1998/04/24 00:32:08 hubbe Exp $
 *
 * Compilator state push / pop operator construction file
 *
 * (Can you tell I like macros?)
 */

/*
 * IMEMBER: do not reset this member when popping
 * ZMEMBER: reset this member to zero when pushing
 *
 * defining STRUCT defines the structures
 * defining DECLARE creates global vars for saving linked list
 *                  of these lists...
 * defining PUSH pushes the selected state(s) on the stack(s)
 * defining POP pops the selected state(s) from the stack(s)
 *
 * define PROGRAM_STATE to select the program state
 */

#ifdef STRUCT
#define IMEMBER(X,Y) X Y ;
#define IMEMBER2(X,Y,Z) X Y Z ;
#define ZMEMBER(X,Y) X Y ;
#define ZMEMBER2(X,Y,Z) X Y Z ;
#define SNAME(X,Y) struct X { struct X *previous;
#define SEND };
#endif

#ifdef DECLARE
#define IMEMBER(X,Y)
#define IMEMBER2(X,Y,Z)
#define ZMEMBER(X,Y)
#define ZMEMBER2(X,Y,Z)
#define SNAME(X,Y) static struct X * Y = 0;
#define SEND
#endif

#ifdef PUSH
#define IMEMBER(X,Y) MEMCPY((char *)&(oLd->Y), (char *)&(Y), sizeof(Y));
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
  IMEMBER(int,comp_stackp)
  IMEMBER(int,compiler_pass)
  ZMEMBER(int,local_class_counter)
  ZMEMBER(int,catch_level)
  ZMEMBER(struct mapping *,module_index_cache)
  SEND

#undef PCODE
#undef STRMEMBER
#undef IMEMBER
#undef ZMEMBER
#undef IMEMBER2
#undef ZMEMBER2
#undef SNAME
#undef SEND

#undef STRUCT
#undef PUSH
#undef POP
#undef DECLARE
