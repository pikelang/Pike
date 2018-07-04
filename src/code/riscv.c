
#undef DISASSEMBLE_32BIT
#undef DISASSEMBLE_128BIT
#if __riscv_xlen == 32
#define DISASSEMBLE_32BIT
#elif __riscv_xlen == 128
#define DISASSEMBLE_128BIT
#endif


#define RV_SHIFT(v,n) ((n)<0? ((v)>>-(n)) : ((v)<<(n)))
#define RV_GET_BITS(v,hi,lo,offs) \
  RV_SHIFT((v)&(((1<<((hi)-(lo)+1))-1)<<(offs)),((lo)-(offs)))
#define RV_GET_BIT(v,n,offs) RV_GET_BITS(v,n,n,offs)
#define RV_GET_SIGNBIT(v,n,offs) (((v)&(1<<(offs)))? ~((1<<(n))-1): 0)

#define RV_I_IMM(v) ((INT32)(RV_GET_BITS(v,10,0,20)| \
			     RV_GET_SIGNBIT(v,11,31)))
#define RV_S_IMM(v) ((INT32)(RV_GET_BITS(v,10,5,25)|RV_GET_BITS(v,4,0,7)| \
			     RV_GET_SIGNBIT(v,11,31)))
#define RV_B_IMM(v) ((INT32)(RV_GET_BITS(v,10,5,25)|RV_GET_BITS(v,4,1,8)| \
			     RV_GET_BIT(v,7,11)|RV_GET_SIGNBIT(v,12,31)))
#define RV_U_IMM(v) RV_GET_BITS(v,31-12,12-12,12)
#define RV_J_IMM(v) ((INT32)(RV_GET_BITS(v,10,1,21)|RV_GET_BIT(v,11,20)|  \
			     RV_GET_BITS(v,19,12,12)|RV_GET_SIGNBIT(v,20,31)))
#define RV_CSR_IMM(v) RV_GET_BITS(v,4,0,15)
#define RV_FUNCT3(v) RV_GET_BITS(v,2,0,12)
#define RV_FUNCT7(v) RV_GET_BITS(v,6,0,25)
#define RV_FUNCT12(v) RV_GET_BITS(v,11,0,20)

#define RV_REG(v,offs) RV_GET_BITS(v,4,0,offs)
#define RV_REGD(v) RV_REG(v,7)
#define RV_REGS1(v) RV_REG(v,15)
#define RV_REGS2(v) RV_REG(v,20)
#define RV_CSR(v) RV_GET_BITS(v,11,0,20)

#define RV_CI_IMM(v) ((INT32)(RV_GET_BITS(v,4,0,2)|RV_GET_SIGNBIT(v,5,12)))
#define RV_CIW_IMM(v) (RV_GET_BITS(v,5,4,11)|RV_GET_BITS(v,9,6,7)|          \
		       RV_GET_BIT(v,2,6)|RV_GET_BIT(v,3,5))
#define RV_CB_IMM(v) ((INT32)(RV_GET_BITS(v,4,3,10)|RV_GET_BITS(v,7,6,5)|   \
			      RV_GET_BITS(v,2,1,3)|RV_GET_BIT(v,5,2)|       \
			      RV_GET_SIGNBIT(v,8,12)))
#define RV_CJ_IMM(v) ((INT32)(RV_GET_BIT(v,4,11)|RV_GET_BITS(v,9,8,9)|      \
			      RV_GET_BIT(v,10,8)|RV_GET_BIT(v,6,7)|         \
			      RV_GET_BIT(v,7,6)|RV_GET_BITS(v,3,1,3)|       \
			      RV_GET_BIT(v,5,2)|RV_GET_SIGNBIT(v,11,12)))
#define RV_CFUNCT3(v) RV_GET_BITS(v,2,0,13)

#define RV_REGC(v,offs) (RV_GET_BITS(v,2,0,offs)+8)
#define RV_REGCS2(v) RV_REG(v,2)
#define RV_REGCDS1(v) RV_REG(v,7)
#define RV_REGCD_(v) RV_REGC(v,2)
#define RV_REGCS1_(v) RV_REGC(v,7)
#define RV_REGCS2_(v) RV_REGC(v,2)


static const char *riscv_regname(unsigned n)
{
  static const char * const regnames[] = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6",
  };
  return regnames[n&0x3f];
}

