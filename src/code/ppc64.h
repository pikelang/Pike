/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: ppc64.h,v 1.1 2007/03/31 22:59:54 marcus Exp $
*/

#define PPC_INSTR_B_FORM(OPCD,BO,BI,BD,AA,LK)			\
      add_to_program(((OPCD)<<26)|((BO)<<21)|((BI)<<16)|	\
		     (((BD)&0x3fff)<<2)|((AA)<<1)|(LK))
#define PPC_INSTR_D_FORM(OPCD,S,A,d)					\
      add_to_program(((OPCD)<<26)|((S)<<21)|((A)<<16)|(((INT32)(d))&0xffff))
#define PPC_INSTR_DS_FORM(OPCD,S,A,ds,XO)				\
      add_to_program(((OPCD)<<26)|((S)<<21)|((A)<<16)|(((INT32)(ds))&0xfffc)|(XO))
#define PPC_INSTR_I_FORM(OPCD,LI,AA,LK)				\
      add_to_program(((OPCD)<<26)|((LI)&0x03fffffc)|((AA)<<1)|(LK))
#define PPC_INSTR_M_FORM(OPCD,S,A,SH,MB,ME,Rc)				\
      add_to_program(((OPCD)<<26)|((S)<<21)|((A)<<16)|((SH)<<11)|	\
		     ((MB)<<6)|((ME)<<1)|(Rc))
#define PPC_INSTR_MD_FORM(OPCD,S,A,SH,MB,XO,Rc)				\
      add_to_program(((OPCD)<<26)|((S)<<21)|((A)<<16)|(((SH)&31)<<11)|	\
		     (((MB)&31)<<6)|((MB)&32)|((XO)<<2)|(((SH)>>4)&2)|(Rc))
#define PPC_INSTR_XL_FORM(OPCD,BO,BI,CRBB,XO,LK) \
      add_to_program(((OPCD)<<26)|((BO)<<21)|((BI)<<16)|((CRBB)<<11)|	\
		     ((XO)<<1)|(LK))
#define PPC_INSTR_XFX_FORM(OPCD,S,SPR,XO)			\
      add_to_program(((OPCD)<<26)|((S)<<21)|((SPR)<<11)|((XO)<<1))

#define BC(BO,BI,BD) PPC_INSTR_B_FORM(16,BO,BI,BD,0,0)

#define CMPLI(crfD,A,UIMM) PPC_INSTR_D_FORM(10,crfD,A,UIMM)
#define ADDIC(D,A,SIMM) PPC_INSTR_D_FORM(12,D,A,SIMM)
#define ADDI(D,A,SIMM) PPC_INSTR_D_FORM(14,D,A,SIMM)
#define ADDIS(D,A,SIMM) PPC_INSTR_D_FORM(15,D,A,SIMM)
#define ORI(A,S,UIMM) PPC_INSTR_D_FORM(24,S,A,UIMM)
#define ORIS(A,S,UIMM) PPC_INSTR_D_FORM(25,S,A,UIMM)
#define LWZ(D,A,d) PPC_INSTR_D_FORM(32,D,A,d)
#define STW(S,A,d) PPC_INSTR_D_FORM(36,S,A,d)
#define LHA(D,A,d) PPC_INSTR_D_FORM(42,D,A,d)
#define LD(D,A,ds) PPC_INSTR_DS_FORM(58,D,A,ds,0)
#define STD(S,A,ds) PPC_INSTR_DS_FORM(62,S,A,ds,0)

#define RLWINM(S,A,SH,MB,ME) PPC_INSTR_M_FORM(21,S,A,SH,MB,ME,0)
#define RLDICL(S,A,SH,MB) PPC_INSTR_MD_FORM(30,S,A,SH,MB,0,0)
#define RLDICR(S,A,SH,ME) PPC_INSTR_MD_FORM(30,S,A,SH,ME,1,0)

#define MFSPR(D,SPR) PPC_INSTR_XFX_FORM(31,D,(((SPR)&0x1f)<<5)|(((SPR)&0x3e0)>>5),339)
#define MTSPR(D,SPR) PPC_INSTR_XFX_FORM(31,D,(((SPR)&0x1f)<<5)|(((SPR)&0x3e0)>>5),467)

