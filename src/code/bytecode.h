/*
 * $Id: bytecode.h,v 1.2 2001/07/20 13:28:04 grubba Exp $
 */

#define UPDATE_PC()

#define ins_pointer(PTR)	ins_int((PTR), (void (*)(char))add_to_program)
#define read_pointer(OFF)	read_int(OFF)
#define upd_pointer(OFF, PTR)	upd_int((OFF), (PTR))
#define ins_align(ALIGN)	do { \
    while (Pike_compiler->new_program->num_program % (ALIGN)) { \
      add_to_program(0); \
    } \
  } while(0)
#define ins_byte(VAL)		add_to_program((VAL))
#define ins_data(VAL)		ins_int((VAL), (void (*)(char))add_to_program)