static const char *riscv_fregname(unsigned n)
{
  static const char * const regnames[] = {
    "ft0", "ft1", "ft2", "ft3", "ft4", "ft5", "ft6", "ft7",
    "fs0", "fs1", "fa0", "fa1", "fa2", "fa3", "fa4", "fa5",
    "fa6", "fa7", "fs2", "fs3", "fs4", "fs5", "fs6", "fs7",
    "fs8", "fs9", "fs10", "fs11", "ft8", "ft9", "ft10", "ft11",
  };
  return regnames[n&0x3f];
}

void riscv_disassemble_code(void *addr, size_t bytes)
{
  static const char * const rvcq1_op[] = { "sub ", "xor ", "or  ", "and ",
					   "subw", "addw" };
  static const char * const load_op[] = {"lb ", "lh ", "lw ", "ld ",
					 "lbu", "lhu", "lwu" };
  static const char * const op_imm_op[] = { "addi", "*sl", "slti", "sltiu",
					    "xori", "*sr", "ori", "andi"};
  static const char * const store_op[] = {"sb", "sh", "sw", "sd"};
  static const char * const op_op[] = {"*add", "sll", "slt", "sltu",
				       "xor", "*srl", "or", "and"};
  static const char * const mul_op[] = {"mul", "mulh", "mulhsu", "mulhu",
					"div", "divu", "rem", "remu"};
  static const char * const branch_op[] = {"beq ", "bne ", NULL, NULL,
					   "blt ", "bge ", "bltu", "bgeu"};
  static const char * const system_op[] = {"rw","rs","rc"};
  const unsigned INT16 *parcel = addr;

  while (bytes >= 2) {
    unsigned INT32 instr = *parcel;
    if ((instr&3) != 3) {
      /* 16-bit format */
      fprintf(stderr, "%p  %04x          ", parcel, instr);
      switch (((instr&3)<<3) | RV_CFUNCT3(instr)) {
      case 0:
	if (!(instr & 0x1f30))
	  fprintf(stderr, "illegal\n");
	else
	  fprintf(stderr, "addi    %s,%s,%d\n",
		  riscv_regname(RV_REGCD_(instr)), riscv_regname(2),
		  (int)RV_CIW_IMM(instr));
	break;
      case 1:
#ifdef DISASSEMBLE_128BIT
	fprintf(stderr, "lq      %s,%d(%s)\n",
		riscv_regname(RV_REGCD_(instr)),
		(int)(RV_GET_BIT(instr,5,12)|RV_GET_BIT(instr,4,11)|
		      RV_GET_BIT(instr,8,10)|RV_GET_BITS(instr,7,6,5)),
		riscv_regname(RV_REGCS1_(instr)));
#else
	fprintf(stderr, "fld     %s,%d(%s)\n",
		riscv_fregname(RV_REGCD_(instr)),
		(int)(RV_GET_BITS(instr,5,3,10)|RV_GET_BITS(instr,7,6,5)),
		riscv_regname(RV_REGCS1_(instr)));
#endif
	break;
      case 2:
	fprintf(stderr, "lw      %s,%d(%s)\n",
		riscv_regname(RV_REGCD_(instr)),
		(int)(RV_GET_BITS(instr,5,3,10)|
		      RV_GET_BIT(instr,2,6)|RV_GET_BIT(instr,6,5)),
		riscv_regname(RV_REGCS1_(instr)));
	break;
      case 3:
#ifdef DISASSEMBLE_32BIT
	fprintf(stderr, "flw     %s,%d(%s)\n",
		riscv_fregname(RV_REGCD_(instr)),
		(int)(RV_GET_BITS(instr,5,3,10)|
		      RV_GET_BIT(instr,2,6)|RV_GET_BIT(instr,6,5)),
		riscv_regname(RV_REGCS1_(instr)));
#else
	fprintf(stderr, "ld      %s,%d(%s)\n",
		riscv_regname(RV_REGCD_(instr)),
		(int)(RV_GET_BITS(instr,5,3,10)|RV_GET_BITS(instr,7,6,5)),
		riscv_regname(RV_REGCS1_(instr)));
#endif
	break;
      case 4:
	  fprintf(stderr, "reserved\n");
	  break;
      case 5:
#ifdef DISASSEMBLE_128BIT
	fprintf(stderr, "sq      %s,%d(%s)\n",
		riscv_regname(RV_REGCS2_(instr)),
		(int)(RV_GET_BIT(instr,5,12)|RV_GET_BIT(instr,4,11)|
		      RV_GET_BIT(instr,8,10)|RV_GET_BITS(instr,7,6,5)),
		riscv_regname(RV_REGCS1_(instr)));
#else
	fprintf(stderr, "fsd     %s,%d(%s)\n",
		riscv_fregname(RV_REGCS2_(instr)),
		(int)(RV_GET_BITS(instr,5,3,10)|RV_GET_BITS(instr,7,6,5)),
		riscv_regname(RV_REGCS1_(instr)));
#endif
	break;
      case 6:
	fprintf(stderr, "sw      %s,%d(%s)\n",
		riscv_regname(RV_REGCS2_(instr)),
		(int)(RV_GET_BITS(instr,5,3,10)|
		      RV_GET_BIT(instr,2,6)|RV_GET_BIT(instr,6,5)),
		riscv_regname(RV_REGCS1_(instr)));
	break;
      case 7:
#ifdef DISASSEMBLE_32BIT
	fprintf(stderr, "fsw     %s,%d(%s)\n",
		riscv_fregname(RV_REGCS2_(instr)),
		(int)(RV_GET_BITS(instr,5,3,10)|
		      RV_GET_BIT(instr,2,6)|RV_GET_BIT(instr,6,5)),
		riscv_regname(RV_REGCS1_(instr)));
#else
	fprintf(stderr, "sd      %s,%d(%s)\n",
		riscv_regname(RV_REGCS2_(instr)),
		(int)(RV_GET_BITS(instr,5,3,10)|RV_GET_BITS(instr,7,6,5)),
		riscv_regname(RV_REGCS1_(instr)));
#endif
	break;
      case 8:
	fprintf(stderr, "addi    %s,%s,%d\n",
		riscv_regname(RV_REGCDS1(instr)),
		riscv_regname(RV_REGCDS1(instr)),
		(int)RV_CI_IMM(instr));
	break;
      case 9:
#ifdef DISASSEMBLE_32BIT
	fprintf(stderr, "jal     %s,%p\n",
		riscv_regname(1),
		((const char *)parcel)+RV_CJ_IMM(instr));
#else
	fprintf(stderr, "addiw   %s,%s,%d\n",
		riscv_regname(RV_REGCDS1(instr)),
		riscv_regname(RV_REGCDS1(instr)),
		(int)RV_CI_IMM(instr));
#endif
	break;
      case 10:
	fprintf(stderr, "addi    %s,%s,%d\n",
		riscv_regname(RV_REGCDS1(instr)),
		riscv_regname(0),
		(int)RV_CI_IMM(instr));
	break;
      case 11:
	if (((instr >> 7) & 31) == 2)
	  fprintf(stderr, "addi    %s,%s,%d\n",
		  riscv_regname(2), riscv_regname(2),
		  (int)(INT32)(RV_GET_BIT(instr,4,6)|RV_GET_BIT(instr,6,5)|
			       RV_GET_BITS(instr,8,7,3)|RV_GET_BIT(instr,5,2)|
			       RV_GET_SIGNBIT(instr,9,12)));
	else
	  fprintf(stderr, "lui     %s,0x%x\n",
		  riscv_regname(RV_REGCDS1(instr)),
		  (unsigned)(RV_CI_IMM(instr)&0xfffff));
	break;
      case 12:
	switch ((instr >> 10) & 3) {
	case 0:
#ifdef DISASSEMBLE_128BIT
	  fprintf(stderr, "srli    %s,%s,%u\n",
		  riscv_regname(RV_REGCDS1(instr)),
		  riscv_regname(RV_REGCDS1(instr)),
		  (unsigned)((instr & 0x107c)?
			     (((instr & 0x1000)? 96 : 0)+
			      RV_GET_BITS(instr,4,0,2)) : 64));
#else
	  fprintf(stderr, "srli    %s,%s,%u\n",
		  riscv_regname(RV_REGCDS1(instr)),
		  riscv_regname(RV_REGCDS1(instr)),
		  (unsigned)((RV_GET_BIT(instr,5,12)|RV_GET_BITS(instr,4,0,2))));
#endif
	  break;
	case 1:
#ifdef DISASSEMBLE_128BIT
	  fprintf(stderr, "srai    %s,%s,%u\n",
		  riscv_regname(RV_REGCDS1(instr)),
		  riscv_regname(RV_REGCDS1(instr)),
		  (unsigned)((instr & 0x107c)?
			     (((instr & 0x1000)? 96 : 0)+
			      RV_GET_BITS(instr,4,0,2)) : 64));
#else
	  fprintf(stderr, "srai    %s,%s,%u\n",
		  riscv_regname(RV_REGCDS1(instr)),
		  riscv_regname(RV_REGCDS1(instr)),
		  (unsigned)((RV_GET_BIT(instr,5,12)|RV_GET_BITS(instr,4,0,2))));
#endif
	  break;
	case 2:
	  fprintf(stderr, "andi    %s,%s,%d\n",
		  riscv_regname(RV_REGCS1_(instr)),
		  riscv_regname(RV_REGCS1_(instr)),
		  (int)RV_CI_IMM(instr));
	  break;
	  break;
	case 3:
	  if ((instr & 0x1000) && ((instr >> 5)&3) >= 2)
	    fprintf(stderr, "???\n");
	  else
	    fprintf(stderr, "%s    %s,%s,%s\n",
		    rvcq1_op[RV_GET_BIT(instr,2,12)|RV_GET_BITS(instr,1,0,5)],
		    riscv_regname(RV_REGCS1_(instr)),
		    riscv_regname(RV_REGCS1_(instr)),
		    riscv_regname(RV_REGCS2_(instr)));
	  break;
	}
	break;
      case 13:
	fprintf(stderr, "j       %s,%p\n",
		riscv_regname(1),
		((const char *)parcel)+RV_CJ_IMM(instr));
	break;
      case 14:
	fprintf(stderr, "beq     %s,%s,%p\n",
		riscv_regname(RV_REGCS1_(instr)),
		riscv_regname(0),
		((const char *)parcel)+RV_CB_IMM(instr));
	break;
      case 15:
	fprintf(stderr, "bne     %s,%s,%p\n",
		riscv_regname(RV_REGCS1_(instr)),
		riscv_regname(0),
		((const char *)parcel)+RV_CB_IMM(instr));
	break;
      case 16:
#ifdef DISASSEMBLE_128BIT
	if (!(instr & 0x107c))
	  fprintf(stderr, "slli    %s,%s,%u\n",
		  riscv_regname(RV_REGCDS1(instr)),
		  riscv_regname(RV_REGCDS1(instr)),
		  64u);
	else
#endif
	  fprintf(stderr, "slli    %s,%s,%u\n",
		  riscv_regname(RV_REGCDS1(instr)),
		  riscv_regname(RV_REGCDS1(instr)),
		  (unsigned)((RV_GET_BIT(instr,5,12)|RV_GET_BITS(instr,4,0,2))));
	break;
      case 17:
#ifdef DISASSEMBLE_128BIT
	fprintf(stderr, "lq      %s,%d(%s)\n",
		riscv_regname(RV_REGCDS1(instr)),
		(int)(RV_GET_BIT(instr,5,12)|
		      RV_GET_BIT(instr,4,6)|RV_GET_BITS(instr,9,6,2)),
		riscv_regname(2));
#else
	fprintf(stderr, "fld     %s,%d(%s)\n",
		riscv_fregname(RV_REGCDS1(instr)),
		(int)(RV_GET_BIT(instr,5,12)|
		      RV_GET_BITS(instr,4,3,5)|RV_GET_BITS(instr,8,6,2)),
		riscv_regname(2));
#endif
	break;
      case 18:
	fprintf(stderr, "lw      %s,%d(%s)\n",
		riscv_regname(RV_REGCDS1(instr)),
		(int)(RV_GET_BIT(instr,5,12)|
		      RV_GET_BITS(instr,4,2,4)|RV_GET_BITS(instr,7,6,2)),
		riscv_regname(2));
	break;
      case 19:
#ifdef DISASSEMBLE_32BIT
	fprintf(stderr, "flw     %s,%d(%s)\n",
		riscv_fregname(RV_REGCDS1(instr)),
		(int)(RV_GET_BIT(instr,5,12)|
		      RV_GET_BITS(instr,4,2,4)|RV_GET_BITS(instr,7,6,2)),
		riscv_regname(2));
#else
	fprintf(stderr, "ld      %s,%d(%s)\n",
		riscv_regname(RV_REGCDS1(instr)),
		(int)(RV_GET_BIT(instr,5,12)|
		      RV_GET_BITS(instr,4,3,5)|RV_GET_BITS(instr,8,6,2)),
		riscv_regname(2));
#endif
	break;
      case 20:
	if (RV_REGCS2(instr)) {
	  fprintf(stderr, "add     %s,%s,%s\n",
		  riscv_regname(RV_REGCDS1(instr)),
		  riscv_regname((instr & 0x1000)? RV_REGCDS1(instr) : 0),
		  riscv_regname(RV_REGCS2(instr)));
	} else if ((instr & 0x1000) && !RV_REGCDS1(instr)) {
	  fprintf(stderr, "ebreak\n");
	} else {
	  fprintf(stderr, "jalr    %s,%s,0\n",
		  riscv_regname((instr & 0x1000)? 1 : 0),
		  riscv_regname(RV_REGCDS1(instr)));
	}
	break;
      case 21:
#ifdef DISASSEMBLE_128BIT
	fprintf(stderr, "sq      %s,%d(%s)\n",
		riscv_regname(RV_REGCS2(instr)),
		(int)(RV_GET_BITS(instr,5,4,11)|RV_GET_BITS(instr,9,6,7)),
		riscv_regname(2));
#else
	fprintf(stderr, "fsd     %s,%d(%s)\n",
		riscv_fregname(RV_REGCS2(instr)),
		(int)(RV_GET_BITS(instr,5,3,10)|RV_GET_BITS(instr,8,6,7)),
		riscv_regname(2));
#endif
	break;
      case 22:
	fprintf(stderr, "sw      %s,%d(%s)\n",
		riscv_regname(RV_REGCS2(instr)),
		(int)(RV_GET_BITS(instr,5,2,9)|RV_GET_BITS(instr,7,6,7)),
		riscv_regname(2));
	break;
      case 23:
#ifdef DISASSEMBLE_32BIT
	fprintf(stderr, "fsw     %s,%d(%s)\n",
		riscv_fregname(RV_REGCS2(instr)),
		(int)(RV_GET_BITS(instr,5,2,9)|RV_GET_BITS(instr,7,6,7)),
		riscv_regname(2));
#else
	fprintf(stderr, "sd      %s,%d(%s)\n",
		riscv_regname(RV_REGCS2(instr)),
		(int)(RV_GET_BITS(instr,5,3,10)|RV_GET_BITS(instr,8,6,7)),
		riscv_regname(2));
#endif
	break;
      }
      bytes -= 2;
      parcel++;
    } else if((instr&0x1f) != 0x1f) {
      /* 32-bit format */
      if (bytes < 4) break;
      instr |= ((unsigned INT32)parcel[1])<<16;
      fprintf(stderr, "%p  %08x      ", parcel, instr);
      switch ((instr >> 2) & 0x1f) {
      case 0:
	if (RV_FUNCT3(instr) <= 6)
	  fprintf(stderr, "%s     %s,%d(%s)\n",
		  load_op[RV_FUNCT3(instr)],
		  riscv_regname(RV_REGD(instr)),
		  (int)RV_I_IMM(instr),
		  riscv_regname(RV_REGS1(instr)));
	else
	  fprintf(stderr, "???\n");
	break;
      case 3:
	switch(RV_FUNCT3(instr)) {
	case 0:
	  fprintf(stderr, "fence   %u,%u\n",
		  (unsigned)RV_GET_BITS(instr,3,0,24),
		  (unsigned)RV_GET_BITS(instr,3,0,20));
	  break;
	case 1:
	  fprintf(stderr, "fence.i\n");
	  break;
	default:
	  fprintf(stderr, "???\n");
	  break;
	}
	break;
      case 4:
      case 6:
	{
	  const char *op = op_imm_op[RV_FUNCT3(instr)];
	  if (*op == '*') {
	    fprintf(stderr, "%s%ci%c   %s,%s,%u\n", op+1,
		    ((instr & 0x40000000)? 'a':'l'),
		    ((instr&4)? 'w':' '),
		    riscv_regname(RV_REGD(instr)),
		    riscv_regname(RV_REGS1(instr)),
		    (unsigned)RV_GET_BITS(instr,5,0,20));
	  } else
	    fprintf(stderr, "%s%-*c%s,%s,%d\n", op,
		    (int)(8-strlen(op)), ((instr&4)? 'w':' '),
		    riscv_regname(RV_REGD(instr)),
		    riscv_regname(RV_REGS1(instr)),
		    (int)RV_I_IMM(instr));
	}
	break;
      case 5:
	fprintf(stderr, "auipc   %s,0x%x\n",
		riscv_regname(RV_REGD(instr)),
		(unsigned)RV_U_IMM(instr));
	break;
      case 8:
	if (RV_FUNCT3(instr) <= 3)
	  fprintf(stderr, "%s      %s,%d(%s)\n",
		  store_op[RV_FUNCT3(instr)],
		  riscv_regname(RV_REGS2(instr)),
		  (int)RV_S_IMM(instr),
		  riscv_regname(RV_REGS1(instr)));
	else
	  fprintf(stderr, "???\n");
	break;
	break;
      case 12:
      case 14:
	{
	  const char *op =
	    ((instr & 0x02000000)? mul_op:op_op)[RV_FUNCT3(instr)];
	  if (*op == '*') {
	    op++;
	    if (instr & 0x40000000)
	      op = (*op == 'a'? "sub":"sra");
	  }
	  fprintf(stderr, "%s%-*c%s,%s,%s\n", op,
		  (int)(8-strlen(op)), ((instr&4)? 'w':' '),
		  riscv_regname(RV_REGD(instr)),
		  riscv_regname(RV_REGS1(instr)),
		  riscv_regname(RV_REGS2(instr)));
	}
	break;
      case 13:
	fprintf(stderr, "lui     %s,0x%x\n",
		riscv_regname(RV_REGD(instr)),
		(unsigned)RV_U_IMM(instr));
	break;
      case 24:
	if (branch_op[RV_FUNCT3(instr)])
	  fprintf(stderr, "%s    %s,%s,%p\n",
		  branch_op[RV_FUNCT3(instr)],
		  riscv_regname(RV_REGS1(instr)),
		  riscv_regname(RV_REGS2(instr)),
		  ((const char *)parcel)+RV_B_IMM(instr));
	else
	  fprintf(stderr, "???\n");
	break;
      case 25:
	fprintf(stderr, "jalr    %s,%s,%d\n",
		riscv_regname(RV_REGD(instr)),
		riscv_regname(RV_REGS1(instr)),
		(int)RV_I_IMM(instr));
	break;
      case 27:
	fprintf(stderr, "jal     %s,%p\n",
		riscv_regname(RV_REGD(instr)),
		((const char *)parcel)+RV_J_IMM(instr));
	break;
      case 28:
	switch(RV_FUNCT3(instr)) {
	case 0:
	  switch(RV_FUNCT12(instr)) {
	  case 0:
	    fprintf(stderr, "ecall\n");
	    break;
	  case 1:
	    fprintf(stderr, "ebreak\n");
	    break;
	  default:
	    fprintf(stderr, "???\n");
	    break;
	  }
	  break;
	case 1:
	case 2:
	case 3:
	  fprintf(stderr, "csr%s   %s,%u,%s\n",
		  system_op[RV_FUNCT3(instr)-1],
		  riscv_regname(RV_REGD(instr)),
		  (unsigned)RV_CSR(instr),
		  riscv_regname(RV_REGS1(instr)));
	  break;
	case 5:
	case 6:
	case 7:
	  fprintf(stderr, "csr%si  %s,%u,%u\n",
		  system_op[RV_FUNCT3(instr)-5],
		  riscv_regname(RV_REGD(instr)),
		  (unsigned)RV_CSR(instr),
		  (unsigned)RV_CSR_IMM(instr));
	  break;
	default:
	  fprintf(stderr, "???\n");
	}
	break;
      case 1:  /* LOAD-FP */
      case 9:  /* STORE-FP */
      case 11: /* AMO */
      case 16: /* MADD */
      case 17: /* MSUB */
      case 18: /* NMSUB */
      case 19: /* NMADD */
      case 20: /* OP-FP */
      default:
	fprintf(stderr, "???\n");
      }
      bytes -= 4;
      parcel+=2;
    } else {
      /* > 32-bit format */
      unsigned parcel_count, i;
      if (!(instr & 0x20))
	parcel_count = 3; /* 48-bit */
      else if(!(instr & 0x40))
	parcel_count = 4; /* 64-bit */
      else if((instr & 0x7000) != 0x7000)
	parcel_count = 5 + ((instr >> 12) & 7); /* 80-176-bit */
      else
	parcel_count = 1; /* Reserved for >=192-bit, not defined... */
      if (bytes < parcel_count * 2)
	break;
      fprintf(stderr, "%p  ", parcel);
      for (i = parcel_count; i > 0; ) {
	fprintf(stderr, "%04x", parcel[--i]);
      }
      fprintf(stderr, "\n");
      bytes -= parcel_count * 2;
      parcel += parcel_count;
    }
  }
  if (bytes > 0) {
    const unsigned char *rest = (const unsigned char *)parcel;
    while(bytes > 0) {
      fprintf(stderr, "%p  %02x\n", rest, *rest);
      --bytes;
      rest++;
    }
  }
}