#define BCLR(BO,BI) PPC_INSTR_XL_FORM(19,BO,BI,0,16,0)
#define BCLRL(BO,BI) PPC_INSTR_XL_FORM(19,BO,BI,0,16,1)
#define B(LI) PPC_INSTR_I_FORM(18,LI,0,0)
#define BL(LI) PPC_INSTR_I_FORM(18,LI,0,1)
#define BLA(LI) PPC_INSTR_I_FORM(18,LI,1,1)

#define LOW_GET_JUMP()	((INT32)PROG_COUNTER[JUMP_EPILOGUE_SIZE])
#define LOW_SKIPJUMP()	(SET_PROG_COUNTER(PROG_COUNTER + JUMP_EPILOGUE_SIZE + 1))

#define SET_REG32(REG, X) do {						  \
    INT32 val_ = X;							  \
    INT32 reg_ = REG;							  \
    if ((-32768 <= val_) && (val_ <= 32767)) {				  \
      /* addi reg,0,val */						  \
      ADDI(reg_, 0, val_);						  \
    } else {								  \
      /* addis reg,0,%hi(val) */					  \
      ADDIS(reg_, 0, val_ >> 16);					  \
      if (val_ & 0xffff) {						  \
	/* ori reg,reg,%lo(val) */					  \
	ORI(reg_, reg_, val_);						  \
      }									  \
    }									  \
  } while(0)

#define SET_REG64(REG, x) do {					\
    INT64 val__ = x;						\
    INT32 reg__ = REG;						\
    if ((-2147483648 <= val__) && (val__ <= 2147483647)) {	\
      SET_REG32(reg__, val__);					\
    } else {							\
      SET_REG32(reg__, val__>>32);				\
      /* rldicr reg,reg,32,31 */				\
      RLDICR(reg__, reg__, 32, 31);				\
      if (val__ & 0xffff0000) {					\
	/* oris reg,reg,%hi(val) */				\
	ORIS(reg__, reg__, val__ >> 16);			\
      }								\
      if (val__ & 0xffff) {					\
	/* ori reg,reg,%lo(val) */				\
	ORI(reg__, reg__, val__);				\
      }								\
    }								\
  } while(0)

#define PPC_SPREG_LR 8

#define PPC_REG_RET 3

#define PPC_REG_ARG1 3
#define PPC_REG_ARG2 4
#define PPC_REG_ARG3 5

#define PPC_REG_PIKE_PC 7
#define PPC_REG_PIKE_MARK_SP 8
#define PPC_REG_PIKE_FP 9
#define PPC_REG_PIKE_SP 10

#define PPC_REG_PIKE_INTERP 29 /* 31 */

extern int ppc64_codegen_state, ppc64_codegen_last_pc;
void ppc64_flush_code_generator_state(void);
#define FLUSH_CODE_GENERATOR_STATE ppc64_flush_code_generator_state

#define PPC_CODEGEN_FP_ISSET 1
#define PPC_CODEGEN_SP_ISSET 2
#define PPC_CODEGEN_SP_NEEDSSTORE 4
#define PPC_CODEGEN_MARK_SP_ISSET 8
#define PPC_CODEGEN_MARK_SP_NEEDSSTORE 16
#define PPC_CODEGEN_PC_ISSET 32

#define LOAD_FP_REG() do {				\
    if(!(ppc64_codegen_state & PPC_CODEGEN_FP_ISSET)) {	\
      /* ld pike_fp,frame_pointer(pike_interpreter) */	\
      LD(PPC_REG_PIKE_FP, PPC_REG_PIKE_INTERP, 	\
	 OFFSETOF(Pike_interpreter, frame_pointer));	\
      ppc64_codegen_state |= PPC_CODEGEN_FP_ISSET;	\
    }							\
  } while(0)

#define LOAD_SP_REG() do {				\
    if(!(ppc64_codegen_state & PPC_CODEGEN_SP_ISSET)) {	\
      /* ld pike_sp,stack_pointer(pike_interpreter) */	\
      LD(PPC_REG_PIKE_SP, PPC_REG_PIKE_INTERP,		\
	  OFFSETOF(Pike_interpreter, stack_pointer));	\
      ppc64_codegen_state |= PPC_CODEGEN_SP_ISSET;	\
    }							\
  } while(0)

