/*
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
 * define FILE_STATE to select the file state
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
#define SEND free(oLd); \
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

#ifdef FILE_STATE
  SNAME(file_state,previous_file_state)
  ZMEMBER(INT32,old_line)
  ZMEMBER(INT32,current_line)
  ZMEMBER(INT32,nexpands)
  ZMEMBER(int,pragma_all_inline)
  ZMEMBER(struct inputstate *,istate)
  ZMEMBER(struct hash_table *,defines)
  STRMEMBER(current_file,"current_file")
  SEND
#endif

#ifdef PROGRAM_STATE
  SNAME(program_state,previous_program_state)
  ZMEMBER(INT32,last_line)
  STRMEMBER(last_file,"last_file")
  ZMEMBER(struct program,fake_program)
  ZMEMBER(node *,init_node)
  ZMEMBER(INT32,last_pc)
  ZMEMBER(int,num_parse_error)
  ZMEMBER(struct locals *,local_variables)
  ZMEMBER(dynamic_buffer,inherit_names)
  ZMEMBER2(dynamic_buffer,areas,[NUM_AREAS])
  IMEMBER(int,comp_stackp)
  SEND
#endif

#undef PCODE
#undef STRMEMBER
#undef IMEMBER
#undef ZMEMBER
#undef IMEMBER2
#undef ZMEMBER2
#undef SNAME
#undef SEND
