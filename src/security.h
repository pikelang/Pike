#include "global.h"

#ifndef SECURITY_H
#define SECURITY_H

#ifdef PIKE_SECURITY
#include "object.h"

#define SECURITY_BIT_INDEX 1      /* index read-only */
#define SECURITY_BIT_SET_INDEX 2  /* set index */
#define SECURITY_BIT_CALL 4       /* call functions (and clone?) */
#define SECURITY_BIT_SECURITY 8   /* Do anything :) */
#define SECURITY_BIT_NOT_SETUID 16
#define SECURITY_BIT_CONDITIONAL_IO 32
#define SECURITY_BIT_DESTRUCT 64

typedef unsigned char pike_security_bits;

struct pike_creds
{
  struct object *user;
  struct object *default_creds;
  pike_security_bits data_bits;
  pike_security_bits may_always;
};

#define CHECK_VALID_CREDS(X) (X)
#define CHECK_VALID_UID(X) (X)

#define OBJ2CREDS(O) ((struct pike_creds *)(CHECK_VALID_CREDS(O)->storage))

#define CURRENT_CREDS \
  (Pike_interpreter.frame_pointer ? Pike_interpreter.frame_pointer->current_creds : Pike_interpreter.current_creds)

/* Do we need a 'may never' ? */
#define CHECK_SECURITY(BIT) \
   (!CURRENT_CREDS || (OBJ2CREDS(CURRENT_CREDS)->may_always & (BIT)))

#define CHECK_DATA_SECURITY(DATA,BIT) (\
   CHECK_SECURITY(BIT) || \
   !(DATA)->prot || (OBJ2CREDS((DATA)->prot)->data_bits & (BIT)) || \
   (OBJ2CREDS((DATA)->prot)->user == OBJ2CREDS(CURRENT_CREDS)->user) )

#define CHECK_DATA_SECURITY_OR_ERROR(DATA,BIT,ERR) do {	\
  if(!CHECK_DATA_SECURITY(DATA,BIT))             \
     Pike_error ERR;					\
 }while(0)

#define CHECK_SECURITY_OR_ERROR(BIT,ERR) do { \
  if(!CHECK_SECURITY(BIT))             \
     Pike_error ERR;					\
 }while(0)

#define SET_CURRENT_CREDS(O) do { struct object *_o=(O);		\
 if(Pike_interpreter.frame_pointer)					\
 {									\
   if(Pike_interpreter.frame_pointer->current_creds)			\
     free_object(Pike_interpreter.frame_pointer->current_creds);	\
   Pike_interpreter.frame_pointer->current_creds = CHECK_VALID_UID(_o);	\
 }else{									\
   if(Pike_interpreter.current_creds)					\
     free_object(Pike_interpreter.current_creds);			\
   Pike_interpreter.current_creds = CHECK_VALID_UID(_o);		\
 }									\
 if (_o)								\
   add_ref(_o);								\
}while(0)

#define INITIALIZE_PROT(X) \
  do { if(CURRENT_CREDS) add_ref((X)->prot=CHECK_VALID_CREDS(OBJ2CREDS(CURRENT_CREDS)->default_creds?OBJ2CREDS(CURRENT_CREDS)->default_creds:CURRENT_CREDS)); else (X)->prot=0; }while(0)

#define FREE_PROT(X) do { if((X)->prot) free_object((X)->prot); (X)->prot=0; }while(0)

#define VALID_FILE_IO(name, access_type)				\
  if(!CHECK_SECURITY(SECURITY_BIT_SECURITY))				\
  {									\
    int e;								\
    struct svalue *base_sp=Pike_sp-args;					\
									\
    if(!CHECK_SECURITY(SECURITY_BIT_CONDITIONAL_IO))			\
      Pike_error(name ": Permission denied.\n");				\
									\
    push_constant_text(name);						\
    push_constant_text(access_type);					\
									\
    for(e=0;e<args;e++) push_svalue(base_sp+e);				\
									\
    safe_apply(OBJ2CREDS(CURRENT_CREDS)->user,"valid_io",args+2);	\
									\
    switch(Pike_sp[-1].type)							\
    {									\
      case PIKE_T_ARRAY:							\
      case PIKE_T_OBJECT:							\
      case PIKE_T_MAPPING:							\
	assign_svalue(Pike_sp-args-1,Pike_sp-1);					\
	pop_n_elems(args);						\
	return;								\
									\
      case PIKE_T_INT:							\
	switch(Pike_sp[-1].u.integer)					\
	{								\
	  case 0: /* return 0 */					\
	    errno=EPERM;						\
	    pop_n_elems(args+1);					\
	    push_int(0);						\
	    return;							\
									\
	  case 1: /* return 1 */					\
	    pop_n_elems(args+1);					\
	    push_int(1);						\
	    return;							\
	    								\
	  case 2: /* ok */						\
	    pop_stack();						\
	    break;							\
	    								\
	  case 3: /* permission denied */				\
	    Pike_error(name ": permission denied.\n");			\
	    								\
	  default:							\
	    Pike_error("Error in user->valid_io, wrong return value.\n");	\
	}								\
	break;								\
									\
      default:								\
	Pike_error("Error in user->valid_io, wrong return type.\n");		\
									\
      case PIKE_T_STRING:							\
	assign_svalue(Pike_sp-args-1,Pike_sp-1);					\
        pop_stack();							\
    }									\
  }


/* Prototypes begin here */
void init_pike_security(void);
void exit_pike_security(void);
/* Prototypes end here */

#else

#define INITIALIZE_PROT(X)
#define FREE_PROT(X)
#define CHECK_SECURITY_OR_ERROR(BIT,ERR)
#define CHECK_DATA_SECURITY_OR_ERROR(DATA,BIT,ERR)

#define init_pike_security()
#define exit_pike_security()

#define VALID_FILE_IO(name, type)

#endif

#endif