#define LOAD_MARK_SP_REG() do {						\
    if(!(ppc64_codegen_state & PPC_CODEGEN_MARK_SP_ISSET)) {		\
      /* ld pike_mark_sp,mark_stack_pointer(pike_interpreter) */	\
      LD(PPC_REG_PIKE_MARK_SP, PPC_REG_PIKE_INTERP,			\
	 OFFSETOF(Pike_interpreter, mark_stack_pointer));		\
      ppc64_codegen_state |= PPC_CODEGEN_MARK_SP_ISSET;			\
    }									\
  } while(0)

#define INCR_SP_REG(n) do {				\
      /* addi pike_sp,pike_sp,n */			\
      ADDI(PPC_REG_PIKE_SP, PPC_REG_PIKE_SP, n);	\
      ppc64_codegen_state |= PPC_CODEGEN_SP_NEEDSSTORE;	\
    } while(0)

#define INCR_MARK_SP_REG(n) do {				\
      /* addi pike_mark_sp,pike_mark_sp,n */			\
      ADDI(PPC_REG_PIKE_MARK_SP, PPC_REG_PIKE_MARK_SP, n);	\
      ppc64_codegen_state |= PPC_CODEGEN_MARK_SP_NEEDSSTORE;	\
    } while(0)

#define UPDATE_PC() do {						\
    size_t tmp = PIKE_PC;						\
    if(ppc64_codegen_state & PPC_CODEGEN_PC_ISSET) {			\
      INT32 diff = (tmp-ppc64_codegen_last_pc)*sizeof(PIKE_OPCODE_T);	\
      if (diff) {							\
	if ((-32768 <= diff) && (diff <= 32767)) {			\
	  /* addi pike_pc,pike_pc,diff */				\
	  ADDI(PPC_REG_PIKE_PC, PPC_REG_PIKE_PC, diff);			\
	} else {							\
	  /* addis pike_pc,pike_pc,%hi(diff) */				\
	  ADDIS(PPC_REG_PIKE_PC, PPC_REG_PIKE_PC, (diff+32768)>>16);	\
	  if ((diff &= 0xffff) > 32767)					\
	    diff -= 65536;						\
	  if (diff) {							\
	    /* addi pike_pc,pike_pc,%lo(diff) */			\
	    ADDI(PPC_REG_PIKE_PC, PPC_REG_PIKE_PC, diff);		\
	  }								\
	}								\
      }									\
    } else {								\
      /* bl .+4 */							\
      BL(4);								\
      /* mflr pike_pc */						\
      MFSPR(PPC_REG_PIKE_PC, PPC_SPREG_LR);				\
      /* addi pike_pc,pike_pc,-4 */					\
      ADDI(PPC_REG_PIKE_PC, PPC_REG_PIKE_PC, -sizeof(PIKE_OPCODE_T));	\
    }									\
    ppc64_codegen_last_pc = tmp;					\
    ppc64_codegen_state |= PPC_CODEGEN_PC_ISSET;			\
    LOAD_FP_REG();							\
    /* std pike_pc,pc(pike_fp) */					\
    STD(PPC_REG_PIKE_PC, PPC_REG_PIKE_FP, OFFSETOF(pike_frame, pc));	\
  } while(0)

#define ADJUST_PIKE_PC(pc) do {						\
    ppc64_codegen_last_pc = pc;						\
    ppc64_codegen_state |= PPC_CODEGEN_PC_ISSET;			\
  } while (0)

#define ins_pointer(PTR)  add_to_program((INT32)(PTR))
#define read_pointer(OFF) (Pike_compiler->new_program->program[(INT32)(OFF)])
#define upd_pointer(OFF,PTR) (Pike_compiler->new_program->program[(INT32)(OFF)] = (INT32)(PTR))
#define ins_align(ALIGN)
#define ins_byte(VAL)	  add_to_program((INT32)(VAL))
#define ins_data(VAL)	  add_to_program((INT32)(VAL))
#define read_program_data(PTR, OFF)	((INT32)((PTR)[OFF]))

