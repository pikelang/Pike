/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: compilation.h,v 1.31 2004/03/13 14:45:05 grubba Exp $
*/

/*
 * Compilator state push / pop operator construction file
 *
 * (Can you tell I like macros?)
 */

/*
 * IMEMBER: do not reset this member when pushing
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

#ifdef PIKE_DEBUG
#define DO_DEBUG_CODE(X) X
#else
#define DO_DEBUG_CODE(X)
#endif

#ifdef STRUCT
#define IMEMBER(X,Y,Z) X Y ;
#define DMEMBER(X,Y,Z) X Y ;
#define STACKMEMBER(X,Y,Z) X Y ;
#define IMEMBER2(X,Y,Z,Q) X Y Z ;
#define ZMEMBER(X,Y,Z) X Y ;
#define ZMEMBER2(X,Y,Z,Q) X Y Z ;
#define SNAME(X,Y) struct X { struct X *previous;
#define SEND };
#endif

#ifdef EXTERN
#define IMEMBER(X,Y,Z)
#define DMEMBER(X,Y,Z)
#define STACKMEMBER(X,Y,z)
#define IMEMBER2(X,Y,Z,Q)
#define ZMEMBER(X,Y,Z)
#define ZMEMBER2(X,Y,Z,Q)
#define SNAME(X,Y) extern struct X * Y;
#define SEND
#endif

#ifdef DECLARE
#define IMEMBER(X,Y,Z) Z,
#define DMEMBER(X,Y,Z) Z,
#define STACKMEMBER(X,Y,Z) Z,
#define IMEMBER2(X,Y,Z,Q) Q,
#define ZMEMBER(X,Y,Z) Z,
#define ZMEMBER2(X,Y,Z,Q) Q,
#define SNAME(X,Y) \
  extern struct X PIKE_CONCAT(Y,_base); \
  struct X * Y = & PIKE_CONCAT(Y,_base); \
  struct X PIKE_CONCAT(Y,_base) = { 0, 
#define SEND };
#endif

#ifdef PUSH
#define IMEMBER(X,Y,Z) MEMCPY((char *)&(nEw->Y), (char *)&(Pike_compiler->Y), sizeof(nEw->Y));
#define DMEMBER(X,Y,Z) IMEMBER(X,Y)
#define STACKMEMBER(X,Y,Z) (nEw->Y=Pike_compiler->Y);
#define IMEMBER2(X,Y,Z,Q) IMEMBER(X,Y)
#define ZMEMBER(X,Y,Z) MEMSET((char *)&(nEw->Y), 0, sizeof(nEw->Y));
#define ZMEMBER2(X,Y,Z,Q) ZMEMBER(X,Y)
#define SNAME(X,Y) { \
      struct X *nEw; \
      nEw=ALLOC_STRUCT(X); \
      nEw->previous=Pike_compiler;
#define SEND \
      Pike_compiler=nEw; \
      }

#endif


#ifdef POP
#define IMEMBER(X,Y,Z) 
#define IMEMBER2(X,Y,Z,Q) IMEMBER(X,Y)
#define ZMEMBER(X,Y,Z) 
#define ZMEMBER2(X,Y,Z,Q) ZMEMBER(X,Y)

#define DMEMBER(X,Y,Z) DO_DEBUG_CODE( \
    if(MEMCMP((char *)&(Pike_compiler->Y), (char *)&(oLd->Y), sizeof(oLd->Y))) \
      Pike_fatal("Variable " #Y " became whacked during compilation.\n"); ) \
  IMEMBER(X,Y)

#define STACKMEMBER(X,Y,Z) DO_DEBUG_CODE( \
    if(Pike_compiler->Y < oLd->Y) \
      Pike_fatal("Stack " #Y " shrunk %ld steps compilation, currently: %p.\n", \
            PTRDIFF_T_TO_LONG(oLd->Y - Pike_compiler->Y), Pike_compiler->Y); )

#define SNAME(X,Y) { \
      struct X *oLd=Pike_compiler->previous;

#define SEND \
     free((char *)Pike_compiler); \
     Pike_compiler=oLd; \
    }

#define PCODE(X) X
#else
#define PCODE(X)
#endif


#ifdef PIKE_DEBUG
#define STRMEMBER(X,Y) \
  PCODE(if(Pike_compiler->X) Pike_fatal("Variable " #X " not deallocated properly.\n");) \
  ZMEMBER(struct pike_string *,X,Y)
#else
#define STRMEMBER(X,Y) \
  ZMEMBER(struct pike_string *,X,Y)
#endif

  SNAME(program_state,Pike_compiler)
  ZMEMBER(INT32,last_line,0)
  STRMEMBER(last_file,0)
  ZMEMBER(struct object *,fake_object,0)
  ZMEMBER(struct program *,new_program,0)
  ZMEMBER(struct program *,malloc_size_program,0)
  ZMEMBER(node *,init_node,0)
  ZMEMBER(INT32,last_pc,0)
  ZMEMBER(int,num_parse_error,0)
  ZMEMBER(struct compiler_frame *,compiler_frame,0)
  ZMEMBER(INT32,num_used_modules,0)
  IMEMBER(int,compiler_pass,0)
  ZMEMBER(int,local_class_counter,0)
  ZMEMBER(int,catch_level,0)
  ZMEMBER(INT32,current_modifiers,0)
  ZMEMBER(int,varargs,0)
  STRMEMBER(last_identifier,0)
  ZMEMBER(struct mapping *,module_index_cache,0)
  STACKMEMBER(struct pike_type **,type_stackp,type_stack)
  STACKMEMBER(struct pike_type ***,pike_type_mark_stackp,pike_type_mark_stack)
  ZMEMBER(INT32,parent_identifier,0)
  IMEMBER(int, compat_major, PIKE_MAJOR_VERSION)
  IMEMBER(int, compat_minor, PIKE_MINOR_VERSION)
  ZMEMBER(int, flags, 0)
  ZMEMBER(struct compilation *,compiler,0)
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

#undef EXTERN
#undef STRUCT
#undef PUSH
#undef POP
#undef DECLARE
#undef DO_DEBUG_CODE
