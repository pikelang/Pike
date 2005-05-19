/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: bytecode.h,v 1.11 2005/05/19 22:35:35 mast Exp $
*/

#define UPDATE_PC()

#define ins_pointer(PTR)	add_relocated_int_to_program((PTR))
#define read_pointer(OFF)	read_int(OFF)
#define upd_pointer(OFF, PTR)	upd_int((OFF), (PTR))
#define ins_align(ALIGN)	do { \
    while (Pike_compiler->new_program->num_program % (ALIGN)) { \
      add_to_program(0); \
    } \
  } while(0)
#define ins_byte(VAL)		add_to_program((VAL))
#define ins_data(VAL)		add_relocated_int_to_program((VAL))

#define PROG_COUNTER pc

#define READ_INCR_BYTE(PC)	EXTRACT_UCHAR((PC)++)

#define CHECK_RELOC(REL, PROG_SIZE)		\
  do {						\
    if ((REL) > (PROG_SIZE)-4) {		\
      Pike_error("Bad relocation: %"PRINTSIZET"d > %"PRINTSIZET"d\n",	\
		 (REL), (PROG_SIZE)-4);		\
    }						\
  } while(0)

/* FIXME: Uses internal variable 'byteorder'. */
#define DECODE_PROGRAM(P)						\
  do {									\
    struct program *p_ = (P);						\
    int num_reloc = (int)p_->num_relocations;				\
    if (byteorder != PIKE_BYTEORDER) {					\
      int e;								\
      /* NOTE: Only 1234 <==> 4321 byte-order relocation supported. */	\
      for (e=0; e<num_reloc; e++) {					\
	size_t reloc = p_->relocations[e];				\
	unsigned INT8 tmp1;						\
	unsigned INT8 tmp2;						\
	tmp1 = p_->program[reloc];					\
	tmp2 = p_->program[reloc+1];					\
	p_->program[reloc] = p_->program[reloc+3];			\
	p_->program[reloc+1] = p_->program[reloc+2];			\
	p_->program[reloc+3] = tmp1;					\
	p_->program[reloc+2] = tmp2;					\
      }									\
    }									\
  } while(0)