INT32 ppc64_ins_f_jump(unsigned int a, int backward_jump);
INT32 ppc64_ins_f_jump_with_arg(unsigned int a, unsigned INT32 b, int backward_jump);
INT32 ppc64_ins_f_jump_with_2_args(unsigned int a, unsigned INT32 b, unsigned INT32 c, int backward_jump);
void ppc64_update_f_jump(INT32 offset, INT32 to_offset);
INT32 ppc64_read_f_jump(INT32 offset);
#define INS_F_JUMP ppc64_ins_f_jump
#define INS_F_JUMP_WITH_ARG ppc64_ins_f_jump_with_arg
#define INS_F_JUMP_WITH_TWO_ARGS ppc64_ins_f_jump_with_2_args
#define UPDATE_F_JUMP ppc64_update_f_jump
#define READ_F_JUMP ppc64_read_f_jump

#define READ_INCR_BYTE(PC)	(((PC)++)[0])

#if 0
#define RELOCATE_program(P, NEW)	do {			\
    PIKE_OPCODE_T *op_ = NEW;					\
    struct program *p_ = P;					\
    size_t rel_ = p_->num_relocations;				\
    INT32 disp_, delta_ = p_->program - op_;			\
    while (rel_--) {						\
      DO_IF_DEBUG(						\
        if ((op_[p_->relocations[rel_]] & 0xfc000002) !=	\
	    0x48000000) {					\
          Pike_fatal("Bad relocation: %d, off:%d, opcode: 0x%08x\n",	\
		rel_, p_->relocations[rel_],			\
		op_[p_->relocations[rel_]]);			\
	}							\
      );							\
      disp_ = op_[p_->relocations[rel_]] & 0x03ffffff;		\
      if(disp_ & 0x02000000)					\
	disp_ -= 0x04000000;					\
      disp_ += delta_ << 2;					\
      if(disp_ < -33554432 || disp_ > 33554431)			\
	Pike_fatal("Relocation %d out of range!\n", disp_);		\
      op_[p_->relocations[rel_]] = 0x48000000 |			\
	(disp_ & 0x03ffffff);					\
    }								\
  } while(0)
#endif

extern void ppc64_flush_instruction_cache(void *addr, size_t len);
#define FLUSH_INSTRUCTION_CACHE ppc64_flush_instruction_cache

#if 0
struct dynamic_buffer_s;

void ppc64_encode_program(struct program *p, struct dynamic_buffer_s *buf);
void ppc64_decode_program(struct program *p);

#define ENCODE_PROGRAM(P, BUF)	ppc64_encode_program(P, BUF)
#define DECODE_PROGRAM(P)	ppc64_decode_program(p)
#endif

#ifdef PIKE_CPU_REG_PREFIX
#define PPC_REGNAME(n) PIKE_CPU_REG_PREFIX #n
#else
#define PPC_REGNAME(n) #n
#endif

#define CALL_MACHINE_CODE(pc)						    \
  __asm__ __volatile__( "	mtctr %0\n"				    \
			"	mr "PPC_REGNAME(29)",%1\n"		    \
			"	bctr"					    \
			:						    \
			: "r" (pc), "r" (&Pike_interpreter)		    \
			: "ctr", "lr", "cc", "memory", "r29", "r0", "r2",   \
			  "r3", "r4", "r5", "r6", "r7", "r8", "r9",	    \
			  "r10", "r11", "r12")

#define OPCODE_INLINE_BRANCH
#define OPCODE_RETURN_JUMPADDR

#ifdef OPCODE_RETURN_JUMPADDR

/* Don't need an lvalue in this case. */
#define PROG_COUNTER ((PIKE_OPCODE_T *)__builtin_return_address(0))

#define JUMP_EPILOGUE_SIZE 2
#define JUMP_SET_TO_PC_AT_NEXT(PC) \
  ((PC) = PROG_COUNTER + JUMP_EPILOGUE_SIZE)

#else /* !OPCODE_RETURN_JUMPADDR */

#ifdef __linux
/* SVR4 ABI */
#define PROG_COUNTER (((PIKE_OPCODE_T **)__builtin_frame_address(1))[1])
#else
/* PowerOpen ABI */
#define PROG_COUNTER (((PIKE_OPCODE_T **)__builtin_frame_address(1))[2])
#endif

#define JUMP_EPILOGUE_SIZE 0

#endif /* !OPCODE_RETURN_JUMPADDR */


#ifdef PIKE_DEBUG
void ppc64_disassemble_code(void *addr, size_t bytes);
#define DISASSEMBLE_CODE(ADDR, BYTES)	ppc64_disassemble_code(ADDR, BYTES)
#endif
