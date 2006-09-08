/*
 * Machine code generator for AMD64.
 */

#include "operators.h"
#include "constants.h"
#include "object.h"
#include "builtin_functions.h"


/* This is defined on windows */
#ifdef REG_NONE
#undef REG_NONE
#endif


/* Register encodings */
enum amd64_reg {REG_RAX = 0, REG_RBX = 3, REG_RCX = 1, REG_RDX = 2,
				 REG_R8 = 8, REG_R9 = 9, REG_10 = 10, REG_R11 = 11,
				 REG_R12 = 12, REG_R13 = 13, REG_R14 = 14, REG_R15 = 15, REG_NONE = 4};

#define CLEAR_REGS() do {} while (0)

#define PUSH_INT(X) ins_int((INT32)(X), (void (*)(char))add_to_program)

/*
 * Copies a 32 bit immidiate value to stack
 * 0xc7 = mov 
 */
#define MOV_VAL_TO_RELSTACK(VALUE, OFFSET) do {	\
	INT32 off_ = (OFFSET);								\
	add_to_program(0xc7);								\
	  if (off_ < -128 || off_ > 127) {					\
      add_to_program (0x84);							\
      add_to_program (0x24);							\
      PUSH_INT (off_);									\
    }													\
    else if (off_) {									\
      add_to_program (0x44);							\
      add_to_program (0x24);							\
      add_to_program (off_);							\
    }													\
    else {												\
      add_to_program (0x04);							\
      add_to_program (0x24);							\
    }													\			
	PUSH_INT(VALUE);									\
} while(0)												\



/* 0xe8 = call */
#define CALL_RELATIVE(X) do{							\
  struct program *p_=Pike_compiler->new_program;		\
  add_to_program(0xe8);									\
  add_to_program(0);                                  \
  add_to_program(0);                                  \
  add_to_program(0);                                  \
  add_to_program(0);                                  \
  add_to_relocations(p_->num_program-4);				\
  *(INT32 *)(p_->program + p_->num_program - 4)=      \
    ((INT32)(X)) - (INT32)(p_->program + p_->num_program);   \
}while(0)

static void update_arg1(INT32 value) {
  MOV_VAL_TO_RELSTACK(value, 0);
}

static void update_arg2(INT32 value) {
  MOV_VAL_TO_RELSTACK(value, 4);
}


static enum amd64_reg next_reg;
static enum amd64_reg sp_reg, fp_reg, mark_sp_reg;
ptrdiff_t amd64_prev_stored_pc; /* PROG_PC at the last point Pike_fp->pc was updated. */


static void amd64_call_c_function(void *addr) {
	CALL_RELATIVE(addr);
	next_reg = REG_RAX;
	sp_reg = fp_reg = mark_sp_reg = REG_NONE;
	CLEAR_REGS();
}


void amd64_update_pc(void) {
	INT32 tmp = PIKE_PC, disp;
	
	if (amd64_prev_stored_pc < 0) {
		enum amd64_reg tmp_reg = alloc_reg (1 << fp_reg);
		load_fp_reg (1 << tmp_reg);
#ifdef PIKE_DEBUG
    if (a_flag >= 60)
      fprintf (stderr, "pc %d  update pc absolute\n", tmp);
#endif
    /* Store the negated pointer to make the relocation displacements
     * work in the right direction. */
    MOV_VAL32_TO_REG (0, tmp_reg);
    add_to_relocations(PIKE_PC - 4);
    upd_pointer(PIKE_PC - 4, - (INT32) (tmp + Pike_compiler->new_program->program));
    CHECK_VALID_REG (tmp_reg);
    add_to_program(0xf7);		/* neg tmp_reg */
    add_to_program(0xd8 | tmp_reg);
    MOV_REG_TO_RELADDR (tmp_reg, OFFSETOF (pike_frame, pc), fp_reg);
    DEALLOC_REG (tmp_reg);
  }
  else if ((disp = tmp - ia32_prev_stored_pc)) {
#ifdef PIKE_DEBUG
    if (a_flag >= 60)
      fprintf (stderr, "pc %d  update pc relative: %d\n", tmp, disp);
#endif
    load_fp_reg (0);
    ADD_VAL_TO_RELADDR (disp, OFFSETOF (pike_frame, pc), fp_reg);
  }
  else {
#ifdef PIKE_DEBUG
    if (a_flag >= 60)
      fprintf (stderr, "pc %d  update pc - already up-to-date\n", tmp);
#endif
  }
  amd64_prev_stored_pc = tmp;
}


static void maybe_update_pc(void) {
	static int last_prog_id=-1;
	static size_t last_num_linenumbers=-1;
	
	if(
#ifdef PIKE_DEBUG
    /* Update the pc more often for the sake of the opcode level trace. */
     d_flag ||
#endif
     (amd64_prev_stored_pc == -1) ||
     last_prog_id != Pike_compiler->new_program->id ||
     last_num_linenumbers != Pike_compiler->new_program->num_linenumbers
  ) {
    last_prog_id=Pike_compiler->new_program->id;
    last_num_linenumbers = Pike_compiler->new_program->num_linenumbers;
    UPDATE_PC();
  }
}

void ins_f_byte(unsigned int b) {
	 void *addr;
	 b-=F_OFFSET;
#ifdef PIKE_DEBUG
  	if(b>255)
		Pike_error("Instruction too big %d\n",b);
#endif
  	maybe_update_pc();
	addr=instrs[b].address;
	amd64_call_c_function(addr);
}

void ins_f_byte_with_arg(unsigned int a, INT32 b) {
	maybe_update_pc();
	update_arg1(b);
	ins_f_byte(a);
}

void ins_f_byte_with_2_args(unsigned int a, INT32 b, INT32 c) {
	maybe_update_pc();
	update_arg1(b);
	update_arg2(c);
	ins_f_byte(a);
}

