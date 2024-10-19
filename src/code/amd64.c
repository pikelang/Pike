/*
 * Machine code generator for AMD64.
 */

#include "operators.h"
#include "constants.h"
#include "object.h"
#include "builtin_functions.h"
#include "bignum.h"
#include "pike_compiler.h"

/* These come from linux include files. */
#ifdef REG_RBX
#undef REG_RAX
#undef REG_RBX
#undef REG_RCX
#undef REG_RDX
#undef REG_RSP
#undef REG_RBP
#undef REG_RSI
#undef REG_RDI
#undef REG_R8
#undef REG_R9
#undef REG_R10
#undef REG_R11
#undef REG_R12
#undef REG_R13
#undef REG_R14
#undef REG_R15
#endif


#define REG_RAX MISSING_P_
#define REG_RBX MISSING_P_
#define REG_RCX MISSING_P_
#define REG_RDX MISSING_P_
#define REG_RSP MISSING_P_
#define REG_RBP MISSING_P_
#define REG_RSI MISSING_P_
#define REG_RDI MISSING_P_
#define REG_R8  MISSING_P_
#define REG_R9  MISSING_P_
#define REG_R10 MISSING_P_
#define REG_R11 MISSING_P_
#define REG_R12 MISSING_P_
#define REG_R13 MISSING_P_
#define REG_R14 MISSING_P_
#define REG_R15 MISSING_P_

struct soff { int off; int type; int value; };

static struct soff SVAL(int i)
{
  struct soff res;
  res.off = res.type = i*sizeof(struct svalue);
  res.value = res.type + 8;
  return res;
}

/* Register encodings */
enum amd64_reg {P_REG_RAX = 0, P_REG_RBX = 3, P_REG_RCX = 1, P_REG_RDX = 2,
		P_REG_RSP = 4, P_REG_RBP = 5, P_REG_RSI = 6, P_REG_RDI = 7,
		P_REG_R8 = 8, P_REG_R9 = 9, P_REG_R10 = 10, P_REG_R11 = 11,
		P_REG_R12 = 12, P_REG_R13 = 13, P_REG_R14 = 14, P_REG_R15 = 15,
		P_REG_INVALID = -1,

                P_REG_XMM0 = 0, P_REG_XMM1 = 1, P_REG_XMM2 = 2, P_REG_XMM3 = 3,
                P_REG_XMM4 = 4, P_REG_XMM5 = 5, P_REG_XMM6 = 6, P_REG_XMM7 = 7,
                P_REG_XMM8 = 8, P_REG_XMM9 = 9, P_REG_XMM10=10, P_REG_XMM11=11,
                P_REG_XMM12=12, P_REG_XMM13=13, P_REG_XMM14=14, P_REG_XMM15=15,
               };

#define PIKE_MARK_SP_REG	 P_REG_R12
#define PIKE_SP_REG		     P_REG_R13
#define PIKE_FP_REG		     P_REG_R14
#define Pike_interpreter_reg P_REG_R15

#ifdef __NT__
/* From http://software.intel.com/en-us/articles/introduction-to-x64-assembly/
 *
 * Note: Space for the arguments needs to be allocated on the stack as well.
 */
#define ARG1_REG	P_REG_RCX
#define ARG2_REG	P_REG_RDX
#define ARG3_REG	P_REG_R8
#define ARG4_REG	P_REG_R9
#else
/* From SysV ABI for AMD64 draft 0.99.5. */
#define ARG1_REG	P_REG_RDI
#define ARG2_REG	P_REG_RSI
#define ARG3_REG	P_REG_RDX
#define ARG4_REG	P_REG_RCX
#define ARG5_REG	P_REG_R8
#define ARG6_REG	P_REG_R9
#endif

static enum amd64_reg sp_reg = -1, fp_reg = -1, mark_sp_reg = -1;
static int dirty_regs = 0, ret_for_func = 0;
ptrdiff_t amd64_prev_stored_pc = -1; /* PROG_PC at the last point Pike_fp->pc was updated. */
static int branch_check_threads_update_etc = 0;
static int store_pc = 0;

#define MAX_LABEL_USES 6
struct label {
  int n_label_uses;
  ptrdiff_t addr;
  ptrdiff_t offset[MAX_LABEL_USES];
};

static void label( struct label *l )
{
  int i;
  if (l->addr >= 0) Pike_fatal("Label reused.\n");
  for( i=0; i<l->n_label_uses; i++ )
  {
    int dist = PIKE_PC - (l->offset[i] + 1);
    if( dist > 0x7f || dist < -0x80 )
      Pike_fatal("Branch %d too far\n", dist);
    Pike_compiler->new_program->program[l->offset[i]] = dist;
  }
  l->n_label_uses = 0;
  l->addr = PIKE_PC;
}

static void ib( char x )
{
  add_to_program( x );
}

static void iw( short x )
{
  add_to_program( x );
  add_to_program( x>>8 );
}

static void id( int x )
{
  add_to_program( (x)&0xff );
  add_to_program( (x>>8)&0xff );
  add_to_program( (x>>16)&0xff );
  add_to_program( (x>>24)&0xff );
}

/* x86 opcodes  */
#define opcode(X) ib(X)


static void modrm( int mod, int r, int m )
{
  if (r < 0) Pike_fatal("Using an invalid register(1)!\n");
  if (m < 0) Pike_fatal("Using an invalid register(2)!\n");
  ib( ((mod<<6) | ((r&0x7)<<3) | (m&0x7)) );
}

static void sib( int scale, int index, enum amd64_reg base )
{
  ib( (scale<<6) | ((index&0x7)<<3) | (base&0x7) );
}


static void modrm_sib( int mod, int r, int m )
{
  modrm( mod, r, m );
  if( (m&7) == P_REG_RSP)
    sib(0, P_REG_RSP, P_REG_RSP );
}

static void rex( int w, enum amd64_reg r, int x, enum amd64_reg b )
{
  unsigned char res = 1<<6;
  /* bit  7, 5-4 == 0 */
  if( w )        res |= 1<<3;
  if( r > 0x7 )  res |= 1<<2;
  if( x       )  res |= 1<<1;
  if( b > 0x7 )  res |= 1<<0;
  if( res != (1<<6) )
    add_to_program( res );
}


static void offset_modrm_sib( int offset, int r, int m )
{
  /* OFFSET */
  if( offset < -128  || offset > 127 )
  {
    modrm_sib( 2, r, m );
    id( offset );
  }
  else if( offset || (m&7) == P_REG_RSP || (m&7) == P_REG_RBP )
  {
    modrm_sib( 1, r, m );
    ib( offset );
  }
  else
  {
    modrm_sib( 0, r, m );
  }
}

static void ret()
{
  opcode(0xc3);
}

static void push(enum amd64_reg reg )
{
  if (reg < 0) Pike_fatal("Pushing invalid register.\n");
  if (reg & 0x08) add_to_program(0x41);
  add_to_program(0x50 + (reg & 0x07));
}

static void pop(enum amd64_reg reg )
{
  if (reg < 0) Pike_fatal("Popping invalid register.\n");
  if (reg & 0x08) add_to_program(0x41);
  add_to_program(0x58 + (reg & 0x07));
}

static void mov_reg_reg(enum amd64_reg from_reg, enum amd64_reg to_reg )
{
  rex( 1, from_reg, 0, to_reg );
  opcode( 0x89 );
  modrm( 3, from_reg, to_reg );
}

static void mov_reg32_reg(enum amd64_reg from_reg, enum amd64_reg to_reg )
{
  rex( 0, from_reg, 0, to_reg );
  opcode( 0x89 );
  modrm( 3, from_reg, to_reg );
}

#define PUSH_INT(X) ins_int((INT32)(X), add_to_program)
static void low_mov_mem_reg(enum amd64_reg from_reg, ptrdiff_t offset,
                            enum amd64_reg to_reg)
{
  opcode( 0x8b );
  offset_modrm_sib(offset, to_reg, from_reg );
}

static void mov_mem_reg( enum amd64_reg from_reg, ptrdiff_t offset, enum amd64_reg to_reg )
{
  rex( 1, to_reg, 0, from_reg );
  low_mov_mem_reg( from_reg, offset, to_reg );
}

static void xor_reg_reg( enum amd64_reg reg1,  enum amd64_reg reg2 )
{
  rex(1,reg1,0,reg2);
  opcode( 0x31 );
  modrm(3,reg1,reg2);
}

static void and_reg_reg( enum amd64_reg reg1,  enum amd64_reg reg2 )
{
  rex(1,reg1,0,reg2);
  opcode( 0x23 );
  modrm(3,reg1,reg2);
}

static void or_reg_reg( enum amd64_reg reg1,  enum amd64_reg reg2 )
{
  rex(1,reg1,0,reg2);
  opcode( 0x09 );
  modrm(3,reg1,reg2);
}

static void or_reg_imm( enum amd64_reg reg, INT_TYPE imm32 )
{
  rex( 1, 0, 0, reg );
  if( imm32 < -0x80 || imm32 > 0x7f )
  {
    if( reg == P_REG_RAX )
    {
      opcode( 0xd ); /* OR rax,imm32 */
      id( imm32 );
    }
    else
    {
      opcode( 0x81 ); /* OR REG,imm32 */
      modrm( 3, 1, reg);
      id( imm32 );
    }
  }
  else
  {
    add_to_program(0x83); /* OR REG,imm8 */
    modrm( 3, 1, reg );
    ib( imm32 );
  }
}

static void and_reg_imm( enum amd64_reg reg, INT_TYPE imm32 )
{
  rex( 1, 0, 0, reg );

  if( imm32 < -0x80 || imm32 > 0x7f )
  {
    if( reg == P_REG_RAX )
    {
      opcode( 0x25 ); /* AND rax,imm32 */
      id( imm32 );
    }
    else
    {
      opcode( 0x81 ); /* AND REG,imm32 */
      modrm( 3,4, reg);
      id( imm32 );
    }
  }
  else
  {
    add_to_program(0x83); /* AND REG,imm8 */
    modrm( 3, 4, reg );
    ib( imm32 );
  }
}

static void and_reg32_imm( enum amd64_reg reg, int imm32 )
{
  rex( 0, 0, 0, reg );

  if( imm32 < -0x80 || imm32 > 0x7f )
  {
    if( reg == P_REG_RAX )
    {
      opcode( 0x25 ); /* AND rax,imm32 */
      id( imm32 );
    }
    else
    {
      opcode( 0x81 ); /* AND REG,imm32 */
      modrm( 3,4, reg);
      id( imm32 );
    }
  }
  else
  {
    add_to_program(0x83); /* AND REG,imm8 */
    modrm( 3, 4, reg );
    ib( imm32 );
  }
}

static void mov_mem32_reg( enum amd64_reg from_reg, ptrdiff_t offset, enum amd64_reg to_reg )
{
  rex( 0, to_reg, 0, from_reg );
  low_mov_mem_reg( from_reg, offset, to_reg );
}

static void mov_mem16_reg( enum amd64_reg from_reg, ptrdiff_t offset, enum amd64_reg to_reg )
{
#if 0
  mov_mem32_reg( from_reg, offset, to_reg );
  and_reg_imm(to_reg, 0xffff);
#else
  rex( 1,to_reg,0,from_reg );
  /* movzx r/m16 -> r32. This automatically zero-extends the upper 32-bit */
  opcode( 0xf );
  opcode( 0xb7 );
  offset_modrm_sib(offset, to_reg, from_reg );
#endif
}

static void mov_mem8_reg( enum amd64_reg from_reg, ptrdiff_t offset, enum amd64_reg to_reg )
{
#if 0
  mov_mem32_reg( from_reg, offset, to_reg );
  and_reg_imm(to_reg, 0xff);
#else
  rex( 0,to_reg,0,from_reg );
 /* movzx r/m8 -> r32. This automatically zero-extends the upper 32-bit */
  opcode( 0xf );
  opcode( 0xb6 );
  offset_modrm_sib(offset, to_reg, from_reg );
#endif
}

static void add_reg_imm( enum amd64_reg src, int imm32);

static void shl_reg_imm( enum amd64_reg from_reg, int shift )
{
  rex( 1, from_reg, 0, 0 );
  if( shift == 1 )
  {
    opcode( 0xd1 );     /* SAL */
    modrm( 3, 4, from_reg );
  }
  else
  {
    opcode( 0xc1 );
    modrm( 3, 4, from_reg );
    ib( shift );
  }
}

static void shl_reg_reg( enum amd64_reg reg, enum amd64_reg sreg)
{
  if( sreg != P_REG_RCX )
    Pike_fatal("Not supported\n");

  rex( 1, 0, 0, reg );
  opcode( 0xd3 );
  modrm( 3, 4, reg );
}

static void shl_reg32_reg( enum amd64_reg reg, enum amd64_reg sreg)
{
  if( sreg != P_REG_RCX )
    Pike_fatal("Not supported\n");
  rex( 0, 0, 0, reg );
  opcode( 0xd3 );
  modrm( 3, 4, reg );
}

static void shr_mem_reg( enum amd64_reg reg, int off, enum amd64_reg sreg)
{
  if( sreg != P_REG_RCX )
    Pike_fatal("Not supported\n");

  rex( 1, 0, 0, reg );
  opcode( 0xd3 ); /* SAR r/m64,CL */
  offset_modrm_sib(off, 7, reg );
/*   modrm( 3, 7, reg ); */
}


static void shr_reg_reg( enum amd64_reg reg, enum amd64_reg reg_rcx )
{
  if( reg_rcx != P_REG_RCX )
    Pike_fatal("Not supported\n");

  rex( 1, 0, 0, reg );
  opcode( 0xd3 ); /* SAR r/m64,CL */
  modrm( 3, 7, reg );
}


static void shr_reg_imm( enum amd64_reg from_reg, int shift )
{
  rex( 1, from_reg, 0, 0 );
  if( shift == 1 )
  {
    opcode( 0xd1 );     /* SAR */
    modrm( 3, 7, from_reg );
  }
  else
  {
    opcode( 0xc1 );
    modrm( 3, 7, from_reg );
    ib( shift );
  }
}

static void clear_reg( enum amd64_reg reg )
{
  xor_reg_reg( reg, reg );
}

#if 0
static void neg_reg( enum amd64_reg reg )
{
  rex(1,0,0,reg);
  opcode(0xf7);
  modrm(3,3,reg);
}
#endif

static void mov_imm_reg( long imm, enum amd64_reg reg )
{
  if( !imm )
  {
    clear_reg(reg);
    return;
  }
  if( (imm > 0x7fffffffLL) || (imm < -0x80000000LL) )
  {
    rex(1,0,0,reg);
    opcode(0xb8 | (reg&0x7)); /* mov imm64 -> reg64 */
    id( (imm & 0xffffffffLL) );
    id( ((imm >> 32)&0xffffffffLL) );
  }
  else if( imm > 0 )
  {
    rex(0,0,0,reg);
    opcode( 0xc7 ); /* mov imm32 -> reg/m 32, SE*/
    modrm( 3,0,reg );
    id( (int)imm  );
  }
  else
  {
    rex(1,0,0,reg);
    opcode( 0xc7 ); /* mov imm32 -> reg/m 64, SE*/
    modrm( 3,0,reg );
    id( (int)imm  );
  }
}

/* static void mov_ptr_reg( void *x, enum amd64_reg reg ) */
/* { */
/*     mov_imm_reg( (long)x, reg ); */
/* } */

static void mov_mem128_reg( enum amd64_reg from_reg, int offset, enum amd64_reg to_reg )
{
  if( from_reg > 7 )
    Pike_fatal("Not supported\n");
  rex(0,to_reg,0,from_reg);
  opcode( 0x66 );
  opcode( 0x0f );
  opcode( 0x6f ); /* MOVDQA xmm,m128 */
  offset_modrm_sib( offset, to_reg, from_reg );
}

static void mov_reg_mem128( enum amd64_reg from_reg, enum amd64_reg to_reg, int offset )
{
  if( from_reg > 7 )
    Pike_fatal("Not supported\n");
  rex(0,from_reg,0,to_reg);
  opcode( 0x66 );
  opcode( 0x0f );
  opcode( 0x7f ); /* MOVDQA m128,xmm */
  offset_modrm_sib( offset, from_reg, to_reg );
}

static void low_set_if_cond(unsigned char subop, enum amd64_reg reg)
{
  rex( 0, 0, 0, reg );
  opcode( 0xf );
  opcode( subop );
  modrm( 3, 0, reg );
}

static void set_if_lt(enum amd64_reg reg)
{
  low_set_if_cond( 0x9c, reg );
}

static void set_if_lte(enum amd64_reg reg)
{
  low_set_if_cond( 0x9e, reg );
}

static void set_if_gt(enum amd64_reg reg)
{
  low_set_if_cond( 0x9f, reg );
}

static void set_if_gte(enum amd64_reg reg)
{
  low_set_if_cond( 0x9d, reg );
}

static void set_if_eq(enum amd64_reg reg)
{
  low_set_if_cond( 0x94, reg );
}

static void set_if_neq(enum amd64_reg reg)
{
  low_set_if_cond( 0x95, reg );
}



#if 0
static void low_mov_if_cond_reg(unsigned char subop,
				enum amd64_reg to_reg,
				enum amd64_reg from_reg )
{
  rex(0, from_reg, 0, to_reg );
  opcode( 0xf );
  opcode( subop );
  modrm_sib( 3, from_reg, to_reg );
}

static void mov_if_lte_reg( enum amd64_reg from_reg, enum amd64_reg to_reg )
{
  low_mov_if_cond_reg( 0x4e, from_reg, to_reg);
}

static void mov_if_gte_reg( enum amd64_reg from_reg, enum amd64_reg to_reg )
{
  low_mov_if_cond_reg( 0x4d, from_reg, to_reg );
}

static void mov_if_lt_reg( enum amd64_reg from_reg, enum amd64_reg to_reg )
{
  low_mov_if_cond_reg( 0x4c, from_reg, to_reg );
}
static void mov_if_gt_reg( enum amd64_reg from_reg, enum amd64_reg to_reg )
{
  low_mov_if_cond_reg( 0x4f, from_reg, to_reg );
}

static void mov_if_neq_reg( enum amd64_reg from_reg, enum amd64_reg to_reg )
{
  low_mov_if_cond_reg( 0x45, from_reg, to_reg );
}

static void mov_if_eq_reg( enum amd64_reg from_reg, enum amd64_reg to_reg )
{
  low_mov_if_cond_reg( 0x44, from_reg, to_reg );
}
#endif

static void low_mov_reg_mem(enum amd64_reg from_reg, enum amd64_reg to_reg, ptrdiff_t offset )
{
  opcode( 0x89 );
  offset_modrm_sib( offset, from_reg, to_reg );
}

static void mov_reg_mem( enum amd64_reg from_reg, enum amd64_reg to_reg, ptrdiff_t offset )
{
  rex(1, from_reg, 0, to_reg );
  low_mov_reg_mem( from_reg, to_reg, offset );
}

static void mov_reg_mem32( enum amd64_reg from_reg, enum amd64_reg to_reg, ptrdiff_t offset )
{
  rex(0, from_reg, 0, to_reg );
  low_mov_reg_mem( from_reg, to_reg, offset );
}

#if 0
static void mov_reg_mem16( enum amd64_reg from_reg, enum amd64_reg to_reg, ptrdiff_t offset )
{
  opcode( 0x66 );
  rex(0, from_reg, 0, to_reg );
  low_mov_reg_mem( from_reg, to_reg, offset );
}
#endif

static void PIKE_UNUSED_ATTRIBUTE mov_imm_mem16( short imm, enum amd64_reg to_reg,
						 ptrdiff_t offset )
{
    opcode( 0x66 ); /* switch 32/16 */

    /* basically copy of mov_imm_mem32 */
    rex( 0, 0, 0, to_reg );
    opcode( 0xc7 ); /* now: mov imm16 -> r/m16 (sign extend)*/

    /* This does not work for rg&7 == 4 or 5. */
    offset_modrm_sib( offset, 0, to_reg );
    iw( imm );
}



static void mov_imm_mem( long imm, enum amd64_reg to_reg, ptrdiff_t offset )
{
  if( imm >= -0x80000000LL && imm <= 0x7fffffffLL )
  {
    rex( 1, 0, 0, to_reg );
    opcode( 0xc7 ); /* mov imm32 -> r/m64 (sign extend)*/
    offset_modrm_sib( offset, 0, to_reg );
    id( imm );
  }
  else
  {
    if( to_reg == P_REG_RAX )
      Pike_fatal( "Clobbered TMP P_REG_RAX reg\n");
    mov_imm_reg( imm, P_REG_RAX );
    mov_reg_mem( P_REG_RAX, to_reg, offset );
  }
}

static void mov_imm_mem32( int imm, enum amd64_reg to_reg, ptrdiff_t offset )
{
    rex( 0, 0, 0, to_reg );
    opcode( 0xc7 ); /* mov imm32 -> r/m32 (sign extend)*/
    /* This does not work for rg&7 == 4 or 5. */
    offset_modrm_sib( offset, 0, to_reg );
    id( imm );
}

static void sub_reg_imm( enum amd64_reg reg, int imm32 );

static void neg_mem( enum amd64_reg reg, int off )
{
  rex( 1, 0, 0, reg );
  opcode(0xf7);
  offset_modrm_sib( off, 3, reg );
}

static void not_mem( enum amd64_reg reg, int off )
{
  rex( 1, 0, 0, reg );
  opcode(0xf7);
  offset_modrm_sib( off, 2, reg );
}

static void add_reg_imm( enum amd64_reg reg, int imm32 )
{
  if( !imm32 ) return;
  if( imm32 < 0 )
  {
    /* This makes the disassembly easier to read. */
    sub_reg_imm( reg, -imm32 );
    return;
  }

  rex( 1, 0, 0, reg );

  if( imm32 < -0x80 || imm32 > 0x7f )
  {
    if( reg == P_REG_RAX )
    {
      opcode( 0x05 ); /* ADD rax,imm32 */
      id( imm32 );
    }
    else
    {
      opcode( 0x81 ); /* ADD REG,imm32 */
      modrm( 3, 0, reg);
      id( imm32 );
    }
  }
  else
  {
    add_to_program(0x83); /* ADD REG,imm8 */
    modrm( 3, 0, reg );
    ib( imm32 );
  }
}


static void low_add_mem_imm(int w, enum amd64_reg reg, int offset, int imm32 )
{
  int r2 = imm32 == -1 ? 1 : 0;
  int large = 0;
  if( !imm32 ) return;
  rex( w, 0, 0, reg );

  if( r2 ) imm32 = -imm32;

  if( imm32 == 1  )
    opcode( 0xff ); /* INCL(DECL) r/m32 */
  else if( imm32 >= -128 && imm32 < 128 )
    opcode( 0x83 ); /* ADD imm8,r/m32 */
  else
  {
    opcode( 0x81 ); /* ADD imm32,r/m32 */
    large = 1;
  }
  offset_modrm_sib( offset, r2, reg );
  if( imm32 != 1  )
  {
      if( large )
          id( imm32 );
      else
          ib( imm32 );
  }
}

static void add_mem32_imm( enum amd64_reg reg, int offset, int imm32 )
{
  low_add_mem_imm( 0, reg, offset, imm32 );
}

static void add_mem_imm( enum amd64_reg reg, int offset, int imm32 )
{
  low_add_mem_imm( 1, reg, offset, imm32 );
}

#ifndef USE_VALGRIND
static void add_mem8_imm( enum amd64_reg reg, int offset, int imm32 )
{
  int r2 = imm32 == -1 ? 1 : 0;
  if( !imm32 ) return;
  rex( 0, 0, 0, reg );

  if( imm32 == 1 || imm32 == -1 )
    opcode( 0xfe ); /* INCL r/m8 */
  else if( imm32 >= -128 && imm32 < 128 )
    opcode( 0x80 ); /* ADD imm8,r/m32 */
  else
    Pike_fatal("Not sensible");

  offset_modrm_sib( offset, r2, reg );
  if( imm32 != 1 && !r2 )
  {
    ib( imm32 );
  }
}
#endif

static void sub_reg_imm( enum amd64_reg reg, int imm32 )
{
  if( !imm32 ) return;

  if( imm32 < 0 )
    /* This makes the disassembly easier to read. */
    return add_reg_imm( reg, -imm32 );

  rex( 1, 0, 0, reg );

  if( imm32 < -0x80 || imm32 > 0x7f )
  {
    if( reg == P_REG_RAX )
    {
      opcode( 0x2d ); /* SUB rax,imm32 */
      id( imm32 );
    }
    else
    {
      opcode( 0x81 ); /* SUB REG,imm32 */
      modrm( 3, 5, reg);
      id( imm32 );
    }
  }
  else
  {
    opcode(0x83); /* SUB REG,imm8 */
    modrm( 3, 5, reg );
    ib( imm32 );
  }
}

static void test_reg_reg( enum amd64_reg reg1, enum amd64_reg reg2 )
{
  rex(1,reg1,0,reg2);
  opcode(0x85);
  modrm(3, reg1, reg2 );
}

static void test_reg32_reg( enum amd64_reg reg1, enum amd64_reg reg2 )
{
  rex(0,reg1,0,reg2);
  opcode(0x85);
  modrm(3, reg1, reg2 );
}

static void test_reg( enum amd64_reg reg1 )
{
  test_reg_reg( reg1, reg1 );
}

static void test_reg32( enum amd64_reg reg1 )
{
  test_reg32_reg( reg1, reg1 );
}

static void cmp_reg_imm( enum amd64_reg reg, int imm32 )
{
  if(!imm32)
  {
   test_reg( reg );
   return;
  }
  rex(1, 0, 0, reg);
  if( imm32 > 0x7f || imm32 < -0x80 )
  {
    if( reg == P_REG_RAX )
    {
      opcode( 0x3d );
      id( imm32 );
    }
    else
    {
      opcode( 0x81 );
      modrm(3,7,reg);
      id( imm32 );
    }
  }
  else
  {
    opcode( 0x83 );
    modrm( 3,7,reg);
    ib( imm32 );
  }
}

static void cmp_reg32_imm( enum amd64_reg reg, int imm32 )
{
  if(!imm32)
  {
   test_reg32( reg );
   return;
  }
  rex(0, 0, 0, reg);
  if( imm32 > 0x7f || imm32 < -0x80 )
  {
    if( reg == P_REG_RAX )
    {
      opcode( 0x3d );
      id( imm32 );
    }
    else
    {
      opcode( 0x81 );
      modrm(3,7,reg);
      id( imm32 );
    }
  }
  else
  {
    opcode( 0x83 );
    modrm( 3,7,reg);
    ib( imm32 );
  }
}

static void cmp_reg_reg( enum amd64_reg reg1, enum amd64_reg reg2 )
{
  rex(1, reg2, 0, reg1);
  opcode( 0x39 );
  modrm( 3, reg2, reg1 );
}

static int jmp_rel_imm32( int addr )
{
  int rel = addr - (PIKE_PC + 5); // counts from the next instruction
  int res;
  opcode( 0xe9 );
  res = PIKE_PC;
  id( rel );
  return res;
}

static void jmp_rel_imm( int addr )
{
  int rel = addr - (PIKE_PC + 2); // counts from the next instruction
  if(rel >= -0x80 && rel <= 0x7f )
  {
      opcode( 0xeb );
      ib( rel );
      return;
  }
  jmp_rel_imm32( addr );
}

static void call_rel_imm32( int addr )
{
  int rel = addr - (PIKE_PC + 5); // counts from the next instruction
  opcode( 0xe8 );
  id( rel );
  sp_reg = -1;
  return;
}


static void jmp_reg( enum amd64_reg reg )
{
  rex(0,reg,0,0);
  opcode( 0xff );
  modrm( 3, 4, reg );
}

static void call_reg( enum amd64_reg reg )
{
  rex(0,reg,0,0);
  opcode( 0xff );
  modrm( 3, 2, reg );
}

static void call_imm( void *ptr )
{
  size_t addr = (size_t)ptr;
  if( (addr & ~0x7fffffffLL) && !(addr & ~0x3fffffff8LL) )
  {
    mov_imm_reg( addr>>3, P_REG_RAX);
    shl_reg_imm( P_REG_RAX, 3 );
  }
  else
  {
    mov_imm_reg(addr, P_REG_RAX );
  }
  call_reg( P_REG_RAX );
}

/* reg += reg2 */
static void add_reg_reg( enum amd64_reg reg, enum amd64_reg reg2 )
{
  rex(1,reg2,0,reg);
  opcode( 0x1 );
  modrm( 3, reg2, reg );
}

/* reg *= reg2   res in rdx:rax */
static void mul_rax_reg_reg128( enum amd64_reg reg )
{
  /* 64b * 64b -> 128b
     This would actually not need an overflow check.
     instead, push high if not zero, push low, genreate gmp.
     but... Well.
   */
  rex(1,0,0,reg); /* imul r/m64 */
  opcode( 0xf7 );
  modrm( 3,5,reg );
}


/* reg *= reg2 */
static void mul_reg_reg( enum amd64_reg reg, enum amd64_reg reg2 )
{
  rex(1,reg,0,reg2);
  opcode( 0xf );
  opcode( 0xaf );
  modrm( 3, reg, reg2 );
}

/* reg /= reg2 */
static void div_reg_reg( enum amd64_reg reg, enum amd64_reg reg2 )
{
  if( reg != P_REG_RAX )  Pike_error("Not supported, reg1 must be RAX\n");
  if( reg2 == P_REG_RDX ) Pike_error("Clobbers RAX+RDX\n");
  rex(1,0,0,0);
  opcode(0x99);
  /*
   *cqo:
   *sign extend 64bit rax -> 128bit in rdx:rax
  */

  rex(1,0,0,reg2);
  opcode( 0xf7 );
  modrm( 3, 7, reg2 );
  /* idiv, ax = dx:ax / reg2
     dx = dx:ax % reg2
  */
}

/* reg += reg2 */
static void add_reg32_reg32( enum amd64_reg reg, enum amd64_reg reg2 )
{
  rex(0,reg2,0,reg);
  opcode( 0x1 );
  modrm( 3, reg2, reg );
}

/* reg -= reg2 */
static void sub_reg_reg( enum amd64_reg reg, enum amd64_reg reg2 )
{
  rex(1,reg2,0,reg);
  opcode( 0x29 );
  modrm( 3, reg2, reg );
}

/* dst += *(src+off) */
static void add_reg_mem( enum amd64_reg dst, enum amd64_reg src, long off )
{
  rex(1,dst,0,src);
  opcode( 0x3 );
  offset_modrm_sib( off, dst, src );
}


/* dst -= *(src+off) */
static void sub_reg_mem( enum amd64_reg dst, enum amd64_reg src, long off )
{
  rex(1,dst,0,src);
  opcode( 0x2b );
  offset_modrm_sib( off, dst, src );
}


/* dst = src + imm32  (LEA, does not set flags!) */
static void add_reg_imm_reg( enum amd64_reg src, long imm32, enum amd64_reg dst )
{
  if( imm32 > 0x7fffffffLL ||
      imm32 <-0x80000000LL)
    Pike_fatal("LEA [reg+imm] > 32bit Not supported\n");

  if( src == dst )
  {
    if( !imm32 ) return;
    add_reg_imm( src, imm32 );
  }
  else
  {
    if( !imm32 )
    {
      mov_reg_reg( src, dst );
      return;
    }
    rex(1,dst,0,src);
    opcode( 0x8d ); /* LEA r64,m */
    offset_modrm_sib( imm32, dst, src );
  }
}

/* load code adress + imm to reg, always 32bit offset */
static void mov_rip_imm_reg( int imm, enum amd64_reg reg )
{
  imm -= 7; /* The size of this instruction. */

  rex( 1, reg, 0, 0 );
  opcode( 0x8d ); /* LEA */
  modrm( 0, reg, 5 );
  id( imm );
}

static void add_imm_mem( int imm32, enum amd64_reg reg, int offset )
{
  int r2 = (imm32 == -1) ? 1 : 0;
  int large = 0;

  /* OPCODE */
  rex( 1, 0, 0, reg );
  if( imm32 == 1 || imm32 == -1 )
    opcode( 0xff ); /* INCL(decl) r/m32 */
  else if( -128 <= imm32 && 128 > imm32  )
    opcode( 0x83 ); /* ADD imm8,r/m32 */
  else
  {
    opcode( 0x81 ); /* ADD imm32,r/m32 */
    large = 1;
  }
  offset_modrm_sib( offset, r2, reg );

  /* VALUE */
  if( imm32 != 1 && !r2 )
  {
    if( large )
      id( imm32 );
    else
      ib( imm32 );
  }
}


static void jump_rel8( struct label *res, unsigned char op )
{
  opcode( op );

  if (res->addr >= 0) {
    ib(res->addr - (PIKE_PC+1));
    return;
  }

  if( res->n_label_uses >= MAX_LABEL_USES )
    Pike_fatal( "Label used too many times\n" );
  res->offset[res->n_label_uses] = PIKE_PC;
  res->n_label_uses++;

  ib(0);
}

static int jnz_imm_rel32( int rel )
{
  int res;
  opcode( 0xf );
  opcode( 0x85 );
  res = PIKE_PC;
  id( rel );
  return res;
}

static int jz_imm_rel32( int rel )
{
  int res;
  opcode( 0xf );
  opcode( 0x84 );
  res = PIKE_PC;
  id( rel );
  return res;
}

#define      jne(X) jnz(X)
#define      je(X)  jz(X)
static void jmp( struct label *l ) { return jump_rel8( l, 0xeb ); }
static void jo( struct label *l )  { return jump_rel8( l, 0x70 ); }
static void jno( struct label *l ) { return jump_rel8( l, 0x71 ); }
static void jz( struct label *l )  { return jump_rel8( l, 0x74 ); }
static void jnz( struct label *l ) { return jump_rel8( l, 0x75 ); }
static void js( struct label *l )  { return jump_rel8( l, 0x78 ); }
static void jl( struct label *l )  { return jump_rel8( l, 0x7c ); }
static void jge( struct label *l ) { return jump_rel8( l, 0x7d ); }
static void jle( struct label *l ) { return jump_rel8( l, 0x7e ); }
#if 0
static void jc( struct label *l )  { return jump_rel8( l, 0x72 ); }
static void jnc( struct label *l ) { return jump_rel8( l, 0x73 ); }
static void jg( struct label *l )  { return jump_rel8( l, 0x7f ); }
#endif

#define LABELS()  struct label label_A, label_B, label_C, label_D, label_E;label_A.addr = -1;label_A.n_label_uses = 0;label_B.addr = -1;label_B.n_label_uses = 0;label_C.addr = -1;label_C.n_label_uses = 0;label_D.addr = -1;label_D.n_label_uses = 0;label_E.addr=-1;label_E.n_label_uses=0;
#define LABEL_A label(&label_A)
#define LABEL_B label(&label_B)
#define LABEL_C label(&label_C)
#define LABEL_D label(&label_D)
#define LABEL_E label(&label_E)

static void amd64_ins_branch_check_threads_etc(int code_only);
static int func_start = 0;

/* Called at the start of a function. */
void amd64_start_function(int store_lines )
{
  store_pc = store_lines;
  branch_check_threads_update_etc = 0;
  /*
   * store_lines is true for any non-constant evaluation function
   * In reality we should only do this if we are going to be using
   * check_threads_etc (ie, if we have a loop).
   *
   * It does not really waste all that much space in the code, though.
   *
   * We could really do this one time per program instead of once per
   * function.
   *
   * But it is very hard to know when a new program starts, and
   * PIKE_PC is not reliable as a marker since constant evaluations
   * are also done using this code. So for now, waste ~20 bytes per
   * function in the program code.
   */
  if( store_lines )
    amd64_ins_branch_check_threads_etc(1);
  func_start = PIKE_PC;
}

/* Called when the current function is done */
void amd64_end_function(int UNUSED(no_pc))
{
  branch_check_threads_update_etc = 0;
}

#ifdef MACHINE_CODE_STACK_FRAMES
static void amd64_pop_internal_c_frame()
{
  /* C.f. amd64_ins_start_function() */
  mov_reg_reg(P_REG_RBP, P_REG_RSP);
  pop(P_REG_RBP);
  add_reg_imm(P_REG_RSP, 8);
}
#endif /* MACHINE_CODE_STACK_FRAMES */

static void amd64_load_fp_reg(void);
static void amd64_call_c_function(void *addr);

void amd64_ins_start_function(void)
{
#ifdef MACHINE_CODE_STACK_FRAMES
  LABELS();
  amd64_load_fp_reg();
  /* Note: really mem16, but we & with PIKE_FRAME_RETURN_INTERNAL anyway */
  mov_mem32_reg( fp_reg, OFFSETOF(pike_frame, flags), P_REG_RAX );
  and_reg32_imm( P_REG_RAX, PIKE_FRAME_RETURN_INTERNAL);
  jz( &label_A );

  amd64_call_c_function(check_c_stack_margin);

  /* This is an internal frame (no entry prologue), so we'll insert a
     minimal frame that can be used to unwind the C stack. */
  mov_mem_reg(fp_reg, OFFSETOF(pike_frame, next), P_REG_RAX);
  mov_mem_reg(P_REG_RAX, OFFSETOF(pike_frame, return_addr), P_REG_RAX);
  push(P_REG_RAX);
  push(P_REG_RBP);
  mov_reg_reg(P_REG_RSP, P_REG_RBP);

  LABEL_A;
#endif /* MACHINE_CODE_STACK_FRAMES */
}

/* Machine code entry prologue.
 *
 * On entry:
 *   RDI: Pike_interpreter	(ARG1_REG)
 *
 * During interpreting:
 *   R15: Pike_interpreter
 */
void amd64_ins_entry(void)
{
  size_t orig_ppc = PIKE_PC;
  /* Push all registers that the ABI requires to be preserved. */
  push(P_REG_RBP);
  mov_reg_reg(P_REG_RSP, P_REG_RBP);
  push(P_REG_R15);
  push(P_REG_R14);
  push(P_REG_R13);
  push(P_REG_R12);
  push(P_REG_RBX);
  sub_reg_imm(P_REG_RSP, 8);	/* Align on 16 bytes. */
  mov_reg_reg(ARG1_REG, Pike_interpreter_reg);
  amd64_flush_code_generator_state();

  if( PIKE_PC - orig_ppc != ENTRY_PROLOGUE_SIZE ) /* sanity check */
    Pike_fatal("ENTRY_PROLOGUE_SIZE incorrectly set, should be 0x%lx\n",
               (unsigned long)(PIKE_PC-orig_ppc) );
}

void amd64_flush_code_generator_state(void)
{
  amd64_prev_stored_pc = -1;
  ret_for_func = 0;
  fp_reg = -1;
  sp_reg = -1;
  mark_sp_reg = -1;
  dirty_regs = 0;
}

static void flush_dirty_regs(void)
{
  /* NB: PIKE_FP_REG is currently never dirty. */
  if (dirty_regs & (1 << PIKE_SP_REG)) {
    mov_reg_mem(PIKE_SP_REG, Pike_interpreter_reg,
			      OFFSETOF(Pike_interpreter_struct, stack_pointer));
    dirty_regs &= ~(1 << PIKE_SP_REG);
  }
  if (dirty_regs & (1 << PIKE_MARK_SP_REG)) {
    mov_reg_mem(PIKE_MARK_SP_REG, Pike_interpreter_reg,
			      OFFSETOF(Pike_interpreter_struct, mark_stack_pointer));
    dirty_regs &= ~(1 << PIKE_MARK_SP_REG);
  }
}


/* NB: We load Pike_fp et al into registers that
 *     are persistent across function calls.
 */
static void amd64_load_fp_reg(void)
{
  if (fp_reg < 0) {
    mov_mem_reg(Pike_interpreter_reg,
                OFFSETOF(Pike_interpreter_struct, frame_pointer),
                PIKE_FP_REG);
    fp_reg = PIKE_FP_REG;
  }
}

static void amd64_load_sp_reg(void)
{
  if (sp_reg < 0) {
    mov_mem_reg(Pike_interpreter_reg,
                OFFSETOF(Pike_interpreter_struct, stack_pointer),
                PIKE_SP_REG);
    sp_reg = PIKE_SP_REG;
  }
}

static void amd64_load_mark_sp_reg(void)
{
  if (mark_sp_reg < 0) {
    mov_mem_reg(Pike_interpreter_reg,
                OFFSETOF(Pike_interpreter_struct, mark_stack_pointer),
                PIKE_MARK_SP_REG);
    mark_sp_reg = PIKE_MARK_SP_REG;
  }
}


static void mov_sval_type(enum amd64_reg src, enum amd64_reg dst )
{
  mov_mem8_reg( src, SVAL(0).type, dst);
/*  and_reg32_imm( dst, 0x1f );*/
}


#if 0
static void svalue_is_referenced(enum amd64_reg in, struct label *not )
{
    /* bit 4 set, and no higher bit set (all with 8bit values). */
    /* aka: type & 248 == 8. Incidentally, 248 is equal to -8 in signed 8bit*/
    and_reg_imm(in,-8);
    /* jz(not) -- saves one comparison in most cases, adds code size, more jumps. */
    cmp_reg_imm(in,MIN_REF_TYPE);
    jne( not );
}

static void mem_svalue_is_referenced(enum amd64_reg in, struct label *not )
{
    /* bit 4 set, and no higher bit set. */
    if( in == P_REG_RAX ) Pike_error("RAX not supported here.\n");
    mov_sval_type(in,P_REG_RAX);
    svalue_is_referenced(P_REG_RAX, not );
}
#endif

static void update_arg1(INT32 value)
{
    mov_imm_reg(value, ARG1_REG);
  /* FIXME: Alloc stack space on NT. */
}

static void update_arg2(INT32 value)
{
    mov_imm_reg(value, ARG2_REG);
  /* FIXME: Alloc stack space on NT. */
}

static void amd64_add_sp( int num )
{
  amd64_load_sp_reg();
  add_reg_imm( sp_reg, num * sizeof(struct svalue));
  dirty_regs |= 1 << PIKE_SP_REG;
  flush_dirty_regs(); /* FIXME: Need to change LABEL handling to remove this  */
}

static void amd64_add_mark_sp( int num )
{
  amd64_load_mark_sp_reg();
  add_reg_imm( mark_sp_reg, num * sizeof(struct svalue*));
  dirty_regs |= 1 << PIKE_MARK_SP_REG;
  flush_dirty_regs(); /* FIXME: Need to change LABEL handling to remove this */
}

/* Note: Uses RAX and RCX internally. reg MUST not be P_REG_RAX. */
static void amd64_push_svaluep_to(int reg, int spoff)
{
  LABELS();
  if( reg == P_REG_RAX )
    Pike_fatal("Using RAX in push_svaluep not supported\n" );
  amd64_load_sp_reg();
  mov_mem_reg(reg, OFFSETOF(svalue, tu.t.type), P_REG_RAX);
  mov_mem_reg(reg, OFFSETOF(svalue, u.refs), P_REG_RCX);
  mov_reg_mem(P_REG_RAX, sp_reg, SVAL(spoff).type);
  mov_reg_mem(P_REG_RCX, sp_reg, SVAL(spoff).value);
  and_reg32_imm(P_REG_RAX, MIN_REF_TYPE);
  jz(&label_A);
  add_imm_mem( 1, P_REG_RCX, OFFSETOF(pike_string, refs));
 LABEL_A;
}

static void amd64_push_svaluep(int reg)
{
  amd64_push_svaluep_to( reg, 0 );
  amd64_add_sp( 1 );
}

static void amd64_push_int(INT64 value, int subtype)
{
  amd64_load_sp_reg();
  mov_imm_mem((subtype<<16) + PIKE_T_INT, sp_reg, SVAL(0).type);
  mov_imm_mem(value, sp_reg, SVAL(0).value);
  amd64_add_sp( 1 );
}

static void amd64_push_int_reg(enum amd64_reg reg )
{
  amd64_load_sp_reg();
  mov_imm_mem( PIKE_T_INT, sp_reg, OFFSETOF(svalue, tu.t.type));
  mov_reg_mem( reg, sp_reg, OFFSETOF(svalue, u.integer));
  amd64_add_sp( 1 );
}

static void amd64_get_storage( enum amd64_reg reg, ptrdiff_t offset )
{
  amd64_load_fp_reg();
#if 1
  /* Note: We really should keep pike_frame->current_storage up to date instead.. */
  mov_mem_reg( fp_reg, OFFSETOF(pike_frame,current_storage), reg );
  add_reg_imm( reg, offset );
#else
  /* fp->current_object->storage */
  mov_mem_reg(fp_reg, OFFSETOF(pike_frame, current_object), P_REG_RAX);
  mov_mem_reg(P_REG_RAX, OFFSETOF(object,storage), reg );
  mov_mem_reg(fp_reg, OFFSETOF(pike_frame, context), P_REG_RAX);
  /* + fp->context->storage_offset */
  add_reg_mem( reg, P_REG_RAX, OFFSETOF(inherit,storage_offset) );
  /* + offset */
  add_reg_imm( reg, offset);
#endif
}


static void amd64_mark(int offset)
{
  amd64_load_sp_reg();
  amd64_load_mark_sp_reg();
  if (offset) {
    add_reg_imm_reg(sp_reg, -offset * sizeof(struct svalue), P_REG_RAX);
    mov_reg_mem(P_REG_RAX, mark_sp_reg, 0);
  } else {
    mov_reg_mem(sp_reg, mark_sp_reg, 0);
  }
   amd64_add_mark_sp( 1 );
}

static void amd64_call_c_function(void *addr)
{
  flush_dirty_regs();
  call_imm(addr);
}

static void amd64_free_svalue_off(enum amd64_reg src, int off, int guaranteed_ref )
{
  LABELS();
  if( src == P_REG_RAX )
    Pike_fatal("Clobbering RAX for free-svalue\n");
    /* load type -> RAX */
  mov_mem8_reg( src, off, P_REG_RAX );
  and_reg_imm(P_REG_RAX, MIN_REF_TYPE);
  jz( &label_A );

  /* Load pointer to refs -> RAX */
  mov_mem_reg( src,off+OFFSETOF(svalue, u.refs), P_REG_RAX);
   /* if( !--*RAX ) */
  add_mem32_imm( P_REG_RAX, OFFSETOF(pike_string,refs),  -1);
  if( !guaranteed_ref )
  {
    /* We need to see if refs got to 0. */
    jnz( &label_A );
    /* if so, call really_free_svalue */
    add_reg_imm_reg( src, off, ARG1_REG );
    amd64_call_c_function(really_free_svalue);
  }
  LABEL_A;
}

static void amd64_free_svalue(enum amd64_reg src, int guaranteed_ref )
{
  amd64_free_svalue_off(src,0,guaranteed_ref);
}

/* Type already in register. Note: Clobbers the type register. */
static void amd64_free_svalue_type(enum amd64_reg src, enum amd64_reg type,
                                   int guaranteed_ref )
{
  LABELS();
  /* if type < MIN_REF_TYPE+1 */
  if( src == P_REG_RAX )
    Pike_fatal("Clobbering RAX for free-svalue\n");

  and_reg32_imm(type, MIN_REF_TYPE);
  jz( &label_A );

  /* Load pointer to refs -> RAX */
  mov_mem_reg( src, OFFSETOF(svalue, u.refs), P_REG_RAX);
   /* if( !--*RAX ) */
  add_mem32_imm( P_REG_RAX, OFFSETOF(pike_string,refs),  -1);
  if( !guaranteed_ref )
  {
    /* We need to see if refs got to 0. */
    jnz( &label_A );
    /* if so, call really_free_svalue */
    if( src != ARG1_REG )
      mov_reg_reg( src, ARG1_REG );
    amd64_call_c_function(really_free_svalue);
  }
  LABEL_A;
}

void amd64_ref_svalue( enum amd64_reg src, int already_have_type )
{
  LABELS();
  if( src == P_REG_RAX ) Pike_fatal("Clobbering src in ref_svalue\n");
  if( !already_have_type )
      mov_sval_type( src, P_REG_RAX );
  and_reg32_imm( P_REG_RAX, MIN_REF_TYPE );
  jz( &label_A );
  /* Load pointer to refs -> RAX */
  mov_mem_reg( src, OFFSETOF(svalue, u.refs), P_REG_RAX);
   /* *RAX++ */
  add_mem32_imm( P_REG_RAX, OFFSETOF(pike_string,refs),  1);
 LABEL_A;
}

void amd64_assign_svalue_no_free( enum amd64_reg dst, enum amd64_reg src, ptrdiff_t src_offset )
{
  if( dst == P_REG_RAX ||src == P_REG_RAX )
    Pike_fatal( "Clobbering src/dst in amd64_assign_svalue_no_free <%d,%d>\n", dst, src);
  /* Copy src -> dst */
  mov_mem_reg(src, src_offset, P_REG_RAX);
  mov_reg_mem(P_REG_RAX, dst, 0 );
  mov_mem_reg(src, src_offset+sizeof(long), P_REG_RAX);
  mov_reg_mem(P_REG_RAX, dst, sizeof(long) );
}

void amd64_assign_local( int b )
{
  amd64_load_fp_reg();
  amd64_load_sp_reg();
  mov_mem_reg( fp_reg, OFFSETOF(pike_frame, locals), P_REG_RBX);
  add_reg_imm( P_REG_RBX, b*sizeof(struct svalue) );
  amd64_free_svalue(P_REG_RBX, 0);
  amd64_assign_svalue_no_free( P_REG_RBX, sp_reg, -1*sizeof(struct svalue));
}

static void amd64_pop_mark(void)
{
  amd64_add_mark_sp( -1 );
}

static void amd64_push_string(int strno, int subtype)
{
  struct pike_string *str = Pike_compiler->new_program->strings[strno];
  amd64_load_sp_reg();

  mov_imm_reg((INT64)str,P_REG_RAX);
  mov_imm_mem((subtype<<16) | PIKE_T_STRING, sp_reg, SVAL(0).type);
  mov_reg_mem(P_REG_RAX,sp_reg,SVAL(0).value);
  add_imm_mem(1, P_REG_RAX,OFFSETOF(pike_string, refs));

  amd64_add_sp(1);
}

static void amd64_push_local_function(int fun)
{
  amd64_load_fp_reg();
  amd64_load_sp_reg();
  mov_mem_reg(fp_reg, OFFSETOF(pike_frame, context), P_REG_RAX);
  mov_mem_reg(fp_reg, OFFSETOF(pike_frame, current_object),P_REG_RCX);
  mov_mem32_reg(P_REG_RAX, OFFSETOF(inherit, identifier_level),
                P_REG_RAX);
  mov_reg_mem(P_REG_RCX, sp_reg, OFFSETOF(svalue, u.object));
  add_reg_imm(P_REG_RAX, fun);
  add_imm_mem(1, P_REG_RCX,(INT32)OFFSETOF(object, refs));
  shl_reg_imm(P_REG_RAX, 16);
  add_reg_imm(P_REG_RAX, PIKE_T_FUNCTION);
  mov_reg_mem(P_REG_RAX, sp_reg, OFFSETOF(svalue, tu.t.type));
  amd64_add_sp(1);
}

void amd64_update_pc(void)
{
  INT32 tmp = PIKE_PC, disp;
  const enum amd64_reg tmp_reg = P_REG_RAX;
#ifdef PIKE_AMD64_VALIDATE_RSP
  LABELS();
#endif

  if(amd64_prev_stored_pc == - 1)
  {
    amd64_load_fp_reg();
    mov_rip_imm_reg(tmp - PIKE_PC, tmp_reg);
    mov_reg_mem(tmp_reg, fp_reg, OFFSETOF(pike_frame, pc));
#ifdef PIKE_DEBUG
    if (a_flag >= 60)
      fprintf (stderr, "pc %d  update pc via lea\n", tmp);
#endif
    amd64_prev_stored_pc = tmp;
  }
  else if ((disp = tmp - amd64_prev_stored_pc))
  {
#ifdef PIKE_DEBUG
    if (a_flag >= 60)
      fprintf (stderr, "pc %d  update pc relative: %d\n", tmp, disp);
#endif
    amd64_load_fp_reg();
    mov_rip_imm_reg(tmp - PIKE_PC, tmp_reg);
    mov_reg_mem(tmp_reg, fp_reg, OFFSETOF(pike_frame, pc));
    /* amd64_load_fp_reg(); */
    /* add_imm_mem(disp, fp_reg, OFFSETOF (pike_frame, pc)); */
    amd64_prev_stored_pc += disp;
  }
   else {
#ifdef PIKE_DEBUG
    if (a_flag >= 60)
      fprintf (stderr, "pc %d  update pc - already up-to-date\n", tmp);
#endif
   }

#ifdef PIKE_AMD64_VALIDATE_RSP
  mov_reg_reg(P_REG_RSP, tmp_reg);
  and_reg_imm(tmp_reg, 0x0f);
  je(&label_A);
  /* Broken RSP */
  call_reg(tmp_reg);	/* Jump to zero-page should trigger a SIGSEGV. */
  LABEL_A;
#endif
}


static void maybe_update_pc(void)
{
  static int last_prog_id=-1;
  static size_t last_num_linenumbers=-1;
  if( !store_pc ) return;

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

static void maybe_load_fp(void)
{
  static int last_prog_id=-1;
  static size_t last_num_linenumbers=-1;
  if( !store_pc ) return;

  if(
#ifdef PIKE_DEBUG
    /* Update the pc more often for the sake of the opcode level trace. */
     d_flag ||
#endif
     (amd64_prev_stored_pc == -1) ||
     last_prog_id != Pike_compiler->new_program->id ||
     last_num_linenumbers != Pike_compiler->new_program->num_linenumbers
  ) {
    amd64_load_fp_reg();
  }
}

static void sync_registers(int flags)
{
  maybe_update_pc();
  flush_dirty_regs();

  if (flags & I_UPDATE_SP) sp_reg = P_REG_INVALID;
  if (flags & I_UPDATE_M_SP) mark_sp_reg = P_REG_INVALID;
  if (flags & I_UPDATE_FP) fp_reg = P_REG_INVALID;
}

static void amd64_call_c_opcode(void *addr, int flags)
{
  sync_registers(flags);
  call_imm( addr );
}


#ifdef PIKE_DEBUG
static void ins_debug_instr_prologue (PIKE_INSTR_T instr, INT32 arg1, INT32 arg2)
{
  /* NB: instr has been offset by F_OFFSET. */
  int flags = instrs[instr].flags;

  /* Note: maybe_update_pc() is called by amd64_call_c_opcode() above,
   *       which has the side-effect of loading fp_reg. Some of the
   *       opcodes use amd64_call_c_opcode() in conditional segments.
   *       This is to make sure that fp_reg is always loaded on exit
   *       from such opcodes.
   */

  maybe_load_fp();

/* For now: It is very hard to read the disassembled source when
   this is inserted */
  if( !d_flag )
    return;

  maybe_update_pc();

  if (flags & I_HASARG2)
      mov_imm_reg(arg2, ARG3_REG);
  if (flags & I_HASARG)
      mov_imm_reg(arg1, ARG2_REG);
  mov_imm_reg(instr, ARG1_REG);

  if (flags & I_HASARG2)
    amd64_call_c_function (simple_debug_instr_prologue_2);
  else if (flags & I_HASARG)
    amd64_call_c_function (simple_debug_instr_prologue_1);
  else
    amd64_call_c_function (simple_debug_instr_prologue_0);
}
#else  /* !PIKE_DEBUG */
#define ins_debug_instr_prologue(instr, arg1, arg2) maybe_load_fp()
#endif

static void amd64_push_this_object( )
{
  amd64_load_fp_reg();
  amd64_load_sp_reg();

  mov_imm_mem( PIKE_T_OBJECT, sp_reg, OFFSETOF(svalue, tu.t.type));
  mov_mem_reg( fp_reg, OFFSETOF(pike_frame, current_object), P_REG_RAX );
  mov_reg_mem( P_REG_RAX, sp_reg, OFFSETOF(svalue,u.object) );
  add_mem32_imm( P_REG_RAX, (INT32)OFFSETOF(object, refs), 1);
  amd64_add_sp( 1 );
}

static void amd64_align()
{
  while( PIKE_PC & 3 )
    ib( 0x90 );
}

static void amd64_ins_branch_check_threads_etc(int code_only)
{
  LABELS();

  if( !branch_check_threads_update_etc )
  {
    /* Create update + call to branch_check_threads_etc */
    if( !code_only )
      jmp( &label_A );
    branch_check_threads_update_etc = PIKE_PC;
    if( (unsigned long long)&fast_check_threads_counter < 0x7fffffffULL )
    {
      /* Short pointer. */
      clear_reg( P_REG_RAX );
      add_mem32_imm( P_REG_RAX,
                     (int)(ptrdiff_t)&fast_check_threads_counter,
                     0x80 );
    }
    else
    {
      mov_imm_reg( (long)&fast_check_threads_counter, P_REG_RAX);
      add_mem_imm( P_REG_RAX, 0, 0x80 );
    }
    mov_imm_reg( (ptrdiff_t)branch_check_threads_etc, P_REG_RAX );
    jmp_reg(P_REG_RAX); /* ret in BCTE will return to desired point. */
    amd64_align();
  }
  if( !code_only )
  {
    LABEL_A;
    /* Use C-stack for counter. We have padding added in entry */
#ifndef USE_VALGRIND
#ifndef MACHINE_CODE_STACK_FRAMES
    /* FIXME: Counter disabled when internal stack frames are used
       since RSP won't point to the entry-level stack frame with
       padding. Dunno if there should be a counter for every internal
       frame too? But that would double C-stack usage per internal
       frame due to 16 byte alignment. */
    add_mem8_imm( P_REG_RSP, 0, 1 );
    jno( &label_B );
#endif
#endif
    call_rel_imm32( branch_check_threads_update_etc );
#ifndef USE_VALGRIND
   LABEL_B;
#endif
  }
}


void amd64_init_interpreter_state(void)
{
#ifdef PIKE_DEBUG
  if( PIKE_T_INT != 0 )
    Pike_fatal("assumption failed: pike_t_int == 0\n");
  if( sizeof(struct svalue) != 16 )
    Pike_fatal("assumption failed: sizeof svalue != 16\n");
  if( OFFSETOF(svalue,tu.t.type) != 0 )
    Pike_fatal("assumption failed: offsetof(svalue.tu.t.type) != 0\n");
  if( OFFSETOF(svalue,u.integer) != 8 )
    Pike_fatal("assumption failed: offsetof(svalue.u.integer) != 8\n");
#endif
}

static void amd64_return_from_function()
{
  if( ret_for_func )
  {
    jmp_rel_imm( ret_for_func );
  }
  else
  {
    ret_for_func = PIKE_PC;
    pop(P_REG_RBX);	/* Stack padding. */
    pop(P_REG_RBX);
    pop(P_REG_R12);
    pop(P_REG_R13);
    pop(P_REG_R14);
    pop(P_REG_R15);
    pop(P_REG_RBP);
    ret();
  }
}

static void if_not_two_int(struct label *to, int load)
{
    amd64_load_sp_reg();
    mov_mem8_reg(sp_reg, SVAL(-1).type, P_REG_RAX );
    mov_mem8_reg(sp_reg, SVAL(-2).type, P_REG_RBX );
    add_reg_reg(P_REG_RAX,P_REG_RBX);
    /*    test_reg32(P_REG_RAX); int == 0 */
    jnz(to);
    if( load )
    {
      mov_mem_reg(sp_reg, SVAL(-1).value, P_REG_RAX );
      mov_mem_reg(sp_reg, SVAL(-2).value, P_REG_RBX );
    }
}

void ins_f_byte(unsigned int b)
{
  int flags;
  void *addr;

  LABELS();

  b-=F_OFFSET;
#ifdef PIKE_DEBUG
  if(b > MAX_SUPPORTED_INSTR)
    Pike_error("Instruction too big %d\n",b);
#endif
  maybe_update_pc();

  flags = instrs[b].flags;

  addr=instrs[b].address;
  switch(b + F_OFFSET)
  {
#if 0
  case F_ESCAPE_CATCH:

  case F_EXIT_CATCH:
    ins_debug_instr_prologue(b, 0, 0);
    ins_f_byte( F_ESCAPE_CATCH );
    amd64_load_sp_reg();
    amd64_push_int( 0, 1 );
    return;
#endif
    /*  -3       -2        -1       0
     * lval[0], lval[1], <RANDOM>, **SP**
     * ->
     *  -4       -3       -2      -1      0
     * lval[0], lval[1], *lval, <RANDOM>, *SP**
     *
     * This will also free *lval iff it has type
     * array,multiset,mapping or string.
     */
  case F_LTOSVAL2_AND_FREE:
    {
      ins_debug_instr_prologue(b, 0, 0);
      amd64_load_sp_reg();
      mov_mem8_reg( sp_reg, -3*sizeof(struct svalue), P_REG_RBX );
      cmp_reg_imm( P_REG_RBX, T_SVALUE_PTR );
      jne( &label_A );

      /* inline version for SVALUE_PTR. */

      /* First, make room for the result, move top and inc sp */
      mov_mem_reg( sp_reg, -sizeof(struct svalue), P_REG_RAX );
      mov_mem_reg( sp_reg, -sizeof(struct svalue)+8, P_REG_RCX );
      mov_reg_mem( P_REG_RAX, sp_reg, 0);
      mov_reg_mem( P_REG_RCX, sp_reg, 8);
      amd64_add_sp( 1 );

      mov_mem_reg( sp_reg,
                   -4*sizeof(struct svalue)+OFFSETOF(svalue,u.lval),
                   P_REG_RBX );
      amd64_push_svaluep_to(P_REG_RBX, -2);

      /* Result is now in sp[-2], lval in -4. */
      /* push svaluep leaves type in RAX */
      mov_reg_reg( P_REG_RAX, P_REG_RCX );
      /* mov_mem8_reg( sp_reg, -2*sizeof(struct svalue), P_REG_RCX ); */
      mov_imm_reg( 1, P_REG_RAX );
      shl_reg32_reg( P_REG_RAX, P_REG_RCX );
      and_reg_imm( P_REG_RAX, (BIT_ARRAY|BIT_MULTISET|BIT_MAPPING|BIT_STRING));
      jz( &label_B ); /* Do nothing.. */

      /* Free the old value. */
      amd64_free_svalue( P_REG_RBX, 0 );
      /* assign 0 */
      mov_imm_mem( PIKE_T_INT, P_REG_RBX, 0 );
      mov_imm_mem( 0, P_REG_RBX, 8 );
      jmp( &label_B ); /* All done */

      LABEL_A;
      /* So, not a svalueptr. Use C-version */
      amd64_call_c_opcode( addr, flags );
      amd64_load_sp_reg();
      LABEL_B;
    }
    return;

  case F_LTOSVAL:
    {
      ins_debug_instr_prologue(b, 0, 0);
      amd64_load_sp_reg();
      mov_mem8_reg( sp_reg, -sizeof(struct svalue)*2, P_REG_RAX );
      /* lval type in RAX. */
      /*
        if( rax == T_SVALUE_PTR )
          push( *l->u.lval )

        possibly:
        if( rax == T_OBJECT )
          object_index_no_free( to, lval->u.object, lval->subtype, &lval[1])

        if( rax == T_ARRAY && lval[1].type == PIKE_T_INT )
          push( l->u.array->item[&lval[1].u.integer] )
      */
      cmp_reg_imm( P_REG_RAX, T_SVALUE_PTR );
      je( &label_A );
      /* So, not a svalueptr */
      mov_reg_reg( sp_reg, ARG1_REG );
      add_reg_imm_reg( sp_reg, -2*sizeof(struct svalue), ARG2_REG );
      amd64_call_c_function(lvalue_to_svalue_no_free);
      amd64_load_sp_reg();
      amd64_add_sp(1);
      jmp(&label_B);
      LABEL_A;
      mov_mem_reg( sp_reg, -sizeof(struct svalue)*2+OFFSETOF(svalue,u.lval),
                   P_REG_RCX );
      amd64_push_svaluep(P_REG_RCX);
      LABEL_B;
    }
    return;
  case F_ASSIGN:
   {
     ins_debug_instr_prologue(b, 0, 0);
      amd64_load_sp_reg();
      mov_mem8_reg( sp_reg, -3*sizeof(struct svalue), P_REG_RAX );
      cmp_reg_imm( P_REG_RAX, T_SVALUE_PTR );

      je( &label_A );
      /* So, not a svalueptr. Use C-version for simplicity */
      amd64_call_c_opcode( addr, flags );
      amd64_load_sp_reg();
      jmp( &label_B );
      amd64_align();

     LABEL_A;
      mov_mem_reg( sp_reg, -3*sizeof(struct svalue)+8, P_REG_RBX );

      /* Free old value. */
      amd64_free_svalue( P_REG_RBX, 0 );

      /* assign new value */
      /* also move assigned value -> sp[-3] */
      mov_mem_reg( sp_reg, -sizeof(struct svalue), P_REG_RAX );
      mov_mem_reg( sp_reg, -sizeof(struct svalue)+8, P_REG_RCX );
      mov_reg_mem( P_REG_RAX, P_REG_RBX, 0 );
      mov_reg_mem( P_REG_RCX, P_REG_RBX, 8 );
      add_reg_imm_reg( sp_reg, -sizeof(struct svalue), P_REG_RCX );
      amd64_push_svaluep_to( P_REG_RCX, -3 );
      /*
        Note: For SVALUEPTR  we know we do not have to free
        the lvalue.
      */
      amd64_add_sp( -2 );
     LABEL_B;
   }
   return;
  case F_ASSIGN_AND_POP:
   {
     ins_debug_instr_prologue(b, 0, 0);
      amd64_load_sp_reg();
      mov_mem8_reg( sp_reg, -3*sizeof(struct svalue), P_REG_RAX );
      cmp_reg_imm( P_REG_RAX, T_SVALUE_PTR );

      je( &label_A );
      /* So, not a svalueptr. Use C-version for simplicity */
      amd64_call_c_opcode( addr, flags );
      amd64_load_sp_reg();
      jmp( &label_B );
      amd64_align();

     LABEL_A;
      mov_mem_reg( sp_reg, -3*sizeof(struct svalue)+8, P_REG_RBX );

      /* Free old value. */
      amd64_free_svalue( P_REG_RBX, 0 );

      /* assign new value */
      mov_mem_reg( sp_reg, -sizeof(struct svalue), P_REG_RAX );
      mov_mem_reg( sp_reg, -sizeof(struct svalue)+8, P_REG_RCX );
      mov_reg_mem( P_REG_RAX, P_REG_RBX, 0 );
      mov_reg_mem( P_REG_RCX, P_REG_RBX, 8 );
      /*
        Note: For SVALUEPTR  we know we do not have to free
        the lvalue.
      */
      amd64_add_sp( -3 );
     LABEL_B;
   }
   return;

  case F_NEGATE:
    {
    LABELS();
    ins_debug_instr_prologue(b, 0, 0);
    amd64_load_sp_reg();
    mov_mem8_reg(sp_reg, SVAL(-1).type, P_REG_RAX );
    test_reg32(P_REG_RAX);
    jz(&label_B);
    jmp(&label_A);
   LABEL_D;
    neg_mem(sp_reg, SVAL(-1).value );
   LABEL_A;
    amd64_call_c_opcode(addr, flags);
    jmp(&label_C);
   LABEL_B;
     mov_imm_mem(PIKE_T_INT, sp_reg, SVAL(-1).type );
     neg_mem(sp_reg, SVAL(-1).value );
     jo(&label_D);
   LABEL_C;
    }
    return;

  case F_NOT:
    {
    LABELS();
    amd64_load_sp_reg();
    mov_mem8_reg(sp_reg, SVAL(-1).type, P_REG_RCX );
    test_reg32(P_REG_RCX);
    jz(&label_B); /* integer. */
    mov_imm_reg( 1, P_REG_RAX );
    shl_reg32_reg( P_REG_RAX, P_REG_RCX );
    and_reg32_imm( P_REG_RAX, (BIT_FLOAT|BIT_ARRAY|BIT_MULTISET|BIT_MAPPING|BIT_STRING|BIT_PROGRAM));
    jnz( &label_A );
    /* object or function */
    amd64_call_c_opcode(addr, flags);
    jmp(&label_C);
    /* int. */
   LABEL_B;
    mov_mem_reg( sp_reg, SVAL(-1).value, P_REG_RAX );
    clear_reg( P_REG_RCX );
    cmp_reg_imm( P_REG_RAX, 0 );
    set_if_eq(P_REG_RCX);
    mov_imm_mem( PIKE_T_INT, sp_reg, SVAL(-1).type );
    mov_reg_mem( P_REG_RCX, sp_reg, SVAL(-1).value );
    jmp(&label_C);
  LABEL_A; /* array etc, !x is always false */
    amd64_free_svalue_off( sp_reg, SVAL(-1).off, 0 );
    mov_imm_mem( PIKE_T_INT, sp_reg, SVAL(-1).type );
    mov_imm_mem( 0, sp_reg, SVAL(-1).value );
  LABEL_C;
    }
    return;
  case F_COMPL:
    {
    LABELS();
    ins_debug_instr_prologue(b, 0, 0);
    amd64_load_sp_reg();
    mov_mem8_reg(sp_reg, SVAL(-1).type, P_REG_RAX );
    test_reg32(P_REG_RAX);
    jz(&label_B);
   LABEL_A;
    amd64_call_c_opcode(addr, flags);
    jmp(&label_C);
   LABEL_B;
     not_mem(sp_reg, SVAL(-1).value );
   LABEL_C;
    }
    return;

  case F_MOD:
  case F_DIVIDE:
    {
      LABELS();
      if_not_two_int(&label_A,0);
      mov_mem_reg(sp_reg, SVAL(-1).value, P_REG_RBX );
      mov_mem_reg(sp_reg, SVAL(-2).value, P_REG_RAX );
      cmp_reg_imm(P_REG_RAX, 0 );  jl(&label_A);
      if( b+F_OFFSET == F_MOD )
      {
        /* Emulates pikes % functionality. */
        cmp_reg_imm(P_REG_RBX, 0 );  jl(&label_A);
      }
      cmp_reg_imm(P_REG_RBX,0);
      je(&label_A);
      cmp_reg_imm(P_REG_RBX,-1);
      jne(&label_C);
      cmp_reg_imm(P_REG_RAX,0);
      jle(&label_A);
      /* FIXME:

	 Int.native_min / -1 -> exception.

	 This checks for -<whatever> / -1.

	 There is bound to be a better way.
	 Somehow disable the exception?

	 Some SSE alternative that does not throw?

	 Perhaps a 64/64->64 instead of the current 128/64->64?
      */
   LABEL_C;
      div_reg_reg(P_REG_RAX,P_REG_RBX);
      mov_imm_mem(PIKE_T_INT, sp_reg, SVAL(-2).type);
      if( b+F_OFFSET == F_MOD )
	mov_reg_mem(P_REG_RDX,  sp_reg, SVAL(-2).value);
      else
	mov_reg_mem(P_REG_RAX,  sp_reg, SVAL(-2).value);
      amd64_add_sp(-1);
      jmp(&label_B);
   LABEL_A;
      amd64_call_c_opcode(addr, flags);
      amd64_load_sp_reg();
   LABEL_B;
    }
    return;
  case F_MULTIPLY:
    {
      LABELS();
      if_not_two_int(&label_C,1);
      amd64_add_sp(-1);
      mul_rax_reg_reg128(P_REG_RBX);
      jo(&label_A);
      mov_imm_mem(PIKE_T_INT, sp_reg, SVAL(-1).type); /* Only needed for UNDEFINED*x -> 0 */
      jmp(&label_B);
   LABEL_C;
      amd64_call_c_opcode(addr, flags);
      amd64_load_sp_reg();
      jmp(&label_D);
   LABEL_A;
      /* overflows signed. upper 64 in edx, lower in eax. */
      mov_reg_reg( P_REG_RAX, ARG1_REG );
      mov_reg_reg( P_REG_RDX, ARG2_REG ); /* (on unix, not generated, arg2 is rdx) */
      amd64_call_c_function(create_double_bignum);
      mov_imm_mem(PIKE_T_OBJECT, sp_reg, SVAL(-1).type);
   LABEL_B;
      mov_reg_mem(P_REG_RAX,     sp_reg, SVAL(-1).value);
   LABEL_D;
    }
    return;

  case F_AND:
    {
      LABELS();
      ins_debug_instr_prologue(b, 0, 0);
      if_not_two_int(&label_A,1);
      and_reg_reg(P_REG_RBX,P_REG_RAX);
      mov_imm_mem(PIKE_T_INT,sp_reg,SVAL(-2).type);
      mov_reg_mem(P_REG_RBX,sp_reg,SVAL(-2).value);
      amd64_add_sp(-1);
      jmp(&label_B);
     LABEL_A;
      amd64_call_c_opcode(addr, flags);
      amd64_load_sp_reg();
     LABEL_B;
    }
    return;

  case F_OR:
    {
    LABELS();
    ins_debug_instr_prologue(b, 0, 0);
    if_not_two_int(&label_A,1);
    or_reg_reg(P_REG_RAX,P_REG_RBX);
    mov_imm_mem(PIKE_T_INT,sp_reg,SVAL(-2).type);
    mov_reg_mem(P_REG_RBX,sp_reg,SVAL(-2).value);
    amd64_add_sp(-1);
    jmp(&label_B);
   LABEL_A;
    amd64_call_c_opcode(addr, flags);
    amd64_load_sp_reg();
   LABEL_B;
    }
    return;

  case F_XOR:
    {
    LABELS();
    ins_debug_instr_prologue(b, 0, 0);
    if_not_two_int(&label_A,1);
    xor_reg_reg(P_REG_RAX,P_REG_RBX);
    mov_imm_mem(PIKE_T_INT,sp_reg,SVAL(-2).type);
    mov_reg_mem(P_REG_RBX,sp_reg,SVAL(-2).value);
    amd64_add_sp(-1);
    jmp(&label_B);
   LABEL_A;
    amd64_call_c_opcode(addr, flags);
    amd64_load_sp_reg();
   LABEL_B;
    }
    return;

  case F_RSH:
    {
    LABELS();
    ins_debug_instr_prologue(b, 0, 0);
    if_not_two_int(&label_A,0);
    mov_mem_reg(sp_reg, SVAL(-1).value, P_REG_RCX );
    cmp_reg_imm(P_REG_RCX,0);
    jl( &label_A );
    cmp_reg_imm(P_REG_RCX,63);
    jl( &label_B );
  LABEL_A;
    amd64_call_c_opcode(addr, flags);
    amd64_load_sp_reg();
    jmp(&label_C);
  LABEL_B;
    mov_imm_mem(PIKE_T_INT,sp_reg,SVAL(-2).type);
    shr_mem_reg( sp_reg, SVAL(-2).value, P_REG_RCX);
    amd64_add_sp(-1);
  LABEL_C;
    }
    return;

  case F_LSH:
    {
    LABELS();
    ins_debug_instr_prologue(b, 0, 0);
    if_not_two_int(&label_A,0);
    mov_mem_reg(sp_reg, SVAL(-1).value, P_REG_RCX );
    cmp_reg_imm(P_REG_RCX,0);
    jl( &label_A );
    cmp_reg_imm(P_REG_RCX,63);
    jl( &label_B );
  LABEL_A;
    amd64_call_c_opcode(addr, flags);
    amd64_load_sp_reg();
    jmp(&label_C);
  LABEL_B;
    mov_imm_mem(PIKE_T_INT,sp_reg,SVAL(-2).type);
    mov_mem_reg( sp_reg, SVAL(-2).value, P_REG_RBX);

    /* ok. It would have been nice is sal set a
       bit that stayed set when you shifted out a 1 bit. :)
    */
    mov_reg_reg( P_REG_RBX, P_REG_RAX );
    shl_reg_reg( P_REG_RBX, P_REG_RCX);
    mov_reg_reg( P_REG_RBX, P_REG_RDX );
    shr_reg_reg( P_REG_RDX, P_REG_RCX );
    cmp_reg_reg( P_REG_RDX, P_REG_RAX );
    jne( &label_A );
    mov_reg_mem( P_REG_RBX,  sp_reg, SVAL(-2).value);
    amd64_add_sp(-1);
  LABEL_C;
    }
    return;
  case F_ADD_INTS:
    {
      ins_debug_instr_prologue(b, 0, 0);
      amd64_load_sp_reg();
      mov_mem_reg( sp_reg, -sizeof(struct svalue)*2, P_REG_RAX );
      add_reg_mem( P_REG_RAX, sp_reg, -sizeof(struct svalue ) );
      jnz( &label_A );
      /* So. Both are actually integers. */
      mov_mem_reg( sp_reg,
                   -sizeof(struct svalue)+OFFSETOF(svalue,u.integer),
                   P_REG_RAX );

      add_reg_mem( P_REG_RAX,
                   sp_reg,
                   -sizeof(struct svalue)*2+OFFSETOF(svalue,u.integer));

      jo( &label_A );
      amd64_add_sp( -1 );
      /* the type only needs to be set if the subtype was set before.
         It was not, since the type check before would not have
         triggered. */
      mov_reg_mem( P_REG_RAX, sp_reg,
                   -sizeof(struct svalue)+OFFSETOF(svalue,u.integer));
      jmp( &label_B );

      LABEL_A;
      /* Fallback version */
      update_arg1( 2 );
      amd64_call_c_opcode( f_add, I_UPDATE_SP );
      amd64_load_sp_reg();
      LABEL_B;
    }
    return;

  case F_INDEX:
    /*
      pike_sp[-2][pike_sp[-1]]
    */
    ins_debug_instr_prologue(b, 0, 0);
    amd64_load_sp_reg();
    mov_mem8_reg( sp_reg, -2*sizeof(struct svalue), P_REG_RAX );
    mov_mem8_reg( sp_reg, -1*sizeof(struct svalue), P_REG_RBX );
    shl_reg_imm( P_REG_RAX, 8 );
    add_reg_reg( P_REG_RAX, P_REG_RBX );
    mov_mem_reg( sp_reg, -1*sizeof(struct svalue)+8, P_REG_RBX ); /* int */
    mov_mem_reg( sp_reg, -2*sizeof(struct svalue)+8, P_REG_RCX ); /* value */
    cmp_reg32_imm( P_REG_RAX, (PIKE_T_ARRAY<<8)|PIKE_T_INT );
    jne( &label_A );

    /* Array and int index. */
    mov_mem32_reg( P_REG_RCX, OFFSETOF(array,size), P_REG_RDX );
    cmp_reg_imm( P_REG_RBX, 0 ); jge( &label_D );
    /* less than 0, add size */
    add_reg_reg( P_REG_RBX, P_REG_RDX );

   LABEL_D;
    cmp_reg32_imm( P_REG_RBX, 0 ); jl( &label_B ); // <0
    cmp_reg_reg( P_REG_RBX, P_REG_RDX); jge( &label_B ); // >size

    /* array, index inside array. push item, swap, pop, done */
    push( P_REG_RCX ); /* Save array pointer */
    mov_mem_reg( P_REG_RCX, OFFSETOF(array,item), P_REG_RCX );
    shl_reg_imm( P_REG_RBX, 4 );
    add_reg_reg( P_REG_RBX, P_REG_RCX );
    /* This overwrites the array. */
    amd64_add_sp( -1 );
    amd64_push_svaluep_to( P_REG_RBX, -1 );

    pop( P_REG_RCX );
    /* We know it's an array. */
    add_mem32_imm( P_REG_RCX, OFFSETOF(array,refs),  -1);
    jnz( &label_C );
    mov_reg_reg( P_REG_RCX, ARG1_REG );
    amd64_call_c_function(really_free_array);
    jmp( &label_C );

   LABEL_A;
#if 0
    cmp_reg32_imm( P_REG_RAX, (PIKE_T_STRING<<8)|PIKE_T_INT );
    jne( &label_B );
#endif
   LABEL_B;
    /* something else. */
    amd64_call_c_opcode( addr, flags );
    amd64_load_sp_reg();
   LABEL_C;
    /* done */
    return;

    case F_SIZEOF_STRING:
      {
      LABELS();
      ins_debug_instr_prologue(b, 0, 0);
      amd64_load_sp_reg();
      mov_mem8_reg( sp_reg, SVAL(-1).type, P_REG_RAX );
      cmp_reg32_imm( P_REG_RAX, PIKE_T_STRING );
      jne( &label_A );
      mov_mem_reg( sp_reg, SVAL(-1).value, P_REG_RAX);
      mov_mem_reg( P_REG_RAX, OFFSETOF(pike_string,len), P_REG_RBX );
      /* dec refs */
      add_mem32_imm( P_REG_RAX, OFFSETOF(pike_string,refs),  -1);
      jnz( &label_B );
      /* no refs, call really_free_svalue */
      add_reg_imm_reg( sp_reg, SVAL(-1).off, ARG1_REG );
      amd64_call_c_function(really_free_svalue);
      jmp(&label_B);
     LABEL_A;
        /* not a string. */
       amd64_call_c_opcode( addr, flags );
       jmp(&label_C);
     LABEL_B;
      /* save size on stack. */
       mov_imm_mem( PIKE_T_INT, sp_reg, SVAL(-1).type );
       mov_reg_mem( P_REG_RBX, sp_reg, SVAL(-1).value );
     LABEL_C;
      }
      return;
  case F_SIZEOF:
    {
      LABELS();
      ins_debug_instr_prologue(b, 0, 0);
      amd64_load_sp_reg();
      add_reg_imm_reg( sp_reg, -sizeof(struct svalue), P_REG_RBX);
      mov_sval_type( P_REG_RBX, P_REG_RAX );
      /* type in RAX, svalue in ARG1 */
      cmp_reg32_imm( P_REG_RAX, PIKE_T_ARRAY ); jne( &label_A );
      /* It's an array */
      mov_mem_reg( P_REG_RBX, OFFSETOF(svalue, u.array ), ARG1_REG);
      /* load size -> RAX*/
      mov_mem32_reg( ARG1_REG,OFFSETOF(array, size), P_REG_RAX );
      jmp( &label_C );

     LABEL_A;
      cmp_reg32_imm( P_REG_RAX, PIKE_T_STRING );  jne( &label_B );
      /* It's a string */
      mov_mem_reg( P_REG_RBX, OFFSETOF(svalue, u.string ), ARG1_REG);
      /* load size ->RAX*/
      mov_mem32_reg( ARG1_REG,OFFSETOF(pike_string, len ), P_REG_RAX );
      jmp( &label_C );
     LABEL_B;
      /* It's something else, svalue in RBX. */
      mov_reg_reg( P_REG_RBX, ARG1_REG );
      amd64_call_c_function( pike_sizeof );

     LABEL_C;/* all done, res in RAX */
      /* free value, store result */
      push( P_REG_RAX );
      sub_reg_imm(P_REG_RSP, 8);	/* Align on 16 bytes. */
      amd64_free_svalue( P_REG_RBX, 0 );
      add_reg_imm(P_REG_RSP, 8);
      pop( P_REG_RAX );
      mov_reg_mem(P_REG_RAX,    P_REG_RBX, OFFSETOF(svalue, u.integer));
      mov_imm_mem(PIKE_T_INT, P_REG_RBX, OFFSETOF(svalue, tu.t.type));
    }
    return;

  case F_POP_VALUE:
    {
      ins_debug_instr_prologue(b, 0, 0);
      amd64_load_sp_reg();
      amd64_add_sp( -1 );
      amd64_free_svalue( sp_reg, 0 );
    }
    return;

  case F_CATCH:
  case F_CATCH_AT:
    {
      INT32 base_addr = 0;

      LABELS();
      mov_rip_imm_reg(0, ARG1_REG);	/* Address for the POINTER. */
      base_addr = PIKE_PC;

      if ((b + F_OFFSET) == F_CATCH) {
	amd64_call_c_function (setup_catch_context);
      } else {
	amd64_call_c_function (setup_catch_at_context);
      }
      mov_reg_reg(P_REG_RAX, P_REG_RBX);

      /* Pass a pointer to Pike_interpreter.catch_ctx.recovery.recovery to
	 LOW_SET_JMP. */
      mov_mem_reg (Pike_interpreter_reg,
		   OFFSETOF(Pike_interpreter_struct, catch_ctx),
		   ARG1_REG);
      add_reg_imm_reg (ARG1_REG,
		       OFFSETOF(catch_context, recovery) +
		       OFFSETOF(JMP_BUF, recovery),
		       ARG1_REG);

      /* Copy the pointer to Pike_interpreter.catching_eval_jmpbuf
	 too, for compliance with F_CATCH conventions.
	 FIXME: Just ignore catching_eval_jmpbuf in OPCODE_INLINE_CATCH mode? */
      mov_reg_mem (ARG1_REG,
		   Pike_interpreter_reg,
		   OFFSETOF(Pike_interpreter_struct, catching_eval_jmpbuf));

      /* Now, call the actual setjmp function. */
      amd64_call_c_function (LOW_SETJMP_FUNC);

      test_reg (P_REG_RAX);
      jnz (&label_A);

      jmp (&label_B);

      LABEL_A; // Got exception

      amd64_call_c_function (handle_caught_exception);
      mov_reg_reg(P_REG_RAX, P_REG_RBX);

      /* Restore catching_eval_jmpbuf from catch_ctx->recovery.
         FIXME: Just ignore catching_eval_jmpbuf in OPCODE_INLINE_CATCH mode? */
      mov_mem_reg (Pike_interpreter_reg,
		   OFFSETOF(Pike_interpreter_struct, catch_ctx),
		   P_REG_RAX);

      test_reg (P_REG_RAX);
      jz (&label_B); /* Pike_interpreter.catch_ctx == 0 */

      add_reg_imm_reg (P_REG_RAX,
		       OFFSETOF(catch_context, recovery),
		       P_REG_RAX);

      mov_mem_reg (Pike_interpreter_reg,
		   OFFSETOF(Pike_interpreter_struct, recoveries),
		   P_REG_RCX);

      cmp_reg_reg(P_REG_RAX, P_REG_RCX);
      jne (&label_B); /* Pike_interpreter.recoveries !=
			 Pike_interpreter.catch_ctx->recovery */
      add_reg_imm_reg (P_REG_RAX,
		       OFFSETOF(JMP_BUF, recovery),
		       P_REG_RAX);
      mov_reg_mem (P_REG_RAX,
		   Pike_interpreter_reg,
		   OFFSETOF(Pike_interpreter_struct, catching_eval_jmpbuf));
      jmp (&label_C);
      LABEL_B;
      /* Zero Pike_interpreter.catching_eval_jmpbuf. */
      mov_imm_mem(0,
		  Pike_interpreter_reg,
		  OFFSETOF(Pike_interpreter_struct, catching_eval_jmpbuf));

      LABEL_C;
      /* RBX now contains either the address returned from
	 setup_catch_context, or the one from
	 handle_caught_exception. */
      jmp_reg(P_REG_RBX);

      upd_pointer(base_addr - 4, PIKE_PC - base_addr);
    }
    return;

    /* sp-1 = undefinedp(sp-1) */
  case F_UNDEFINEDP:
    {
    LABELS();
    ins_debug_instr_prologue(b, 0, 0);
    amd64_load_sp_reg();

    mov_mem32_reg( sp_reg, SVAL(-1).type, P_REG_RAX );
    cmp_reg32_imm( P_REG_RAX, 1<<16 | PIKE_T_INT );
    jne( &label_A );
    mov_imm_mem( 1, sp_reg,SVAL(-1).value);
    jmp( &label_B );
  LABEL_A;
    amd64_free_svalue_off( sp_reg, SVAL(-1).off,0 );
    mov_imm_mem( 0, sp_reg,SVAL(-1).value);
  LABEL_B;
    mov_imm_mem( PIKE_T_INT, sp_reg,SVAL(-1).type);
    }
    return;

  case F_ZERO_TYPE:
    {
      LABELS();
      ins_debug_instr_prologue(b, 0, 0);
      amd64_load_sp_reg();
      mov_mem32_reg( sp_reg, -sizeof(struct svalue), P_REG_RAX );
      /* Rax now has type + subtype. */
      mov_reg32_reg( P_REG_RAX, P_REG_RBX );
      and_reg32_imm( P_REG_RAX, 0x1f );
      cmp_reg32_imm( P_REG_RAX, PIKE_T_INT );
      jne( &label_A );
      /* It is an integer. */
      shr_reg_imm( P_REG_RBX, 16 );
      /* subtype in RBX. */
      mov_imm_mem( PIKE_T_INT, sp_reg, -sizeof(struct svalue) );
      mov_reg_mem( P_REG_RBX,  sp_reg,
                   -sizeof(struct svalue)+OFFSETOF(svalue,u.integer) );
      jmp( &label_B );
      LABEL_A;
      /* not an integer. Use C version for simplicitly.. */
      amd64_call_c_opcode( addr, flags );
      LABEL_B;
    }
    return;

  case F_EQ:
  case F_NE:
    /* Note: We can do better when doing equality comparison
       without much work. */
  case F_LE:
  case F_GE:
  case F_LT:
  case F_GT:
    {
      LABELS();
      ins_debug_instr_prologue(b, 0, 0);
      if_not_two_int(&label_A,1);
      amd64_add_sp(-1);

      clear_reg(P_REG_RCX);
      cmp_reg_reg(P_REG_RBX, P_REG_RAX);

      switch(b+F_OFFSET)
      {
      case F_GT: set_if_gt( P_REG_RCX);	break;
      case F_LT: set_if_lt( P_REG_RCX);	break;
      case F_LE: set_if_lte(P_REG_RCX );break;
      case F_GE: set_if_gte(P_REG_RCX ); break;
      case F_EQ: set_if_eq( P_REG_RCX ); break;
      case F_NE: set_if_neq(P_REG_RCX ); break;
      }

      mov_imm_mem(PIKE_T_INT, sp_reg, SVAL(-1).type );
      mov_reg_mem(P_REG_RCX, sp_reg, SVAL(-1).value );
      jmp(&label_D);

      LABEL_A;
      /* not an integer. Use C version for simplicitly.. */
      amd64_call_c_opcode( addr, flags );
      amd64_load_sp_reg();
      LABEL_D;
    }
    return;
  case F_ADD:
    {
    addr = f_add;
    update_arg1(2);
    amd64_call_c_opcode(addr, flags);
    amd64_load_sp_reg();
    }
    return;

  case F_SUBTRACT:
    {
    LABELS();
    ins_debug_instr_prologue(b, 0, 0);
    amd64_load_sp_reg();
    mov_mem8_reg(sp_reg, -1*sizeof(struct svalue), P_REG_RAX );
    mov_mem8_reg(sp_reg, -2*sizeof(struct svalue), P_REG_RBX );
    add_reg32_reg32(P_REG_RAX,P_REG_RBX);
    /* test_reg32(P_REG_RAX);  int == 0 */
    jnz(&label_A);

#ifdef PIKE_DEBUG
    if( PIKE_T_INT )
      Pike_fatal("Assertion failed\n");
#endif
    mov_mem_reg(sp_reg, -1*sizeof(struct svalue)+OFFSETOF(svalue,u.integer),
		P_REG_RAX );
    mov_mem_reg(sp_reg, -2*sizeof(struct svalue)+OFFSETOF(svalue,u.integer),
		P_REG_RCX );
    sub_reg_reg(P_REG_RCX,P_REG_RAX);
    jno(&label_B);
  LABEL_A;
    amd64_call_c_opcode(o_subtract, flags);
    amd64_load_sp_reg();
    jmp(&label_C);
  LABEL_B;
    mov_reg_mem( P_REG_RCX, sp_reg,
	       -2*sizeof(struct svalue)+OFFSETOF(svalue,u.integer));
    amd64_add_sp(-1);
  LABEL_C;
    }
    return;

  case F_UNDEFINED:
    ins_debug_instr_prologue(b, 0, 0);
    amd64_push_int(0, 1);
    return;
  case F_CONST0:
    ins_debug_instr_prologue(b, 0, 0);
    amd64_push_int(0, 0);
    return;
  case F_CONST1:
    ins_debug_instr_prologue(b, 0, 0);
    amd64_push_int(1, 0);
    return;
  case F_CONST_1:
    ins_debug_instr_prologue(b, 0, 0);
    amd64_push_int(-1, 0);
    return;
  case F_BIGNUM:
    ins_debug_instr_prologue(b, 0, 0);
    amd64_push_int(0x7fffffff, 0);
    return;
  case F_RETURN_1:
    ins_f_byte(F_CONST1);
    ins_f_byte(F_RETURN);
    return;
  case F_RETURN_0:
    ins_f_byte(F_CONST0);
    ins_f_byte(F_RETURN);
    return;
  case F_MARK:
  case F_SYNCH_MARK:
    ins_debug_instr_prologue(b, 0, 0);
    amd64_mark(0);
    return;
  case F_MARK2:
    ins_f_byte(F_MARK);
    ins_f_byte(F_MARK);
    return;
  case F_MARK_AND_CONST0:
    ins_f_byte(F_MARK);
    ins_f_byte(F_CONST0);
    return;
  case F_MARK_AND_CONST1:
    ins_f_byte(F_MARK);
    ins_f_byte(F_CONST1);
    return;
  case F_POP_MARK:
    ins_debug_instr_prologue(b, 0, 0);
    amd64_pop_mark();
    return;
  case F_POP_TO_MARK:
    ins_debug_instr_prologue(b, 0, 0);
    amd64_load_mark_sp_reg();
    amd64_load_sp_reg();
    amd64_pop_mark();
    mov_mem_reg(mark_sp_reg, 0, P_REG_RBX);
    jmp(&label_A);
    LABEL_B;
    amd64_add_sp( -1 );
    amd64_free_svalue( sp_reg, 0 );
    amd64_load_sp_reg();
    LABEL_A;
    cmp_reg_reg(P_REG_RBX, sp_reg);
    jl(&label_B);
    return;
    /* If we are compiling with debug, F_RETURN does extra checks */

  case F_RETURN:
  case F_DUMB_RETURN:
    {
    LABELS();
    ins_debug_instr_prologue(b, 0, 0);
    amd64_load_fp_reg();
    /* Note: really mem16, but we & with PIKE_FRAME_RETURN_INTERNAL anyway */
    mov_mem32_reg( fp_reg, OFFSETOF(pike_frame, flags), P_REG_RAX );
    and_reg32_imm( P_REG_RAX, PIKE_FRAME_RETURN_INTERNAL);
    jnz( &label_A );
    /* So, it is just a normal return. */
    LABEL_B;
    /* Actually return */
    flush_dirty_regs();
    amd64_return_from_function();
    /* */
    LABEL_A;
    /* We should jump to the given address. */
    mov_mem32_reg( fp_reg, OFFSETOF(pike_frame, flags), P_REG_RAX );
    and_reg32_imm( P_REG_RAX, PIKE_FRAME_RETURN_POP );
    jnz( &label_C );
    amd64_call_c_function( low_return );
    jmp( &label_D );

    LABEL_C;
    amd64_call_c_function( low_return_pop );

    LABEL_D;
#ifdef MACHINE_CODE_STACK_FRAMES
    /* We know that this is a RETURN_INTERNAL frame, so we must pop
       the C stack. */
    amd64_pop_internal_c_frame();
#endif /* MACHINE_CODE_STACK_FRAMES */
    fp_reg = -1;
    amd64_load_fp_reg();
    mov_mem_reg( fp_reg, OFFSETOF(pike_frame, return_addr), P_REG_RAX );
    jmp_reg( P_REG_RAX );
    }
    return;

  case F_CLEAR_STRING_SUBTYPE:
    ins_debug_instr_prologue(b, 0, 0);
    amd64_load_sp_reg();
    mov_mem32_reg(sp_reg, OFFSETOF(svalue, tu.t.type) - sizeof(struct svalue),
		  P_REG_RAX);
    /* NB: We only care about subtype 1! */
    cmp_reg32_imm(P_REG_RAX, (1<<16)|PIKE_T_STRING);
    jne(&label_A);
    and_reg32_imm(P_REG_RAX, 0x1f);
    mov_reg_mem32(P_REG_RAX,
		  sp_reg, OFFSETOF(svalue, tu.t.type) - sizeof(struct svalue));
    LABEL_A;
    return;
  }

  if (instrs[b].flags & I_RETURN) {
#ifdef MACHINE_CODE_STACK_FRAMES
    amd64_load_fp_reg();
    /* Load flags prior to calling the C opcode (which typically pops
       the frame in I_RETURN instructions. Note: really mem16, but we
       & with PIKE_FRAME_RETURN_INTERNAL below. */
    mov_mem32_reg(fp_reg, OFFSETOF(pike_frame, flags), P_REG_RBX);
#endif /* MACHINE_CODE_STACK_FRAMES */
  }

  amd64_call_c_opcode(addr,flags);

  if (instrs[b].flags & I_RETURN) {
    LABELS();

    if ((b + F_OFFSET) == F_RETURN_IF_TRUE) {
      /* Kludge. We must check if the ret addr is
       * PC + JUMP_EPILOGUE_SIZE. */
      mov_rip_imm_reg(JUMP_EPILOGUE_SIZE, P_REG_RCX);
    }
    cmp_reg_imm(P_REG_RAX, -1);
    jne(&label_A);
    amd64_return_from_function();
    LABEL_A;

    if ((b + F_OFFSET) == F_RETURN_IF_TRUE) {
      /* Kludge. We must check if the ret addr is
       * orig_addr + JUMP_EPILOGUE_SIZE. */
      cmp_reg_reg( P_REG_RAX, P_REG_RCX );
      je( &label_C );
    }

#ifdef MACHINE_CODE_STACK_FRAMES
    /* Check if we should pop C stack. The flags word of the frame
       prior to calling the C opcode is loaded into RBX above. */
    and_reg32_imm(P_REG_RBX, PIKE_FRAME_RETURN_INTERNAL);
    jz( &label_B );
    amd64_pop_internal_c_frame();

    LABEL_B;
#endif /* MACHINE_CODE_STACK_FRAMES */

    jmp_reg(P_REG_RAX);

    LABEL_C;
    return;
  }

  if (flags & I_JUMP) {
    jmp_reg(P_REG_RAX);
  }
}

int amd64_ins_f_jump(unsigned int op, int backward_jump)
{
  int flags;
  void *addr;
  int off = op - F_OFFSET;
  int ret = -1;
  LABELS();

#ifdef PIKE_DEBUG
  if(off > MAX_SUPPORTED_INSTR)
    Pike_error("Instruction too big %d\n",off);
#endif
  flags = instrs[off].flags;
  if (!(flags & I_BRANCH)) return -1;

#define START_JUMP() do{                                              \
    ins_debug_instr_prologue(off, 0, 0);                              \
    if (backward_jump) {                                              \
      maybe_update_pc();                                              \
      amd64_ins_branch_check_threads_etc(0);                          \
    }                                                                 \
  } while(0)

  switch( op )
  {
    case F_QUICK_BRANCH_WHEN_ZERO:
    case F_QUICK_BRANCH_WHEN_NON_ZERO:
      START_JUMP();
      amd64_load_sp_reg();
      amd64_add_sp( -1 );
      mov_mem_reg( sp_reg, 8, P_REG_RAX );
      test_reg32(P_REG_RAX);
      if( op == F_QUICK_BRANCH_WHEN_ZERO )
        return jz_imm_rel32(0);
      return jnz_imm_rel32(0);

    case F_BRANCH_WHEN_ZERO:
    case F_BRANCH_WHEN_NON_ZERO:
      START_JUMP();
      amd64_load_sp_reg();
      mov_mem8_reg( sp_reg, -sizeof(struct svalue),  P_REG_RCX );
      cmp_reg32_imm( P_REG_RCX, PIKE_T_INT ); je( &label_C );
      mov_imm_reg( 1, P_REG_RAX );
      shl_reg32_reg( P_REG_RAX, P_REG_RCX );
      and_reg32_imm( P_REG_RAX, BIT_FUNCTION|BIT_OBJECT );
      jnz( &label_A );

      /* string, array, mapping or float. Always true */
      amd64_add_sp(-1);
      amd64_free_svalue_type( sp_reg, P_REG_RCX, 0 );
      mov_imm_reg( 1, P_REG_RBX );
      jmp( &label_B );

     LABEL_A;
      /* function or object. Use svalue_is_true. */
      add_reg_imm_reg(sp_reg, -sizeof(struct svalue), ARG1_REG );
      amd64_call_c_function(svalue_is_true);
      amd64_load_sp_reg();
      mov_reg_reg( P_REG_RAX, P_REG_RBX );
      amd64_add_sp( -1 );
      amd64_free_svalue( sp_reg, 0 ); /* Pop the stack. */
      jmp( &label_B );

     LABEL_C;
      /* integer */
      mov_mem_reg( sp_reg,
                   -sizeof(struct svalue)+OFFSETOF(svalue,u.integer),
                   P_REG_RBX );
      amd64_add_sp(-1);

     LABEL_B; /* Branch or not? */
      test_reg( P_REG_RBX );
      if( op == F_BRANCH_WHEN_ZERO )
        return jz_imm_rel32(0);
      return jnz_imm_rel32(0);

    case F_FOREACH:
      START_JUMP();
      /* -4: array
         -3: lvalue[0]
         -2: lvalue[1]
         -1: counter
      */
      amd64_load_sp_reg();
      mov_mem8_reg( sp_reg, -4*sizeof(struct svalue), P_REG_RBX );
      cmp_reg32_imm( P_REG_RBX, PIKE_T_ARRAY );
      je(&label_D);
      /* Bad arg 1. Let the C opcode throw the error. */
      amd64_call_c_opcode(instrs[off].address, flags);
      /* NOT_REACHED */

     LABEL_D;
      mov_mem_reg( sp_reg, -1*sizeof(struct svalue)+8, P_REG_RAX );
      mov_mem_reg( sp_reg, -4*sizeof(struct svalue)+8, P_REG_RBX );
      mov_mem32_reg( P_REG_RBX, OFFSETOF(array,size), P_REG_RCX );
      cmp_reg_reg( P_REG_RAX, P_REG_RCX );
      jge(&label_A);

      /* increase counter */
      add_mem_imm( sp_reg, -1*(int)sizeof(struct svalue)+8, 1 );

      /* get item */
      mov_mem_reg( P_REG_RBX, OFFSETOF(array,item), P_REG_RBX );
      shl_reg_imm( P_REG_RAX, 4 );
      add_reg_reg( P_REG_RBX, P_REG_RAX );

      mov_mem8_reg( sp_reg, -3*sizeof(struct svalue), P_REG_RAX );
      cmp_reg_imm( P_REG_RAX,T_SVALUE_PTR);
      jne( &label_C );

      /* SVALUE_PTR optimization */
      mov_mem_reg( sp_reg, -3*sizeof(struct svalue)+8, P_REG_RDX );
      push( P_REG_RDX );
      sub_reg_imm(P_REG_RSP, 8);	/* Align on 16 bytes. */
      /* Free old value. */
      amd64_free_svalue( P_REG_RDX, 0 );
      add_reg_imm(P_REG_RSP, 8);
      pop( P_REG_RDX );

      /* Assign new value. */
      mov_mem_reg( P_REG_RBX, 0, P_REG_RAX );
      mov_mem_reg( P_REG_RBX, 8, P_REG_RCX );
      mov_reg_mem( P_REG_RAX, P_REG_RDX, 0 );
      mov_reg_mem( P_REG_RCX, P_REG_RDX, 8 );

      /* inc refs? */
      and_reg32_imm( P_REG_RAX, MIN_REF_TYPE );
      jz( &label_B );
      add_imm_mem( 1, P_REG_RCX, OFFSETOF(pike_string, refs));
      jmp( &label_B );

     LABEL_C;
      add_reg_imm_reg( sp_reg, -3*sizeof(struct svalue), ARG1_REG );
      mov_reg_reg( P_REG_RBX, ARG2_REG );
      amd64_call_c_function( assign_lvalue );
      jmp(&label_B);

     LABEL_A;
      mov_imm_reg( 0, P_REG_RBX );
     LABEL_B;
      test_reg(P_REG_RBX);
      return jnz_imm_rel32(0);

    case F_LOOP:
      START_JUMP();
      /* counter in pike_sp-1 */
      /* decrement until 0. */
      /* if not 0, branch */
      /* otherwise, pop */
      amd64_load_sp_reg();
      mov_mem32_reg( sp_reg, SVAL(-1).type, P_REG_RAX );
      /* Is it a normal integer? subtype -> 0, type -> PIKE_T_INT */
      cmp_reg32_imm( P_REG_RAX, PIKE_T_INT );
      jne( &label_A );

      /* if it is, is it <= 0? */
      mov_mem_reg( sp_reg, SVAL(-1).value, P_REG_RAX );
      cmp_reg_imm(P_REG_RAX,0);
      jle(&label_B); /* it is. */

      /* add_reg_imm( P_REG_RAX, -1 ); */
      /* mov_reg_mem( P_REG_RAX, sp_reg, -sizeof(struct svalue)+8); */
      add_mem_imm( sp_reg, SVAL(-1).value, -1 );
      /* REG_RAX known true, so no need to set it to 1. */
      /* mov_imm_reg( 1, P_REG_RAX ); */
      /* decremented. Jump -> true. */

      /* This is where we would really like to have two instances of
       * the target returned from this function...
       */
      jmp( &label_C );

      LABEL_A; /* Not an integer. */
      amd64_call_c_opcode(instrs[F_LOOP-F_OFFSET].address,
                          instrs[F_LOOP-F_OFFSET].flags );
      amd64_load_sp_reg();
      jmp( &label_C );

      /* result in RAX */
      LABEL_B; /* loop done, inline. Known to be int, and 0 */
      amd64_add_sp( -1 );
      mov_imm_reg(0, P_REG_RAX );

      LABEL_C; /* Branch or not? */
      test_reg( P_REG_RAX );
      return jnz_imm_rel32(0);

    case F_BRANCH_WHEN_EQ: /* sp[-2] != sp[-1] */
    case F_BRANCH_WHEN_NE: /* sp[-2] != sp[-1] */
/*      START_JUMP();*/
      ins_debug_instr_prologue(op, 0, 0);
      amd64_load_sp_reg();
      mov_mem8_reg( sp_reg, -sizeof(struct svalue),  P_REG_RCX );
      mov_mem8_reg( sp_reg, -sizeof(struct svalue)*2,P_REG_RBX );
      cmp_reg_reg( P_REG_RCX, P_REG_RBX );
      jnz( &label_A ); /* Types differ */

      /* Fallback to C for functions, objects and floats. */
      mov_imm_reg(1, P_REG_RBX);
      shl_reg32_reg(P_REG_RBX, P_REG_RCX);
      and_reg_imm(P_REG_RBX, (BIT_FUNCTION|BIT_OBJECT|BIT_FLOAT));
      jnz( &label_A );

      mov_mem_reg( sp_reg, -sizeof(struct svalue)+8,  P_REG_RBX );
      sub_reg_mem( P_REG_RBX, sp_reg, -sizeof(struct svalue)*2+8);
      /* RBX will now be 0 if they are equal.*/

      /* Optimization: The types are equal, pop_stack can be greatly
       * simplified if they are <= max_ref_type */
      cmp_reg32_imm( P_REG_RCX,MIN_REF_TYPE);
      jge( &label_B );
      /* cheap pop. We know that both are > max_ref_type */
      amd64_add_sp( -2 );
      jmp( &label_D );

     LABEL_A; /* Fallback - call opcode. */
      amd64_call_c_opcode( instrs[F_BRANCH_WHEN_NE-F_OFFSET].address,
                           instrs[F_BRANCH_WHEN_NE-F_OFFSET].flags );
      amd64_load_sp_reg();
      /* Opcode returns 0 if equal, -1 if not. */
      mov_reg_reg(P_REG_RAX, P_REG_RBX);
      jmp(&label_D);

    LABEL_B; /* comparison done, pop stack x2, reftypes */
     amd64_add_sp( -2 );

     mov_mem_reg( sp_reg, OFFSETOF(svalue,u.refs),  P_REG_RAX );
     add_mem32_imm( P_REG_RAX, OFFSETOF(pike_string,refs), -1 );
     jnz( &label_C );
     /* Free sp_reg */
     mov_reg_reg( sp_reg, ARG1_REG );
     amd64_call_c_function( really_free_svalue );
     amd64_load_sp_reg();
    LABEL_C;
     add_reg_imm_reg( sp_reg, sizeof(struct svalue), ARG1_REG );
     mov_mem_reg( ARG1_REG, OFFSETOF(svalue,u.refs),  P_REG_RAX );
     add_mem32_imm( P_REG_RAX, OFFSETOF(pike_string,refs), -1 );
     jnz( &label_D );
     amd64_call_c_function( really_free_svalue );
     /* free sp[-2] */
    LABEL_D;
     test_reg(P_REG_RBX);
      if( op == F_BRANCH_WHEN_EQ )
        return jz_imm_rel32(0);
      return jnz_imm_rel32(0);


#if 0
    case F_BRANCH_WHEN_LT: /* sp[-2] < sp[-1] */
    case F_BRANCH_WHEN_GE: /* sp[-2] >= sp[-1] */

    case F_BRANCH_WHEN_GT: /* sp[-2] > sp[-1] */
    case F_BRANCH_WHEN_LE: /* sp[-2] <= sp[-1] */
#endif
    case F_BRANCH:
      START_JUMP();
      add_to_program(0xe9);
      ret=(INT32) PIKE_PC;
      PUSH_INT(0);
      return ret;
  }

  maybe_update_pc();
  addr=instrs[off].address;
  amd64_call_c_opcode(addr, flags);

  amd64_load_sp_reg();
  test_reg(P_REG_RAX);

  if (backward_jump) {
    INT32 skip;
    add_to_program (0x74);	/* jz rel8 */
    add_to_program (0);		/* Bytes to skip. */
    skip = (INT32)PIKE_PC;
    amd64_ins_branch_check_threads_etc(0);
    add_to_program (0xe9);	/* jmp rel32 */
    ret = (INT32) PIKE_PC;
    PUSH_INT (0);
    /* Adjust the skip for the relative jump. */
    Pike_compiler->new_program->program[skip-1] = ((INT32)PIKE_PC - skip);
  }
  else {
    add_to_program (0x0f);	/* jnz rel32 */
    add_to_program (0x85);
    ret = (INT32) PIKE_PC;
    PUSH_INT (0);
  }

  return ret;
}

void ins_f_byte_with_arg(unsigned int a, INT32 b)
{
  maybe_update_pc();
  switch(a) {
  case F_DUP:
    ins_debug_instr_prologue(b, 0, 0);
    amd64_load_sp_reg();
    add_reg_imm_reg(sp_reg, -sizeof(struct svalue) * (b + 1), P_REG_R10 );
    amd64_push_svaluep( P_REG_R10 );
    return;

  case F_SWAP:
    /*
     * swap(pike_sp - 1, pike_sp - (arg1 + 2))
     *
     * NB: *KNOWS* that sizeof(struct svalue) == 16.
     */
    ins_debug_instr_prologue(b, 0, 0);
    amd64_load_sp_reg();
    add_reg_imm_reg( sp_reg, -(b + 2)*sizeof(struct svalue), P_REG_RCX );
    mov_mem128_reg( P_REG_RCX, 0, P_REG_XMM0 );
    mov_mem128_reg( P_REG_RCX, (b + 1)*16, P_REG_XMM1 );
    mov_reg_mem128( P_REG_XMM1, P_REG_RCX, 0 );
    mov_reg_mem128( P_REG_XMM0, P_REG_RCX, (b + 1)*16 );
    return;

  case F_THIS_OBJECT:
    if( b == 0 )
    {
      ins_debug_instr_prologue(a-F_OFFSET, b, 0);
      amd64_push_this_object();
      return;
    }
    break; /* Fallback to C-version. */
  case F_POP_N_ELEMS:
    {
      int i;
      LABELS();

      ins_debug_instr_prologue(a-F_OFFSET,b, 0);
      amd64_load_sp_reg();
      if( b < 3 )
      {
	for(i=0;i<b;i++)
	  amd64_free_svalue_off( sp_reg, SVAL(-1-i).off, 0 );
	amd64_add_sp( -b );
	return;
      }
      break; /* use c version. */

    /* FIXME: Why does not this work? */
/*      add_reg_imm_reg( sp_reg, -SVAL(b).off, P_REG_RBX ); */

/*     LABEL_A; */
/*      amd64_free_svalue_off( sp_reg, SVAL(-1).off,0 ); */
/*      amd64_add_sp( -1 ); */
/*      cmp_reg_reg(P_REG_RBX,sp_reg); */
/*      jne(&label_A); */
    }
    return;
  case F_RETURN_LOCAL:
    /* FIXME: The C version has a trick:
       if locals+b < expendibles, pop to there
       and return.

       This saves a push, and the poping has to be done anyway.
    */
    ins_f_byte_with_arg( F_LOCAL, b );
    ins_f_byte( F_DUMB_RETURN );
    return;

  case F_NEG_INT_INDEX:
    {
      LABELS();
      ins_debug_instr_prologue(a-F_OFFSET, b, 0);
      amd64_load_sp_reg();
      mov_mem8_reg( sp_reg, -1*sizeof(struct svalue),  P_REG_RAX );
      cmp_reg32_imm( P_REG_RAX, PIKE_T_ARRAY );
      jne( &label_A );

      mov_mem_reg( sp_reg,  -1*sizeof(struct svalue)+8, P_REG_RDX ); /* u.array */
      /* -> arr[sizeof(arr)-b] */
      mov_mem32_reg( P_REG_RDX, OFFSETOF(array,size), P_REG_RBX );
      sub_reg_imm( P_REG_RBX, b );
      js( &label_A ); /* if signed result, index outside array */

      shl_reg_imm( P_REG_RBX, 4 );
      add_reg_mem( P_REG_RBX, P_REG_RDX, OFFSETOF(array,item) );

      /* This overwrites the array. */
      amd64_push_svaluep_to( P_REG_RBX, -1 );

      /* We know it's an array. */
      add_mem32_imm( P_REG_RDX, OFFSETOF(array,refs),  -1);
      jnz( &label_C );
      mov_reg_reg( P_REG_RDX, ARG1_REG );
      amd64_call_c_function(really_free_array);
      jmp( &label_C );

     LABEL_A;
       update_arg1( b );
       amd64_call_c_opcode(instrs[a-F_OFFSET].address,
                           instrs[a-F_OFFSET].flags);
     LABEL_C;
    }
    return;

  case F_POS_INT_INDEX:
    {
      LABELS();
      ins_debug_instr_prologue(a-F_OFFSET, b, 0);
      amd64_load_sp_reg();
      mov_mem8_reg( sp_reg, -1*sizeof(struct svalue),  P_REG_RAX );
      cmp_reg32_imm( P_REG_RAX, PIKE_T_ARRAY );
      jne( &label_A );

      mov_mem_reg( sp_reg,  -1*sizeof(struct svalue)+8, P_REG_RDX ); /* u.array */
      /* -> arr[sizeof(arr)-b] */
      mov_mem32_reg( P_REG_RDX, OFFSETOF(array,size), P_REG_RCX );
      cmp_reg32_imm( P_REG_RCX, b );
      jle( &label_A ); /* RCX <= b, index outside array */
      mov_imm_reg( b * sizeof(struct svalue), P_REG_RBX);
      add_reg_mem( P_REG_RBX, P_REG_RDX, OFFSETOF(array,item) );

      /* This overwrites the array. */
      amd64_push_svaluep_to( P_REG_RBX, -1 );

      /* We know it's an array. */
      add_mem32_imm( P_REG_RDX, OFFSETOF(array,refs),  -1);
      jnz( &label_C );
      mov_reg_reg( P_REG_RDX, ARG1_REG );
      amd64_call_c_function(really_free_array);
      jmp( &label_C );

     LABEL_A;
       update_arg1( b );
       amd64_call_c_opcode(instrs[a-F_OFFSET].address,
                           instrs[a-F_OFFSET].flags);
     LABEL_C;
    }
    return;
  case F_LOCAL_INDEX:
    {
      LABELS();
      /*
        local[pike_sp[-1]]
      */
      ins_debug_instr_prologue(a-F_OFFSET, b, 0);
      amd64_load_sp_reg();
      amd64_load_fp_reg();

      mov_mem_reg( fp_reg, OFFSETOF(pike_frame,locals), P_REG_RDX);
      add_reg_imm( P_REG_RDX, b*sizeof(struct svalue));
      mov_mem8_reg( P_REG_RDX, 0, P_REG_RAX );
      mov_mem8_reg( sp_reg, -1*sizeof(struct svalue), P_REG_RBX );
      shl_reg_imm( P_REG_RAX, 8 );
      add_reg_reg( P_REG_RAX, P_REG_RBX );
      mov_mem_reg( sp_reg, -1*sizeof(struct svalue)+8, P_REG_RBX ); /* int */
      mov_mem_reg( P_REG_RDX, 8, P_REG_RCX );                         /* value */
      cmp_reg32_imm( P_REG_RAX, (PIKE_T_ARRAY<<8)|PIKE_T_INT );
      jne( &label_A );

      /* Array and int index. */
      mov_mem32_reg( P_REG_RCX, OFFSETOF(array,size), P_REG_RDX );
      cmp_reg32_imm( P_REG_RBX, 0 ); jge( &label_D );
      /* less than 0, add size */
      add_reg_reg( P_REG_RBX, P_REG_RDX );

      LABEL_D;
      cmp_reg32_imm( P_REG_RBX, 0 ); jl( &label_B ); // <0
      cmp_reg_reg( P_REG_RBX, P_REG_RCX); jge( &label_B ); // >=size

      /* array, index inside array. push item, swap, pop, done */
      mov_mem_reg( P_REG_RCX, OFFSETOF(array,item), P_REG_RCX );
      shl_reg_imm( P_REG_RBX, 4 );
      add_reg_reg( P_REG_RBX, P_REG_RCX );
      amd64_push_svaluep_to( P_REG_RBX, -1 );
      jmp( &label_C );

      LABEL_A;
#if 0
      cmp_reg32_imm( P_REG_RAX, (PIKE_T_STRING<<8)|PIKE_T_INT );
      jne( &label_B );
#endif
      LABEL_B;
      /* something else. */
      update_arg1(b);
      amd64_call_c_opcode(instrs[a-F_OFFSET].address,
			  instrs[a-F_OFFSET].flags);
      LABEL_C;
      /* done */
    }
    return;
  case F_MULTIPLY_INT:
    {
      LABELS();
      ins_debug_instr_prologue(a-F_OFFSET, b, 0);
      amd64_load_sp_reg();
      mov_mem8_reg(sp_reg, SVAL(-1).type, P_REG_RAX );
      test_reg32(P_REG_RAX);
      jnz(&label_A);

      /* FIXME: mul_reg_mem_imm actually exists as an instruction. */
      mov_imm_reg(b, P_REG_RAX);
      mov_mem_reg(sp_reg, SVAL(-1).value, P_REG_RBX );
      mul_reg_reg(P_REG_RBX,P_REG_RAX);
      jo(&label_A);

      mov_imm_mem(PIKE_T_INT, sp_reg, SVAL(-1).type);
      mov_reg_mem(P_REG_RBX,  sp_reg, SVAL(-1).value);
      jmp(&label_B);
   LABEL_A;
     update_arg1(b);
     amd64_call_c_opcode(instrs[a-F_OFFSET].address,
			 instrs[a-F_OFFSET].flags);
   LABEL_B;
    }
    return;

  case F_MOD_INT:
    if( b < 0 )
      break;

    /* FALLTHRU */

  case F_DIVIDE_INT:
    {
      LABELS();
      ins_debug_instr_prologue(a-F_OFFSET, b, 0);
      amd64_load_sp_reg();

      mov_mem8_reg(sp_reg,SVAL(-1).type, P_REG_RAX);
      test_reg32(P_REG_RAX);
      jnz(&label_A);

      mov_mem_reg(sp_reg, SVAL(-1).value, P_REG_RAX);
      if( b == -1 )
      {
        /* 64bit imm not done. */
        cmp_reg_imm(P_REG_RAX,-0x80000000);
        jle(&label_A);
      }
      /*if( a == F_MOD_INT )*/
      {
        cmp_reg_imm(P_REG_RAX, 0);
        jl(&label_A);
      }

      mov_imm_reg(b,P_REG_RBX);
      div_reg_reg(P_REG_RAX,P_REG_RBX);
      mov_imm_mem(PIKE_T_INT, sp_reg, SVAL(-1).type);
      if( a == F_MOD_INT )
	mov_reg_mem(P_REG_RDX,  sp_reg, SVAL(-1).value);
      else
	mov_reg_mem(P_REG_RAX,  sp_reg, SVAL(-1).value);
      jmp(&label_B);
   LABEL_A;
    update_arg1(b);
    amd64_call_c_opcode(instrs[a-F_OFFSET].address,
			instrs[a-F_OFFSET].flags);
   LABEL_B;
    }
    return;

  case F_AND_INT:
    {
    LABELS();
    ins_debug_instr_prologue(a-F_OFFSET,b, 0);
    amd64_load_sp_reg();
    mov_mem8_reg(sp_reg, SVAL(-1).type, P_REG_RAX );
    test_reg32(P_REG_RAX);
    jnz(&label_A);

 /* FIXME: and_mem_imm */
    mov_mem_reg(sp_reg, SVAL(-1).value, P_REG_RAX );
    and_reg_imm(P_REG_RAX,b);
    mov_imm_mem(PIKE_T_INT,sp_reg,SVAL(-1).type);
    mov_reg_mem(P_REG_RAX,sp_reg,SVAL(-1).value);
    jmp(&label_B);
   LABEL_A;
    update_arg1(b);
    amd64_call_c_opcode(instrs[a-F_OFFSET].address,
			instrs[a-F_OFFSET].flags);
   LABEL_B;
    }
    return;

  case F_XOR_INT:
    {
    LABELS();
    amd64_load_sp_reg();
    ins_debug_instr_prologue(a-F_OFFSET,b,0);
    mov_mem8_reg(sp_reg,SVAL(-1).type, P_REG_RAX);
    test_reg32(P_REG_RAX);
    jnz(&label_A);

    mov_mem_reg(sp_reg,SVAL(-1).value, P_REG_RAX);
    mov_imm_reg(b,P_REG_RBX);
    xor_reg_reg(P_REG_RAX,P_REG_RBX);
    mov_imm_mem(PIKE_T_INT,sp_reg,SVAL(-1).type);
    mov_reg_mem(P_REG_RBX,sp_reg,SVAL(-1).value);
    jmp(&label_B);
   LABEL_A;
    update_arg1(b);
    amd64_call_c_opcode(instrs[a-F_OFFSET].address,
			instrs[a-F_OFFSET].flags);
   LABEL_B;
    }
    return;

  case F_OR_INT:
    if( !b )
      return;
   {
    LABELS();
    ins_debug_instr_prologue(a-F_OFFSET, b, 0);
    amd64_load_sp_reg();
    mov_mem8_reg(sp_reg, SVAL(-1).type, P_REG_RAX );
    test_reg32(P_REG_RAX);
    jnz(&label_A);

 /* FIXME: or_mem_imm */
    mov_mem_reg(sp_reg, SVAL(-1).value, P_REG_RAX );
    or_reg_imm(P_REG_RAX,b);
    mov_imm_mem(PIKE_T_INT,sp_reg,SVAL(-1).type);
    mov_reg_mem(P_REG_RAX,sp_reg,SVAL(-1).value);
    jmp(&label_B);
   LABEL_A;
    update_arg1(b);
    amd64_call_c_opcode(instrs[a-F_OFFSET].address,
			instrs[a-F_OFFSET].flags);
   LABEL_B;
    }
    return;

  case F_LSH_INT:
    if( b > 0 && b <= 63 )
    {
      LABELS();
      ins_debug_instr_prologue(a-F_OFFSET, b, 0);
      amd64_load_sp_reg();
      mov_mem8_reg(sp_reg, SVAL(-1).type, P_REG_RAX );
      test_reg32(P_REG_RAX);
      jz(&label_B);
    LABEL_A;
      update_arg1(b);
      amd64_call_c_opcode(instrs[a-F_OFFSET].address,
			  instrs[a-F_OFFSET].flags);
      jmp(&label_C);
    LABEL_B;
      mov_imm_mem(PIKE_T_INT,sp_reg,SVAL(-1).type);
      mov_mem_reg( sp_reg, SVAL(-1).value, P_REG_RBX);
    /* ok. It would have been nice is sal set a
       bit that stayed set when you shifted out a 1 bit. :)
    */
      mov_reg_reg( P_REG_RBX, P_REG_RAX );
      shl_reg_imm( P_REG_RBX, b);

      mov_reg_reg( P_REG_RBX, P_REG_RDX );
      shr_reg_imm( P_REG_RDX, b );
      cmp_reg_reg( P_REG_RDX, P_REG_RAX );
      jne( &label_A );
      mov_reg_mem( P_REG_RBX, sp_reg, SVAL(-1).value);
  LABEL_C;
     return;
    }
    if(!b) return;
    if( b < 0 )
      yyerror("<< with negative constant\n");
    break;
  case F_RSH_INT:
    if( b > 0 && b <= 63 )
    {
      LABELS();
      ins_debug_instr_prologue(a-F_OFFSET, b, 0);
      amd64_load_sp_reg();
      mov_mem8_reg(sp_reg, SVAL(-1).type, P_REG_RAX );
      test_reg32(P_REG_RAX);
      jnz(&label_A);
      mov_mem_reg( sp_reg, SVAL(-1).value, P_REG_RAX);
    /* FIXME: shr_mem_imm */
      mov_imm_mem(PIKE_T_INT,sp_reg,SVAL(-1).type);
      shr_reg_imm( P_REG_RAX, b);
      mov_reg_mem( P_REG_RAX, sp_reg, SVAL(-1).value);
      jmp(&label_B);
    LABEL_A;
      update_arg1(b);
      amd64_call_c_opcode(instrs[a-F_OFFSET].address,
			  instrs[a-F_OFFSET].flags);
    LABEL_B;
    return;
    }
    if(!b) return;
    if( b < 0 )
      yyerror(">> with negative constant\n");
    break;

  case F_SUBTRACT_INT:
  case F_ADD_NEG_INT:
    b = -b;

    /* FALLTHRU */

  case F_ADD_INT:
    {
      LABELS();
      ins_debug_instr_prologue(a-F_OFFSET, b, 0);
      amd64_load_sp_reg();
      mov_mem8_reg( sp_reg,SVAL(-1).type, P_REG_RAX );
      test_reg32(P_REG_RAX);
      jnz( &label_A );
      mov_mem_reg(sp_reg, SVAL(-1).value,P_REG_RAX );
      add_reg_imm( P_REG_RAX, b );
      jo( &label_A ); /* if overflow, use f_add */
      mov_imm_mem( PIKE_T_INT, sp_reg, SVAL(-1).type );
      mov_reg_mem( P_REG_RAX, sp_reg, SVAL(-1).value );
      jmp(&label_B); /* all done. */

      LABEL_A;
      amd64_push_int(b,0);
      update_arg1(2);
      amd64_call_c_opcode( f_add, I_UPDATE_SP );
      amd64_load_sp_reg();
      LABEL_B;
    }
    return;

  case F_NUMBER:
    ins_debug_instr_prologue(a-F_OFFSET, b, 0);
    amd64_push_int(b, 0);
    return;
  case F_NEG_NUMBER:
    ins_debug_instr_prologue(a-F_OFFSET, b, 0);
    amd64_push_int(-(INT64)b, 0);
    return;
  case F_STRING:
    ins_debug_instr_prologue(a-F_OFFSET, b, 0);
    amd64_push_string(b, 0);
    return;
  case F_ARROW_STRING:
    ins_debug_instr_prologue(a-F_OFFSET, b, 0);
    amd64_push_string(b, 1);
    return;
  case F_MARK_AND_CONST0:
    ins_f_byte(F_MARK);
    ins_f_byte(F_CONST0);
    return;
  case F_MARK_AND_CONST1:
    ins_f_byte(F_MARK);
    ins_f_byte(F_CONST0);
    return;
  case F_MARK_AND_STRING:
    ins_f_byte(F_MARK);
    ins_f_byte_with_arg(F_STRING, b);
    return;
  case F_MARK_AND_GLOBAL:
    ins_f_byte(F_MARK);
    ins_f_byte_with_arg(F_GLOBAL, b);
    return;
  case F_MARK_AND_LOCAL:
    ins_f_byte(F_MARK);
    ins_f_byte_with_arg(F_LOCAL, b);
    return;
  case F_MARK_X:
    ins_debug_instr_prologue(a-F_OFFSET, b, 0);
    amd64_mark(b);
    return;
  case F_LFUN:
    ins_debug_instr_prologue(a-F_OFFSET, b, 0);
    amd64_push_local_function(b);
    return;

  case F_ASSIGN_LOCAL:
    ins_debug_instr_prologue(a-F_OFFSET, b, 0);
    amd64_load_sp_reg();
    amd64_assign_local(b);
    add_reg_imm_reg(sp_reg, -sizeof(struct svalue), ARG1_REG);
    amd64_ref_svalue(ARG1_REG, 0);
    return;

  case F_ASSIGN_LOCAL_AND_POP:
    ins_debug_instr_prologue(a-F_OFFSET, b, 0);
    amd64_assign_local(b);
    amd64_add_sp(-1);
    return;


  case F_ASSIGN_PRIVATE_GLOBAL_AND_POP:
  case F_ASSIGN_PRIVATE_GLOBAL:
  {
    LABELS();
    ins_debug_instr_prologue(a-F_OFFSET, b, 0);
    amd64_get_storage( P_REG_RBX, b );
    /* do not assign if this object is destructed. */
    mov_mem_reg( fp_reg, OFFSETOF(pike_frame,current_object), ARG1_REG );
    mov_mem_reg( ARG1_REG, OFFSETOF(object,prog), P_REG_RAX );
    test_reg(P_REG_RAX);
    jnz(&label_A);
    /* Note: will throw error ('cannot index destructed object' or similar).
     * Note: Only arg1 is correct here (points to the destructed object)
     */
    amd64_call_c_function(object_low_set_index);
    LABEL_A;

    amd64_free_svalue( P_REG_RBX, 0 );
    amd64_load_sp_reg();

    if( a == F_ASSIGN_PRIVATE_GLOBAL_AND_POP )
    {
      amd64_add_sp(-1);
      amd64_assign_svalue_no_free( P_REG_RBX, sp_reg, 0);
    }
    else
    {
      amd64_assign_svalue_no_free( P_REG_RBX, sp_reg, -sizeof(struct svalue));
      amd64_ref_svalue(P_REG_RBX,0);
    }
  }
  return;

  case F_ASSIGN_GLOBAL:
  case F_ASSIGN_GLOBAL_AND_POP:
      /* arg1: pike_fp->current obj
         arg2: arg1+idenfier level
         arg3: Pike_sp-1
      */
    /* NOTE: We cannot simply do the same optimization as for
       ASSIGN_LOCAL_AND_POP with assign global, since assigning
       at times does not add references.

       We do know, however, that refs (should) never reach 0 when
       poping the stack. We can thus skip that part of pop_value
    */
    ins_debug_instr_prologue(a-F_OFFSET, b, 0);
    amd64_load_fp_reg();
    amd64_load_sp_reg();

    mov_mem_reg(fp_reg, OFFSETOF(pike_frame, current_object),    ARG1_REG);
    mov_mem_reg(fp_reg, OFFSETOF(pike_frame,context),            ARG2_REG);
    mov_mem16_reg(ARG2_REG, OFFSETOF(inherit, identifier_level), ARG2_REG);

    add_reg_imm( ARG2_REG, b );
    add_reg_imm_reg( sp_reg, -sizeof(struct svalue), ARG3_REG );
    amd64_call_c_function( object_low_set_index );

    if( a == F_ASSIGN_GLOBAL_AND_POP )
    {
      /* assign done, pop. */
      amd64_load_sp_reg();
      amd64_add_sp( -1 );
      amd64_free_svalue( sp_reg, 1 );
    }
    return;

  case F_SIZEOF_LOCAL:
    {
      LABELS();
      ins_debug_instr_prologue(a-F_OFFSET, b, 0);
      amd64_load_fp_reg();
      amd64_load_sp_reg();

      mov_mem_reg( fp_reg, OFFSETOF(pike_frame,locals), ARG1_REG);
      add_reg_imm( ARG1_REG, b*sizeof(struct svalue));
      mov_sval_type( ARG1_REG, P_REG_RAX );
      /* type in RAX, svalue in ARG1 */
      cmp_reg32_imm( P_REG_RAX, PIKE_T_ARRAY );
      jne( &label_A );
      /* It's an array */
      /* move arg to point to the array */
      mov_mem_reg( ARG1_REG, OFFSETOF(svalue, u.array ), ARG1_REG);
      /* load size -> RAX*/
      mov_mem32_reg( ARG1_REG,OFFSETOF(array, size), P_REG_RAX );
      jmp( &label_C );
      LABEL_A;
      cmp_reg32_imm( P_REG_RAX, PIKE_T_STRING );
      jne( &label_B );
      /* It's a string */
      /* move arg to point to the string */
      mov_mem_reg( ARG1_REG, OFFSETOF(svalue, u.string ), ARG1_REG);
      /* load size ->RAX*/
      mov_mem32_reg( ARG1_REG,OFFSETOF(pike_string, len ), P_REG_RAX );
      jmp( &label_C );
      LABEL_B;
      /* It's something else, svalue already in ARG1. */
      amd64_call_c_function( pike_sizeof );
      LABEL_C;/* all done, res in RAX */
      /* Store result on stack */
      amd64_push_int_reg( P_REG_RAX );
    }
    return;

  case F_SIZEOF_LOCAL_STRING:
    {
      LABELS();
      ins_debug_instr_prologue(a-F_OFFSET, b, 0);
      amd64_load_fp_reg();
      amd64_load_sp_reg();

      mov_mem_reg( fp_reg, OFFSETOF(pike_frame,locals), ARG1_REG);
      add_reg_imm( ARG1_REG, b*sizeof(struct svalue));
      mov_sval_type( ARG1_REG, P_REG_RAX );
      mov_mem_reg( ARG1_REG, OFFSETOF(svalue, u.string ), P_REG_RBX);
      /* type in RAX, svalue in ARG1 */
      cmp_reg32_imm( P_REG_RAX, PIKE_T_STRING );
      je( &label_B );
      /* It's something else, svalue already in ARG1. */
      amd64_call_c_function( pike_sizeof );
      jmp( &label_A );
      LABEL_B;
      mov_mem32_reg( P_REG_RBX,OFFSETOF(pike_string, len ), P_REG_RAX );
      /* Store result on stack */
      LABEL_A;
      amd64_push_int_reg( P_REG_RAX );
    }
    return;

  case F_PRIVATE_GLOBAL:
/*
  This is a private (or final) global which is not a 'short' one.

  It is guaranteed to be available as an svalue at
  pike_fp->current_object->storage + pike_fp->context->storage_offset + offset

  (I really wish pike_fp->current_storage was kept up to date for pike
  function..)
*/
    ins_debug_instr_prologue(a-F_OFFSET, b, 0);
    amd64_load_sp_reg();
    amd64_get_storage( P_REG_RBX, b );
    amd64_push_svaluep( P_REG_RBX );
    return;

  case F_GLOBAL:
    ins_debug_instr_prologue(a-F_OFFSET, b, 0);
    amd64_load_fp_reg();
    amd64_load_sp_reg();
    mov_mem_reg(fp_reg, OFFSETOF(pike_frame, context), ARG3_REG);
    mov_mem_reg(fp_reg, OFFSETOF(pike_frame, current_object),
			      ARG2_REG);
    mov_reg_reg(sp_reg, ARG1_REG);
    mov_mem16_reg(ARG3_REG, OFFSETOF(inherit, identifier_level),
                  ARG3_REG);
    add_reg_imm(ARG3_REG, b);
    amd64_call_c_function(low_object_index_no_free);
    amd64_add_sp(1);
    return;

  case F_LOCAL:
    ins_debug_instr_prologue(a-F_OFFSET, b, 0);
    amd64_load_fp_reg();
    amd64_load_sp_reg();
    mov_mem_reg(fp_reg, OFFSETOF(pike_frame, locals), P_REG_RCX);
    add_reg_imm(P_REG_RCX, b*sizeof(struct svalue));
    amd64_push_svaluep(P_REG_RCX);
    return;

  case F_CLEAR_LOCAL:
    ins_f_byte_with_2_args( F_CLEAR_N_LOCAL, b, 1 );
    return;

  case F_SWAP_STACK_LOCAL:
    /*
     * tmp = Pike_sp[-1]
     * Pike_sp[-1] = Pike_fp->locals[arg1]
     * Pike_fp->locals[arg1] = tmp
     */
    ins_debug_instr_prologue(a-F_OFFSET, b, 0);
    amd64_load_fp_reg();
    amd64_load_sp_reg();
    mov_mem_reg(fp_reg, OFFSETOF(pike_frame, locals), P_REG_RAX);
    add_reg_imm_reg( sp_reg, -sizeof(struct svalue), P_REG_RCX );

#if SIZEOF_LONG_DOUBLE != 16
    if (OFFSETOF(array, u.real_item) & 0x0f) {
      mov_mem_reg( P_REG_RAX, b * sizeof(struct svalue), P_REG_RBX );
      mov_mem_reg( P_REG_RCX, 0, P_REG_R10 );
      mov_reg_mem( P_REG_RBX, P_REG_RCX, 0 );
      mov_reg_mem( P_REG_R10, P_REG_RAX, b * sizeof(struct svalue) );
      mov_mem_reg( P_REG_RAX, b * sizeof(struct svalue) + 8, P_REG_RBX );
      mov_mem_reg( P_REG_RCX, 8, P_REG_R10 );
      mov_reg_mem( P_REG_RBX, P_REG_RCX, 8 );
      mov_reg_mem( P_REG_R10, P_REG_RAX, b * sizeof(struct svalue) + 8 );
      return
    }
#endif

    mov_mem128_reg( P_REG_RAX, b * sizeof(struct svalue), P_REG_XMM1 );
    mov_mem128_reg( P_REG_RCX, 0, P_REG_XMM0 );
    mov_reg_mem128( P_REG_XMM0, P_REG_RAX, b * sizeof(struct svalue) );
    mov_reg_mem128( P_REG_XMM1, P_REG_RCX, 0 );
    return;

  case F_INC_LOCAL_AND_POP:
    {
      LABELS();
      ins_debug_instr_prologue(a-F_OFFSET, b, 0);
      amd64_load_fp_reg();
      mov_mem_reg(fp_reg, OFFSETOF(pike_frame, locals), P_REG_RCX);
      add_reg_imm(P_REG_RCX, b*sizeof(struct svalue));
      mov_sval_type(P_REG_RCX, P_REG_RAX);
      cmp_reg32_imm(P_REG_RAX, PIKE_T_INT);
      jne(&label_A);
      /* Integer - Zap subtype and try just incrementing it. */
      mov_reg_mem32(P_REG_RAX, P_REG_RCX, OFFSETOF(svalue, tu.t.type));
      add_imm_mem(1, P_REG_RCX, OFFSETOF(svalue, u.integer));
      jno(&label_B);
      add_imm_mem(-1, P_REG_RCX, OFFSETOF(svalue, u.integer));
      LABEL_A;
      /* Fallback to the C-implementation. */
      update_arg1(b);
      amd64_call_c_opcode(instrs[a-F_OFFSET].address,
			  instrs[a-F_OFFSET].flags);
      LABEL_B;
    }
    return;

  case F_INC_LOCAL:
    ins_f_byte_with_arg(F_INC_LOCAL_AND_POP, b);
    ins_f_byte_with_arg(F_LOCAL, b);
    return;

  case F_POST_INC_LOCAL:
    ins_f_byte_with_arg(F_LOCAL, b);
    ins_f_byte_with_arg(F_INC_LOCAL_AND_POP, b);
    return;

  case F_DEC_LOCAL_AND_POP:
    {
      LABELS();
      ins_debug_instr_prologue(a-F_OFFSET, b, 0);
      amd64_load_fp_reg();
      mov_mem_reg(fp_reg, OFFSETOF(pike_frame, locals), P_REG_RCX);
      add_reg_imm(P_REG_RCX, b*sizeof(struct svalue));
      mov_sval_type(P_REG_RCX, P_REG_RAX);
      cmp_reg32_imm(P_REG_RAX, PIKE_T_INT);
      jne(&label_A);
      /* Integer - Zap subtype and try just decrementing it. */
      mov_reg_mem32(P_REG_RAX, P_REG_RCX, OFFSETOF(svalue, tu.t.type));
      add_imm_mem(-1, P_REG_RCX, OFFSETOF(svalue, u.integer));
      jno(&label_B);
      add_imm_mem(1, P_REG_RCX, OFFSETOF(svalue, u.integer));
      LABEL_A;
      /* Fallback to the C-implementation. */
      update_arg1(b);
      amd64_call_c_opcode(instrs[a-F_OFFSET].address,
			  instrs[a-F_OFFSET].flags);
      LABEL_B;
    }
    return;

  case F_DEC_LOCAL:
    ins_f_byte_with_arg(F_DEC_LOCAL_AND_POP, b);
    ins_f_byte_with_arg(F_LOCAL, b);
    return;

  case F_POST_DEC_LOCAL:
    ins_f_byte_with_arg(F_LOCAL, b);
    ins_f_byte_with_arg(F_DEC_LOCAL_AND_POP, b);
    return;

#if 0
    /*
      These really have to be done inline:

       reg = (sp- *--mark_sp)>>16
       ltosval_and_free(-(args+2)) -> pike_sp-args
       call_builtin(reg)
       -- stack now lvalue, result, so now..
       ins_f_byte( F_ASSIGN or F_ASSIGN_AND_POP )
    */
  case F_LTOSVAL_CALL_BUILTIN_AND_ASSIGN_POP:
  case F_LTOSVAL_CALL_BUILTIN_AND_ASSIGN:

    return;
#endif

  case F_CALL_BUILTIN_AND_POP:
    ins_f_byte_with_arg( F_CALL_BUILTIN, b );
    ins_f_byte( F_POP_VALUE );
    return;

  case F_MARK_CALL_BUILTIN_AND_RETURN:
    ins_f_byte_with_arg( F_MARK_CALL_BUILTIN, b );
    ins_f_byte( F_DUMB_RETURN );
    return;

  case F_MARK_CALL_BUILTIN_AND_POP:
    ins_f_byte_with_arg( F_MARK_CALL_BUILTIN, b );
    ins_f_byte( F_POP_VALUE );
    return;

  case F_CALL_BUILTIN1_AND_POP:
    ins_f_byte_with_arg( F_CALL_BUILTIN1, b );
    ins_f_byte( F_POP_VALUE );
    return;

  case F_CALL_BUILTIN_AND_RETURN:
    ins_f_byte_with_arg( F_CALL_BUILTIN, b );
    ins_f_byte( F_DUMB_RETURN );
    return;

  case F_CALL_BUILTIN:
    ins_debug_instr_prologue(a-F_OFFSET, b, 0);

    amd64_load_mark_sp_reg();
    amd64_load_sp_reg();

    mov_mem_reg( mark_sp_reg, -sizeof(struct svalue*), P_REG_RAX );
    amd64_add_mark_sp( -1 );
    mov_reg_reg( sp_reg, ARG1_REG );
    sub_reg_reg( ARG1_REG, P_REG_RAX );
    shr_reg_imm( ARG1_REG, 4 );
    /* arg1 = (sp_reg - *--mark_sp)/16 (sizeof(svalue)) */

    /* FALLTHRU */

  case F_MARK_CALL_BUILTIN:
    if(a == F_MARK_CALL_BUILTIN )
    {
      ins_debug_instr_prologue(a-F_OFFSET, b, 0);
      mov_imm_reg( 0, ARG1_REG );
    }

    /* FALLTHRU */

  case F_CALL_BUILTIN1:
    if(a == F_CALL_BUILTIN1 )
    {
      ins_debug_instr_prologue(a-F_OFFSET, b, 0);
      mov_imm_reg( 1, ARG1_REG );
    }

    /* Note: It is not actually possible to do ins_debug_instr_prologue
     *       here.
     */
    amd64_call_c_opcode(Pike_compiler->new_program->constants[b].sval.u.efun->function,
                        I_UPDATE_SP);
    return;

  case F_CONSTANT:
    {
      struct svalue *cval = &Pike_compiler->new_program->constants[b].sval;
      ins_debug_instr_prologue(a-F_OFFSET, b, 0);
      if( TYPEOF(*cval) == PIKE_T_INT &&
          SUBTYPEOF(*cval) == NUMBER_UNDEFINED)
      {
          amd64_load_fp_reg();
          amd64_load_sp_reg();
          /* We end up here in programs decoded from decode_value.
             constants are not restored until after the program
             code is generated.

             (there is no way to differentiate a 'real' undefined here)
          */
          mov_mem_reg( fp_reg, OFFSETOF(pike_frame,context), P_REG_RCX );
          mov_mem_reg( P_REG_RCX, OFFSETOF(inherit,prog), P_REG_RCX );
          mov_mem_reg( P_REG_RCX, OFFSETOF(program,constants), P_REG_RCX );
          add_reg_imm( P_REG_RCX, b*sizeof(struct program_constant) +
                       OFFSETOF(program_constant,sval) );
          amd64_push_svaluep( P_REG_RCX );
          /* ~14 instructions total, 3-4 deref */
      }
      else
      {
          amd64_load_sp_reg();
          mov_imm_mem(cval->tu.type_subtype, sp_reg, SVAL(0).type);
          if( TYPEOF(*cval) >= MIN_REF_TYPE )
          {
              mov_imm_reg(cval->u.integer, P_REG_RCX);
              mov_reg_mem(P_REG_RCX, sp_reg, SVAL(0).value);
              add_imm_mem( 1, P_REG_RCX, OFFSETOF(pike_string, refs));
          }
          else
          {
              mov_imm_mem(cval->u.integer, sp_reg, SVAL(0).value);
          }
          amd64_add_sp(1);
          /* 2 or 4 instructions total, 0-1 deref */
      }
    }
    return;

  case F_GLOBAL_LVALUE:
    ins_debug_instr_prologue(a-F_OFFSET, b, 0);
    amd64_load_fp_reg();
    amd64_load_sp_reg();

    amd64_push_this_object( );

    mov_imm_mem( T_OBJ_INDEX,  sp_reg, OFFSETOF(svalue, tu.t.type));
    mov_mem_reg(fp_reg, OFFSETOF(pike_frame, context), P_REG_RAX);
    mov_mem16_reg( P_REG_RAX,OFFSETOF(inherit, identifier_level), P_REG_RAX);
    add_reg_imm( P_REG_RAX, b );
    mov_reg_mem( P_REG_RAX, sp_reg, OFFSETOF(svalue,u.identifier) );
    amd64_add_sp( 1 );
    return;

  case F_LOCAL_LVALUE:
    ins_debug_instr_prologue(a-F_OFFSET, b, 0);
    amd64_load_fp_reg();
    amd64_load_sp_reg();

    /* &frame->locals[b] */
    mov_mem_reg( fp_reg, OFFSETOF(pike_frame, locals), P_REG_RAX);
    add_reg_imm( P_REG_RAX, b*sizeof(struct svalue));

    mov_imm_mem( T_SVALUE_PTR,  sp_reg, OFFSETOF(svalue, tu.t.type));
    mov_reg_mem( P_REG_RAX, sp_reg, OFFSETOF(svalue,u.lval) );
    mov_imm_mem( T_VOID,  sp_reg,
		 OFFSETOF(svalue, tu.t.type)+sizeof(struct svalue));
    amd64_add_sp( 2 );
    return;

  case F_MARK_AT:
    ins_debug_instr_prologue(a-F_OFFSET, b, 0);
    amd64_load_fp_reg();
    amd64_load_mark_sp_reg();
    if (Pike_compiler->compiler_frame->generator_local != -1) {
      mov_mem_reg(fp_reg, OFFSETOF(pike_frame, save_sp), ARG1_REG);
    } else {
      mov_mem_reg(fp_reg, OFFSETOF(pike_frame, locals), ARG1_REG);
    }
    if (b) {
      add_reg_imm_reg(ARG1_REG, sizeof(struct svalue) * b, ARG1_REG);
    }
    mov_reg_mem(ARG1_REG, mark_sp_reg, 0x00);
    amd64_add_mark_sp( 1 );
    return;
  }
  update_arg1(b);
  ins_f_byte(a);
}

int amd64_ins_f_jump_with_arg(unsigned int op, INT32 a, int backward_jump)
{
  LABELS();
  if (!(instrs[op - F_OFFSET].flags & I_BRANCH)) return -1;

  switch( op )
  {
    /* case F_BRANCH_IF_TYPE_IS_NOT: */
    /*   ins_debug_instr_prologue(op-F_OFFSET, a, 0); */
    /*   amd64_load_sp_reg(); */
    /*   amd64_add_sp(-1); */
    /*   mov_sval_type( sp_reg, P_REG_RAX ); */
    /*   cmp_reg32_imm(P_REG_RAX, PIKE_T_OBJECT ); */
    /*   jne( &label_A ); */
    /*   amd64_add_sp(1); */
    /*   amd64_call_c_opcode(instrs[op-F_OFFSET].address, I_UPDATE_SP); */
    /*   jz( &label_B ); /\* do not branch. *\/ */
    /*   /\* branch. *\/ */
    /*   mov_imm_reg( a, P_REG_RAX ); */
    /*   jmp( &label_C ); */
    /*   amd64_load_sp_reg(); */
    /* label_A; */
    /*   amd64_free_svalue( sp_reg,  0); */
    /* label_C; */
    /*   cmp_reg32_imm(P_REG_RAX, a); */
    /*   int res= jnz_imm_rel32(0); */
    /* label_B; */
    /*   return res; */

    case F_BRANCH_IF_NOT_LOCAL:
    case F_BRANCH_IF_LOCAL:
      ins_debug_instr_prologue(op-F_OFFSET, a, 0);
      amd64_load_fp_reg();
      mov_mem_reg( fp_reg, OFFSETOF(pike_frame, locals), ARG1_REG);
      add_reg_imm( ARG1_REG, a*sizeof(struct svalue));
      /* if( type == PIKE_T_INT )
           u.integer -> RAX
         else if( type == PIKE_T_OBJECT || type == PIKE_T_FUNCTION )
           call svalue_is_true(&local)
         else
           1 -> RAX

        The tests are ordered assuming integers are most commonly
        checked. That is not nessasarily true.
      */
      mov_sval_type( ARG1_REG, P_REG_RCX );
      cmp_reg32_imm( P_REG_RCX, PIKE_T_INT );
      je( &label_C );
      mov_imm_reg( 1, P_REG_RAX );
      shl_reg32_reg( P_REG_RAX, P_REG_RCX );
      and_reg32_imm( P_REG_RAX, BIT_FUNCTION|BIT_OBJECT );
      jnz( &label_A );
      /* Not object, int or function. Always true. */
      mov_imm_reg( 1, P_REG_RAX );
      jmp( &label_B );
    LABEL_A;
      amd64_call_c_function(svalue_is_true);
      jmp( &label_B );

    LABEL_C;
     mov_mem_reg( ARG1_REG, OFFSETOF(svalue, u.integer ), P_REG_RAX );
    /* integer. */

    LABEL_B;
      test_reg( P_REG_RAX );
      if( op == F_BRANCH_IF_LOCAL )
        return jnz_imm_rel32(0);
      return jz_imm_rel32(0);
  }

  maybe_update_pc();
  update_arg1(a);
  return amd64_ins_f_jump(op, backward_jump);
}

void ins_f_byte_with_2_args(unsigned int a, INT32 b, INT32 c)
{
  maybe_update_pc();
  switch(a) {
  case F_NUMBER64:
    ins_debug_instr_prologue(a-F_OFFSET, b, c);
    amd64_push_int((((UINT64)b)<<32)|(unsigned INT32)c, 0);
    return;
  case F_MARK_AND_EXTERNAL:
    ins_f_byte(F_MARK);
    ins_f_byte_with_2_args(F_EXTERNAL, b, c);
    return;

  case F_CLEAR_N_LOCAL:
    {
      LABELS();
      ins_debug_instr_prologue(a-F_OFFSET, b, c);
      amd64_load_fp_reg();
      mov_mem_reg(fp_reg, OFFSETOF(pike_frame, locals), P_REG_RBX);
      add_reg_imm(P_REG_RBX, b*sizeof(struct svalue));
      if( c > 1 ) {
	push(P_REG_RBP);
        sub_reg_imm(P_REG_RSP, 8);	/* Align on 16 bytes. */
        add_reg_imm_reg(P_REG_RBX, c*sizeof(struct svalue), P_REG_RBP );
      }

     LABEL_A;
      amd64_free_svalue(P_REG_RBX, 0);
      mov_imm_mem(PIKE_T_INT, P_REG_RBX, OFFSETOF(svalue, tu.t.type));
      mov_imm_mem(0, P_REG_RBX, OFFSETOF(svalue, u.integer));
      if( c > 1 )
      {
        add_reg_imm(P_REG_RBX, sizeof(struct svalue ) );
        cmp_reg_reg( P_REG_RBX, P_REG_RBP );
        jne(&label_A);
        add_reg_imm(P_REG_RSP, 8);
	pop(P_REG_RBP);
      }
    }
    return;

  case F_LOCAL_LOCAL_INDEX:
    {
      LABELS();
      ins_debug_instr_prologue(a-F_OFFSET, b, c);
      amd64_load_fp_reg();
      mov_mem_reg( fp_reg, OFFSETOF(pike_frame,locals), P_REG_RDX);

      add_reg_imm_reg( P_REG_RDX, b*sizeof(struct svalue), P_REG_RBX);
      add_reg_imm_reg( P_REG_RDX, c*sizeof(struct svalue), P_REG_RCX);

      /* bx = svalue for value */
      /* cx = svalue for index */
      mov_sval_type( P_REG_RBX, P_REG_RAX );
      mov_sval_type( P_REG_RCX, P_REG_RDX );
      shl_reg_imm( P_REG_RAX, 8 );
      /* fixme: Add add_mem8_reg */
      add_reg_reg( P_REG_RAX, P_REG_RDX );

      cmp_reg32_imm( P_REG_RAX, (PIKE_T_ARRAY<<8)|PIKE_T_INT );
      jne( &label_B );

      /* ax = value->tyupe<<8|index->type */
      mov_mem_reg( P_REG_RBX, 8, P_REG_RBX );  /* value */
      mov_mem_reg( P_REG_RCX, 8, P_REG_R9 );   /* int */

      /* dx = value->u.array, bx = index */
      /* Array and int index. */
      mov_mem32_reg( P_REG_RDX, OFFSETOF(array,size), P_REG_R8 );
      cmp_reg32_imm( P_REG_RBX, 0 );
      jge( &label_D );
      /* index less than 0, add array size */
      add_reg_reg( P_REG_RBX, P_REG_R8 );

    LABEL_D;
      cmp_reg32_imm( P_REG_RBX, 0 );
      jl( &label_B ); // index (still) <0
      cmp_reg_reg( P_REG_RBX, P_REG_R8);
      jge( &label_B ); //index >=size

      /* array->item + x to stack. done */
      mov_mem_reg( P_REG_RDX, OFFSETOF(array,item), P_REG_RCX );
      /* cx = arr->item */
      shl_reg_imm( P_REG_RBX, 4 );
      add_reg_reg( P_REG_RBX, P_REG_RCX );
      /* bx = bx * sizeof(svalue) + cx */
      amd64_push_svaluep( P_REG_RBX );
      jmp( &label_C );

    LABEL_B;
      /* something else. */
      update_arg1(b);
      update_arg2(c);
      amd64_call_c_opcode(instrs[a-F_OFFSET].address,
                          instrs[a-F_OFFSET].flags);
    LABEL_C;
      /* done */
    }
    return;

   case F_PRIVATE_IF_DIRECT_GLOBAL:
    {
      LABELS();
      amd64_load_sp_reg();
      ins_debug_instr_prologue(a-F_OFFSET, b, c);
      /* do not assign if this object is destructed. */
      mov_mem_reg( fp_reg, OFFSETOF(pike_frame,current_object), ARG2_REG );
      mov_mem_reg( fp_reg, OFFSETOF(pike_frame,context),        ARG3_REG );
      /* if ctx->prog != arg1->prog */
      mov_mem_reg( ARG3_REG, OFFSETOF(inherit,prog), P_REG_R8 );
      mov_mem_reg( ARG2_REG, OFFSETOF(object,prog), P_REG_RAX );
      cmp_reg_reg(P_REG_R8,P_REG_RAX);
     je(&label_A);
      mov_reg_reg(sp_reg, ARG1_REG);
      mov_mem16_reg(ARG3_REG, OFFSETOF(inherit, identifier_level),
		    ARG3_REG);
      add_reg_imm(ARG3_REG, c);
      amd64_call_c_function(low_object_index_no_free);
      amd64_add_sp(1);
      jmp(&label_B);

     LABEL_A;
      mov_mem_reg(ARG2_REG, OFFSETOF(object,storage), P_REG_RBX );
      add_reg_mem(P_REG_RBX,  ARG3_REG, OFFSETOF(inherit,storage_offset));
      add_reg_imm(P_REG_RBX, b );
      amd64_push_svaluep( P_REG_RBX );
     LABEL_B;
      return;
    }

  case F_ASSIGN_PRIVATE_IF_DIRECT_GLOBAL:
    {
      LABELS();
      amd64_load_sp_reg();
      ins_debug_instr_prologue(a-F_OFFSET, b, c);
      /* do not assign if this object is destructed. */
      mov_mem_reg( fp_reg, OFFSETOF(pike_frame,current_object), ARG1_REG );
      mov_mem_reg( fp_reg, OFFSETOF(pike_frame,context),        ARG2_REG );
      /* if ctx->prog != arg1->prog */
      mov_mem_reg( ARG1_REG, OFFSETOF(object,prog), P_REG_RAX );
      mov_mem_reg( ARG2_REG, OFFSETOF(inherit,prog), P_REG_R8 );
      cmp_reg_reg(P_REG_R8,P_REG_RAX);
      je(&label_A);

      /* arg1 = object */
      /* arg2 = c + identifier_level */
      /* arg3 = Pike_sp-1 */
      mov_mem16_reg( ARG2_REG, OFFSETOF(inherit,identifier_level), ARG2_REG);
      add_reg_imm( ARG2_REG, c );
      add_reg_imm_reg( sp_reg, -1*sizeof(struct svalue), ARG3_REG );
      amd64_call_c_function(object_low_set_index);
      jmp(&label_B);
    LABEL_A;
      mov_mem_reg(ARG1_REG, OFFSETOF(object,storage), P_REG_RBX );
      add_reg_mem(P_REG_RBX,  ARG2_REG, OFFSETOF(inherit,storage_offset));
      add_reg_imm(P_REG_RBX, b );
      amd64_free_svalue( P_REG_RBX, 0 );
      amd64_assign_svalue_no_free( P_REG_RBX, sp_reg, -sizeof(struct svalue));
      amd64_ref_svalue(P_REG_RBX,0);
    LABEL_B;
    }
    return;

  case F_ASSIGN_PRIVATE_TYPED_GLOBAL_AND_POP:
  case F_ASSIGN_PRIVATE_TYPED_GLOBAL:
  {
    LABELS();
    amd64_load_sp_reg();

    ins_debug_instr_prologue(a-F_OFFSET, b, c);

    amd64_get_storage( P_REG_RBX, b );

    /* do not assign anything if this object is destructed. */

    /* we really only need to redo this check after we have called
       some other function.  It is sort of hard to know when that
       happens while generating the code, however. Simply assigning
       the global could in theory actually destruct this object (old
       value includes something with a _destruct that calls destruct on
       this object...)

       Note that nothing crash if we do the assign. We "just" leak the
       variable when the storage is eventually free:d.

       The generated code does look sort of silly, however.
    */
    mov_mem_reg( fp_reg, OFFSETOF(pike_frame,current_object), ARG1_REG );
    mov_mem_reg( ARG1_REG, OFFSETOF(object,prog), P_REG_RAX );
    test_reg(P_REG_RAX);
    jnz(&label_A);

    /* will not return. */
    amd64_call_c_function(object_low_set_index);

  LABEL_A;
    /* 1: check type of argument */
    mov_mem8_reg( sp_reg, -sizeof(struct svalue )+OFFSETOF(svalue,tu.t.type), P_REG_RAX);
    cmp_reg32_imm( P_REG_RAX, c );
    je( &label_B );

    /* not the same type. Fall back to use C-function (for code size mainly).. */
    mov_reg_reg( P_REG_RBX, ARG1_REG );
    mov_imm_reg( c, ARG2_REG );
    add_reg_imm_reg( sp_reg, -sizeof(struct svalue), ARG3_REG );
    amd64_call_c_function(assign_to_short_svalue);
    /* pop stack if needed. */
    if( a == F_ASSIGN_PRIVATE_TYPED_GLOBAL_AND_POP )
    {
      amd64_add_sp(-1);
      /* this will either have assigned 0 or thrown an error.
       * if we assigned 0, it was because the argument evaluated as false.
       * However, that means that we might possibly have to free it here..
       *
       * We do not even know that is has at least two references.
       */
      amd64_free_svalue( sp_reg, 0 );
    }

    jmp( &label_E ); /* pop or not. */

   LABEL_B;
    /* at this point: o->storage + off in RBX. */
    if( c >= MIN_REF_TYPE )
    {
      mov_mem_reg( P_REG_RBX, 0, P_REG_RAX );
      test_reg(P_REG_RAX); /* old value == NULL? */
      jz(&label_C);

      /* deref. */
      add_mem32_imm( P_REG_RAX, OFFSETOF(pike_string,refs),  -1);
      jnz(&label_C);

      mov_reg_reg( P_REG_RBX, ARG1_REG );
      mov_imm_reg( c, ARG2_REG );
      amd64_call_c_function(really_free_short_svalue_ptr);
    }
   LABEL_C; /* old value is gone. Assign new value */
    mov_mem_reg( sp_reg, -8, P_REG_RAX );
    mov_reg_mem( P_REG_RAX, P_REG_RBX, 0 );

    if( a == F_ASSIGN_PRIVATE_TYPED_GLOBAL_AND_POP )
      amd64_add_sp( -1 );
    else if( c >= MIN_REF_TYPE )
      add_mem_imm( P_REG_RAX, OFFSETOF(pike_string,refs), 1 );
   LABEL_E;
  }
  return;
  case F_PRIVATE_TYPED_GLOBAL:
    /* b -> off, c -> type */
    ins_debug_instr_prologue(a-F_OFFSET, b, c);
    amd64_load_sp_reg();
    if( c < MIN_REF_TYPE )
    {
      /* really: int or float. And right now, only float */
      amd64_get_storage( P_REG_RBX, b );
      mov_mem_reg( P_REG_RBX, 0, P_REG_RBX );
      mov_imm_mem( c, sp_reg, 0 );
      mov_reg_mem( P_REG_RBX, sp_reg, 8 );
      amd64_add_sp(1);
    }
    else
    {
      LABELS();
      /* one of the refcounted types. */
      amd64_get_storage( P_REG_RBX, b );
      mov_mem_reg( P_REG_RBX, 0, P_REG_RBX );
      test_reg( P_REG_RBX );
      jnz(&label_A);
      amd64_push_int(0,1);
      jmp(&label_B);
    LABEL_A;
      add_mem_imm( P_REG_RBX, 0, 1 );
      mov_imm_mem( c, sp_reg, 0 );
      mov_reg_mem( P_REG_RBX, sp_reg, 8 );
      amd64_add_sp(1);
    LABEL_B;
    }
    return;
  case F_ADD_LOCAL_INT:
  case F_ADD_LOCAL_INT_AND_POP:
   {
      LABELS();
      ins_debug_instr_prologue(a-F_OFFSET, b, c);
      amd64_load_fp_reg();
      mov_mem_reg( fp_reg, OFFSETOF(pike_frame, locals), ARG1_REG);
      add_reg_imm( ARG1_REG, b*sizeof(struct svalue) );

      /* arg1 = dst
         arg2 = int
      */
      mov_sval_type( ARG1_REG, P_REG_RAX );
      cmp_reg32_imm( P_REG_RAX, PIKE_T_INT );
      jne(&label_A); /* Fallback */
      mov_imm_mem( PIKE_T_INT, ARG1_REG, OFFSETOF(svalue, tu.t.type));
      add_imm_mem( c, ARG1_REG,OFFSETOF(svalue,u.integer));
      jno( &label_B);
      add_imm_mem( -c, ARG1_REG,OFFSETOF(svalue,u.integer));
      /* Overflow. Use C version */
      LABEL_A;
      update_arg2(c);
      update_arg1(b);
      ins_f_byte(a);
      /* Push already done by C version. */
      if( a == F_ADD_LOCAL_INT )
        jmp( &label_C );

      LABEL_B;
      if( a == F_ADD_LOCAL_INT )
      {
        /* push the local. */
        /* We know it's an integer (since we did not use the fallback) */
        amd64_load_sp_reg();
        mov_mem_reg( ARG1_REG, OFFSETOF(svalue,u.integer), P_REG_RAX );
        amd64_push_int_reg( P_REG_RAX );
      }
      LABEL_C;
      return;
     }

#if 0
  /* this is a: nonworking, and b: not really all that more efficient anyway.. */
  case F_APPLY_N:
     ins_debug_instr_prologue(a-F_OFFSET, b, c);
     mov_imm_reg( APPLY_SVALUE_STRICT, ARG1_REG );
     mov_imm_reg( c, ARG2_REG );
     mov_ptr_reg( &((Pike_fp->context->prog->constants + b)->sval), ARG3_REG );
     clear_reg( ARG4_REG );
     amd64_call_c_opcode(mega_apply, I_UPDATE_SP);
    return;
#endif
  case F_CALL_BUILTIN_N:
    ins_debug_instr_prologue(a-F_OFFSET, b, c);
    mov_imm_reg( c, ARG1_REG );
    amd64_call_c_opcode(Pike_compiler->new_program->constants[b].sval.u.efun->function,
                        I_UPDATE_SP);
    return;


  case F_ADD_LOCALS_AND_POP:
    {
      LABELS();
      ins_debug_instr_prologue(a-F_OFFSET, b, 0);
      amd64_load_fp_reg();
      mov_mem_reg( fp_reg, OFFSETOF(pike_frame, locals), ARG1_REG);
      add_reg_imm( ARG1_REG, b*sizeof(struct svalue) );
      add_reg_imm_reg( ARG1_REG,(c-b)*sizeof(struct svalue), ARG2_REG );

      /* arg1 = dst
         arg2 = src
      */
      mov_sval_type( ARG1_REG, P_REG_RAX );
      mov_sval_type( ARG2_REG, P_REG_RBX );
      shl_reg_imm( P_REG_RAX, 8 );
      add_reg_reg( P_REG_RAX, P_REG_RBX );
      cmp_reg32_imm( P_REG_RAX, (PIKE_T_INT<<8) | PIKE_T_INT );
      jne(&label_A); /* Fallback */
      mov_mem_reg( ARG2_REG, OFFSETOF(svalue,u.integer), P_REG_RAX );
      add_reg_mem( P_REG_RAX, ARG1_REG, OFFSETOF(svalue,u.integer));
      jo( &label_A);
      /* Clear subtype */
      mov_imm_mem( PIKE_T_INT, ARG1_REG,OFFSETOF(svalue, tu.t.type));
      mov_reg_mem( P_REG_RAX, ARG1_REG, OFFSETOF(svalue,u.integer));
      jmp( &label_B );

      LABEL_A;
      update_arg2(c);
      update_arg1(b);
      ins_f_byte(a); /* Will call C version */
      LABEL_B;
      return;
    }

  case F_ASSIGN_LOCAL_NUMBER_AND_POP:
    ins_debug_instr_prologue(a-F_OFFSET, b, c);
    amd64_load_fp_reg();
    mov_mem_reg( fp_reg, OFFSETOF(pike_frame, locals), ARG1_REG);
    add_reg_imm( ARG1_REG,b*sizeof(struct svalue) );
    mov_reg_reg( ARG1_REG, P_REG_RBX );
    amd64_free_svalue(ARG1_REG, 0);
    mov_imm_mem(c, P_REG_RBX, OFFSETOF(svalue, u.integer));
    mov_imm_mem32(PIKE_T_INT, P_REG_RBX, OFFSETOF(svalue, tu.t.type));
    return;

  case F_LOCAL_2_GLOBAL:
    ins_debug_instr_prologue(a-F_OFFSET, b, 0);
    amd64_load_fp_reg();
    amd64_load_sp_reg();
    mov_mem_reg( fp_reg, OFFSETOF(pike_frame, locals), ARG3_REG );
    add_reg_imm( ARG3_REG, c*sizeof(struct svalue) );
    mov_mem_reg(fp_reg, OFFSETOF(pike_frame, current_object),    ARG1_REG);
    mov_mem_reg(fp_reg, OFFSETOF(pike_frame,context),            ARG2_REG);
    mov_mem16_reg(ARG2_REG, OFFSETOF(inherit, identifier_level), ARG2_REG);
    add_reg_imm( ARG2_REG, b );
    amd64_call_c_function( object_low_set_index );
    return;

  case F_LEXICAL_LOCAL:
    if( c < 5 )
    {
      ins_debug_instr_prologue(a-F_OFFSET, b, c);
      amd64_load_fp_reg();
      mov_reg_reg( fp_reg, P_REG_RAX );
      while(c--)
        mov_mem_reg( P_REG_RAX, OFFSETOF(pike_frame,scope), P_REG_RAX );
      mov_mem_reg( P_REG_RAX, OFFSETOF(pike_frame,locals), P_REG_RAX );
      add_reg_imm_reg( P_REG_RAX, b*sizeof(struct svalue), P_REG_RBX );
      amd64_push_svaluep( P_REG_RBX );
      return;
    }
    /* use c version. Could have a loop version here. */
    break;

  case F_LEXICAL_LOCAL_LVALUE:
    if( c < 5 )
    {
      ins_debug_instr_prologue(a-F_OFFSET, b, c);
      amd64_load_fp_reg();
      amd64_load_sp_reg();
      mov_reg_reg( fp_reg, P_REG_RAX );
      while(c--)
        mov_mem_reg( P_REG_RAX, OFFSETOF(pike_frame,scope), P_REG_RAX );
      mov_mem_reg( P_REG_RAX, OFFSETOF(pike_frame,locals), P_REG_RAX );
      add_reg_imm_reg( P_REG_RAX, b*sizeof(struct svalue), P_REG_RAX );
      mov_imm_mem( T_SVALUE_PTR,  sp_reg, OFFSETOF(svalue, tu.t.type));
      mov_reg_mem( P_REG_RAX, sp_reg, OFFSETOF(svalue,u.lval) );
      mov_imm_mem( T_VOID,  sp_reg,
                   OFFSETOF(svalue, tu.t.type)+sizeof(struct svalue));
      amd64_add_sp( 2 );
      return;
    }
    /* use c version. Could have a loop version here. */
    break;
  case F_LOCAL_2_LOCAL:
    ins_debug_instr_prologue(a-F_OFFSET, b, c);
    if( b != c )
    {
        amd64_load_fp_reg();
        mov_mem_reg( fp_reg, OFFSETOF(pike_frame, locals), P_REG_RBX );
        add_reg_imm( P_REG_RBX, b*sizeof(struct svalue) );
        /* RBX points to dst. */
        amd64_free_svalue( P_REG_RBX, 0 );
        /* assign rbx[0] = rbx[c-b] */
        mov_mem_reg( P_REG_RBX, (c-b)*sizeof(struct svalue), P_REG_RAX );
        mov_mem_reg( P_REG_RBX, (c-b)*sizeof(struct svalue)+8, P_REG_RCX );
        mov_reg_mem( P_REG_RAX, P_REG_RBX, 0 );
        mov_reg_mem( P_REG_RCX, P_REG_RBX, 8 );
        amd64_ref_svalue( P_REG_RBX, 1 );
    }
    return;
  case F_2_LOCALS:
#if 1
    ins_debug_instr_prologue(a-F_OFFSET, b, c);
    amd64_load_fp_reg();
    amd64_load_sp_reg();
    mov_mem_reg(fp_reg, OFFSETOF(pike_frame, locals), P_REG_R8);
    add_reg_imm( P_REG_R8, b*sizeof(struct svalue) );
    amd64_push_svaluep(P_REG_R8);
    add_reg_imm( P_REG_R8, (c-b)*sizeof(struct svalue) );
    amd64_push_svaluep(P_REG_R8);
#else
    ins_f_byte_with_arg( F_LOCAL, b );
    ins_f_byte_with_arg( F_LOCAL, c );
#endif
    return;

  case F_FILL_STACK:
    {
      LABELS();
      if (!b) return;
      ins_debug_instr_prologue(a-F_OFFSET, b, c);
      amd64_load_fp_reg();
      amd64_load_sp_reg();
      mov_mem_reg(fp_reg, OFFSETOF(pike_frame, locals), ARG1_REG);
      add_reg_imm(ARG1_REG, b*sizeof(struct svalue));
      jmp(&label_A);
      LABEL_B;
      amd64_push_int(0, c);
      LABEL_A;
      cmp_reg_reg(sp_reg, ARG1_REG);
      jl(&label_B);
    }
    return;

  case F_INIT_FRAME:
    ins_debug_instr_prologue(a-F_OFFSET, b, c);
    amd64_load_fp_reg();

    if(OFFSETOF(pike_frame, num_locals) != OFFSETOF(pike_frame, num_args)-2 )
        Pike_fatal("This code does not with unless num_args\n"
                   "directly follows num_locals in struct pike_frame\n");

    mov_imm_mem32( (b<<16)|c, fp_reg, OFFSETOF(pike_frame, num_locals));
    return;
  }
  update_arg2(c);
  update_arg1(b);
  ins_f_byte(a);
}

int amd64_ins_f_jump_with_2_args(unsigned int op, INT32 a, INT32 b,
				 int backward_jump)
{
  if (!(instrs[op - F_OFFSET].flags & I_BRANCH)) return -1;
  maybe_update_pc();
  update_arg2(b);
  update_arg1(a);
  return amd64_ins_f_jump(op, backward_jump);
}

void amd64_update_f_jump(INT32 offset, INT32 to_offset)
{
  upd_pointer(offset, to_offset - offset - 4);
}

INT32 amd64_read_f_jump(INT32 offset)
{
  return read_pointer(offset) + offset + 4;
}

#define OP_PREFIX	0x10000000UL
#define G1		(OP_PREFIX|0)
#define G2		(OP_PREFIX|1)
#define G3		(OP_PREFIX|2)
#define G4		(OP_PREFIX|3)

/* Table switch */
#define OP_MULTIBYTE	0x20000000UL
#define OP_F		(OP_MULTIBYTE|0)
#define OP_F_38		(OP_MULTIBYTE|1)
#define OP_F_3A		(OP_MULTIBYTE|2)

/* Additional fields */
#define OP_RM		0x00010000	/* ModR/M */
#define OP_DISPL	0x00020000	/* Displacement */
#define OP_IMM		0x00040000	/* Immediate */

#define OP_PCREL	0x00080000	/* PC-relative */

/* Implicit arguments */
#define OP_REG		0x00100000	/* Register in low 3 bits. */
#define OP_IMPLICIT_A	0x00200000
#define OP_IMPLICIT_C	0x00800000
#define OP_REG_S	0x00400000

/* Argument ordering */

#define OP_B_RM		0x00008000	/* B arg before RM arg. */

/* Operand widths */
#define OP_8		0x01000000	/* 8-bit */
#define OP_S8		0x02000000	/* 8-bit sign extended to 16 */

/* ModRM lookup */
#define OP_OPS		0x40000000	/* Lookup in modrm_ops */


#define OP_S		0

#define REG		(OP_REG)


#define RRM8		(OP_8|OP_RM|OP_B_RM)
#define RMR8		(OP_8|OP_RM)
#define RMR		(OP_RM)
#define RRM		(OP_RM|OP_B_RM)
#define RRMI8		(OP_8|OP_RM|OP_B_RM|OP_IMM)
#define RRMI		(OP_RM|OP_B_RM|OP_IMM)
#define SRM		(OP_S|OP_RM|OP_B_RM)
#define RMS		(OP_S|OP_RM)
#define ALM8		(OP_8|OP_IMPLICIT_A|OP_RM|OP_B_RM)
#define AM		(OP_IMPLICIT_A|OP_RM|OP_B_RM)
#define MAL8		(OP_IMPLICIT_A|OP_RM|OP_8)
#define MA		(OP_IMPLICIT_A|OP_RM)
#define RI8		(OP_RM|OP_IMM|OP_8)
#define RI		(OP_RM|OP_IMM)
#define RMI8OP		(OP_RM|OP_IMM|OP_8|OP_OPS)
#define RMSI8OP		(OP_RM|OP_IMM|OP_S8|OP_OPS)
#define RMIOP		(OP_RM|OP_IMM|OP_OPS)
#define ALI8		(OP_IMPLICIT_A|OP_IMM|OP_8)
#define AI		(OP_IMPLICIT_A|OP_IMM)
#define RM8OP		(OP_RM|OP_8|OP_OPS)
#define RMOP		(OP_RM|OP_OPS)
#define RMC8OP		(OP_RM|OP_8|OP_OPS|OP_IMPLICIT_C)
#define RMCOP		(OP_RM|OP_OPS|OP_IMPLICIT_C)
#define PCREL		(OP_PCREL)
#define PCREL8		(OP_PCREL|OP_8)
#define RM		(OP_RM)

static const char *modrm_ops[6][8] = {
  /* #0: ALU operations (Opcodes 0x80, 0x81, 0x83). */
  {
    "add", "or", "adc", NULL,
    "and", "sub", NULL, "cmp",
  },
  /* #1: Shift operations */
  {
    NULL, NULL, NULL, NULL,
    "sal", "shr", NULL, "sar",
  },
  /* #2: More ALU operations (Opcodes 0xf6, 0xf7). */
  {
    "test", NULL, NULL, "neg",
    "imul", NULL, NULL, "idiv",
  },
  /* #3: Inc/dec (Opcode 0xff) */
  {
    "inc", "dec", "call", "call",
    "jmp", "jmp", NULL, NULL,
  },
  /* #4: Mov (Opcodes 0xc6, 0xc7). */
  {
    "mov", NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
  },
  /* #5: Bit tests (Opcode 0xba) */
  {
    NULL, NULL, NULL, NULL,
    "bt", "bts", "btr", "btc",
  },
};

struct amd64_opcode {
  const char *name;
  UINT64 flags;
};

static struct amd64_opcode amd64_opcodes[4][256] = {
  /* Main table. */
{
  /* 0x00 */
  { "add", RMR8, }, { "add", RMR, }, { "add", RRM8, }, { "add", RRM, },
  { "add", ALI8, }, { "add", AI, }, { NULL, 0, }, { NULL, 0, },
  { "or", RMR8, }, { "or", RMR, }, { "or", RRM8, }, { "or", RRM, },
  { "or", ALI8, }, { "or", AI, }, { NULL, 0, }, { "F", OP_F, },

  /* 0x10 */
  { "adc", RMR8, }, { "adc", RMR, }, { "adc", RRM8, }, { "adc", RRM, },
  { "adc", ALI8, }, { "adc", AI, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0x20 */
  { "and", RMR8, }, { "and", RMR, }, { "and", RRM8, }, { "and", RRM, },
  { "and", ALI8, }, { "and", AI, }, { "es", G2, }, { NULL, 0, },
  { "sub", RMR8, }, { "sub", RMR, }, { "sub", RRM8, }, { "sub", RRM, },
  { "sub", ALI8, }, { "sub", AI, }, { "cs", G2, }, { NULL, 0, },

  /* 0x30 */
  { "xor", RMR8, }, { "xor", RMR, }, { "xor", RRM8, }, { "xor", RRM, },
  { NULL, 0, }, { NULL, 0, }, { "ss", G2, }, { NULL, 0, },
  { "cmp", RMR8, }, { "cmp", RMR, }, { "cmp", RRM8, }, { "cmp", RRM, },
  { "cmp", ALI8, }, { "cmp", AI, }, { "ds", G2, }, { NULL, 0, },

  /* 0x40 - REX */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0x50 */
  { "push", REG, }, { "push", REG, }, { "push", REG, }, { "push", REG, },
  { "push", REG, }, { "push", REG, }, { "push", REG, }, { "push", REG, },
  { "pop", REG, }, { "pop", REG, }, { "pop", REG, }, { "pop", REG, },
  { "pop", REG, }, { "pop", REG, }, { "pop", REG, }, { "pop", REG, },

  /* 0x60 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { "fs", G2, }, { "gs", G2, }, { "osize", G3, }, { "asize", G4, },
  { NULL, 0, }, { "imul", RRMI, }, { NULL, 0, }, { "imul", RRMI8, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0x70 */
  { "jo", PCREL8, }, { "jno", PCREL8, }, { "jb", PCREL8, }, { "jae", PCREL8, },
  { "jz", PCREL8, }, { "jnz", PCREL8, }, { "jbe", PCREL8, }, { "ja", PCREL8, },
  { "js", PCREL8, }, { "jns", PCREL8, }, { "jp", PCREL8, }, { "jpo", PCREL8, },
  { "jnge", PCREL8, }, { "jge", PCREL8, }, { "jle", PCREL8, }, { "jg", PCREL8, },

  /* 0x80 */
  { "and", RMI8OP, }, { "and", RMIOP, }, { NULL, 0, }, { "and", RMSI8OP, },
  { "test", RMR8, }, { "test", RMR, }, { NULL, 0, }, { NULL, 0, },
  { "mov", RMR8, }, { "mov", RMR, }, { "mov", RRM8, }, { "mov", RRM, },
  { "mov", RMS, }, { "lea", RRM, }, { "mov", SRM, }, { NULL, 0, },

  /* 0x90 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { "cbw/cwde/cdqe", OP_IMPLICIT_A, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0xa0 */
  { "mov", ALM8, }, { "mov", AM, }, { "mov", MAL8, }, { "mov", MA, },
  { NULL, 0, }, { NULL, 0, }, { "cmpsb", 0, }, { "cmps", 0, },
  { "test", ALI8, }, { "test", AI, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0xb0 */
  { "mov", RI8, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { "mov", RI, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0xc0 */
  { "sal", RMI8OP|1, }, { "sal", RMIOP|1, }, { NULL, 0, }, { "ret", 0, },
  { NULL, 0, }, { NULL, 0, }, { "mov", RMI8OP|4, }, { "mov", RMIOP|4, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0xd0 */
  { "sal", RM8OP|1, }, { "sal", RMOP|1, }, { "sal", RMC8OP|1, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { "jmp", PCREL, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { "jmp", PCREL8, }, { NULL, 0, },

  /* 0xe0 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { "jcxz", PCREL8, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { "call", PCREL, }, { NULL, 0, }, { NULL, 0, }, { "jmp", PCREL8, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0xf0 */
  { "lock", G1, }, { NULL, 0, }, { "repne", G1, }, { "rep", G1, },
  { NULL, 0, }, { "cmc", 0, }, { "neg", RM8OP|2, }, { "neg", RMOP|2, },
  { "clc", 0, }, { NULL, 0, }, { "cli", 0, }, { NULL, 0, },
  { "cld", 0, }, { NULL, 0, }, { "dec", RM8OP|3, }, { "dec", RMOP|3, },
},
/* Opcode prefix 0x0f. */
{
  /* 0x00 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { "clts", 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0x10 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0x20 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { "comiss/d", RRM, },

  /* 0x30 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { "aes|blendvps/d|crc32", OP_F_38, }, { NULL, 0, }, { "aes|blendps/d", OP_F_3A, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0x40 */
  { "cmovo", RRM, }, { "cmovno", RRM, }, { "cmovb", RRM, }, { "cmovae", RRM, },
  { "cmove", RRM, }, { "cmovne", RRM, }, { "cmovbe", RRM, }, { "cmova", RRM, },
  { "cmovs", RRM, }, { "cmovns", RRM, }, { "cmovp", RRM, }, { "cmovnp", RRM, },
  { "cmovel", RRM, }, { "cmovge", RRM, }, { "cmovle", RRM, }, { "cmovg", RRM, },

  /* 0x50 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { "andps/d", RRM, }, { "andnps/d", RRM, }, { NULL, 0, }, { NULL, 0, },
  { "add[p]s/d", RRM, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0x60 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { "movdqa", RRM, },

  /* 0x70 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { "movdqa", RMR, },

  /* 0x80 */
  { "jo", PCREL, }, { "jno", PCREL, }, { "jb", PCREL, }, { "jae", PCREL, },
  { "je", PCREL, }, { "jne", PCREL, }, { "jbe", PCREL, }, { "ja", PCREL, },
  { "js", PCREL, }, { "jns", PCREL, }, { "jp", PCREL, }, { "jnp", PCREL, },
  { "jl", PCREL, }, { "jge", PCREL, }, { "jle", PCREL, }, { "jg", PCREL, },

  /* 0x90 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { "eq", RM, }, { "neq", RM, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { "lt", RM, }, { "gte", RM, }, { "lte", RM, }, { "gt", RM, },

  /* 0xa0 */
  { NULL, 0, }, { NULL, 0, }, { "cpuid", 0, }, { "bt", RMR, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { "bts", RMR, },
  { NULL, 0, }, { NULL, 0, }, { "clflush", RM, }, { "imul", RRM, },

  /* 0xb0 */
  { "cmpxchg", RMR8, }, { "cmpxchg", RMR, }, { NULL, 0, }, { "btr", RMR, },
  { NULL, 0, }, { NULL, 0, }, { "movzx", RRM, }, { "movzx", RRM, },
  { "bsf", RRM, }, { "bsr", RRM, }, { "bt", RMIOP|5, }, { "btc", RMR, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0xc0 */
  { NULL, 0, }, { NULL, 0, }, { "cmpp/ss/d", RRMI8, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { "cmpxchg8b", RM, },
  { "bswap", REG, }, { "bswap", REG, }, { "bswap", REG, }, { "bswap", REG, },
  { "bswap", REG, }, { "bswap", REG, }, { "bswap", REG, }, { "bswap", REG, },

  /* 0xd0 */
  { "addsubps/d", RRM, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0xe0 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0xf0 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
},
/* Opcode prefix 0x0f 0x38: AES, BLENDVP, CRC32. */
{
  /* 0x00 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0x10 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { "blendvps", RRM, }, { "blendvpd", RRM, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0x20 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0x30 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0x40 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0x50 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0x60 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0x70 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0x80 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0x90 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0xa0 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0xb0 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0xc0 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0xd0 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { "aesimc", RRM, },
  { "aesenc", RRM, }, { "aesenclast", RRM, }, { "aesdec", RRM, }, { "aesdeclast", RRM, },

  /* 0xe0 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0xf0 */
  { "crc32", RRM8, }, { "crc32", RRM, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
},
/* Opcode prefix 0x0f 0x3a: AES, BLENDP. */
{
  /* 0x00 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { "blendps", RRMI8, }, { "blendpd", RRMI8, }, { NULL, 0, }, { NULL, 0, },

  /* 0x10 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0x20 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0x30 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0x40 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0x50 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0x60 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0x70 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0x80 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0x90 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0xa0 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0xb0 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0xc0 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0xd0 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { "aeskeygenassist", RRMI8, },

  /* 0xe0 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },

  /* 0xf0 */
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
  { NULL, 0, }, { NULL, 0, }, { NULL, 0, }, { NULL, 0, },
},
};

const char *amd64_registers[5][16] = {
  {	/* 8-bit registers with no REX. */
    "%al", "%cl", "%dl", "%bl", "%ah", "%ch", "%dh", "%bh",
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  },
  {	/* 8-bit registers with REX. */
    "%al", "%cl", "%dl", "%bl", "%spl", "%bpl", "%sil", "%dil",
    "%r8b", "%r9b", "%r10b", "%r11b", "%r12b", "%r13b", "%r14b", "%r15b",
  },
  {	/* 16-bit registers (0x66 prefix). */
    "%ax", "%cx", "%dx", "%bx", "%sp", "%bp", "%si", "%di",
    "%r8w", "%r9w", "%r10w", "%r11w", "%r12w", "%r13w", "%r14w", "%r15w",
  },
  {	/* 32-bit registers. */
    "%eax", "%ecx", "%edx", "%ebx", "%esp", "%ebp", "%esi", "%edi",
    "%r8d", "%r9d", "%r10d", "%r11d", "%r12d", "%r13d", "%r14d", "%r15d",
  },
  {	/* 64-bit registers (REX.W prefix). */
    "%rax", "%rcx", "%rdx", "%rbx", "%rsp", "%rbp", "%rsi", "%rdi",
    "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15",
  },
};

const char *amd64_describe_reg(int byte_mode, const int *legacy_prefix,
			       int rex, int reg_high, int reg)
{
  int reg_bank = 3;
  if (byte_mode) {
    reg_bank = 0;
    if (rex) reg_bank = 1;
  } else if (rex & 0x04) {
    reg_bank = 4;
  } else if (legacy_prefix[2] == 0x66) {
    reg_bank = 2;
  }
  if (reg_high) reg += 8;
  return amd64_registers[reg_bank][reg];
}

size_t amd64_readint32(PIKE_OPCODE_T *pc, char *buf)
{
  unsigned INT32 val = ((unsigned INT32 *)pc)[0];
  buf += strlen(buf);
  sprintf(buf, "0x%08x", val);
  return 4;
}

size_t amd64_disassemble_sib(PIKE_OPCODE_T *pc,
			     char *buf,
			     int byte_mode,
			     const int *legacy_prefix,
			     int modrm,
			     int rex)
{
  int sib = pc[0];
  int bytes = 1;
  buf[0] = 0;
  if (((sib & 0x07) == 0x05) && !(rex & 1)) {
    switch (modrm & 0xc0) {
    case 0x00:	/* Disp32 */
      {
	INT32 val = ((INT32 *)(pc + 1))[0];
	bytes += 4;
	if (val < 0) {
	  sprintf(buf, "-0x%x", -val);
	} else {
	  sprintf(buf, "0x%x", val);
	}
      }
      break;
    case 0x40:	/* Disp8 + [EBP] */
      {
	INT32 val = (pc + 1)[0];
	bytes++;
	if (val < 0) {
	  sprintf(buf, "-0x%x(%%rbp)", -val);
	} else {
	  sprintf(buf, "0x%x(%%rbp)", val);
	}
      }
      break;
    case 0x80:	/* Disp32 + [EBP] */
      {
	INT32 val = ((INT32 *)(pc + 1))[0];
	bytes += 4;
	if (val < 0) {
	  sprintf(buf, "-0x%x(%%rbp)", -val);
	} else {
	  sprintf(buf, "0x%x(%%rbp)", val);
	}
      }
      break;
    }
  } else {
    sprintf(buf, "%s", amd64_describe_reg(byte_mode, legacy_prefix, rex,
					  rex & 1, sib & 0x07));
  }
  if ((sib & 0x38) != 0x20) {
    if (sib & 0xc0) {
      /* Scale factor active */
      sprintf(buf + strlen(buf), "(%s*%d)",
	      amd64_describe_reg(byte_mode, legacy_prefix, rex,
				 rex & 2, (sib>>3) & 0x07),
	      1<<((sib>>6) & 3));
    } else {
      sprintf(buf + strlen(buf), "(%s)",
	      amd64_describe_reg(byte_mode, legacy_prefix, rex,
				 rex & 2, (sib>>3) & 0x07));
    }
  }
  return bytes;
}

size_t amd64_disassemble_modrm(PIKE_OPCODE_T *pc,
			       char *buf,
			       int byte_mode,
			       const int *legacy_prefix,
			       int modrm,
			       int rex)
{
  size_t bytes = 0;
  int mod;
  char reg_buf[256];

  if (legacy_prefix[3]) {
    /* Size prefix override. ==> 16 bits */
  }

  reg_buf[0] = 0;
  buf[0] = 0;
  switch(modrm & 0xc0) {
  default:
    if ((modrm & 0xc7) == 0x05) {
      /* RIP + disp32 */
      INT32 val = ((INT32 *)pc)[0];
      if (val < 0) {
	sprintf(buf, "-0x%x(%%rip)", -val);
      } else {
	sprintf(buf, "0x%x(%%rip)", val);
      }
      return 4;
    }
    if ((modrm & 0x07) == 0x04) {
      bytes += amd64_disassemble_sib(pc + bytes, reg_buf, byte_mode,
				     legacy_prefix, modrm, rex);
    } else {
      sprintf(reg_buf, "%s", amd64_describe_reg(byte_mode, legacy_prefix, rex,
						rex & 1, modrm & 7));
    }
    switch(modrm & 0xc0) {
    case 0x00:
      sprintf(buf, "(%s)", reg_buf);
      break;
    case 0x40:
      sprintf(buf, "%d(%s)", ((signed char *)pc)[bytes], reg_buf);
      bytes++;
      break;
    case 0x80:
      if (legacy_prefix[2] == 0x66) {
	sprintf(buf, "%d", ((INT16 *)(pc + bytes))[0]);
	bytes += 2;
      } else {
	bytes += amd64_readint32(pc + bytes, buf);
      }
      sprintf(buf + strlen(buf), "(%s)", reg_buf);
      break;
    }
    break;
  case 0xc0:
    sprintf(buf, "%s", amd64_describe_reg(byte_mode, legacy_prefix, rex,
					  rex & 1, modrm & 7));
    break;
  }
  return bytes;
}

void amd64_disassemble_code(PIKE_OPCODE_T *pc, size_t len)
{
  struct string_builder buf;
  size_t pos;

  init_string_builder(&buf, 0);
  for (pos = 0; pos < len;) {
    size_t op_start = pos;
    size_t i;
    const char *opcode = NULL;
    const char *params[4] = { NULL, NULL, NULL, NULL, };
    int legacy_prefix[4] = { 0, 0, 0, 0 };
    int byte;
    int rex = 0;
    int modrm = 0;
    int byte_mode = 0;
    struct amd64_opcode *table = amd64_opcodes[0];
    struct amd64_opcode *op;
    char buffers[4][256];

    // Handle prefixes.
    while(1) {
      byte = pc[pos++];
      op = table + byte;
      if (op->flags & OP_PREFIX) {
	legacy_prefix[op->flags & 3] = byte;
	continue;
      }
      break;
    }

    /* Handle REX */
    if ((byte & 0xf0) == 0x40) {
      rex = byte;
      byte = pc[pos++];
      op = table + byte;
    }

    while (op->flags & OP_MULTIBYTE) {
      table = amd64_opcodes[1 + (op->flags & 0xff)];
      byte = pc[pos++];
      op = table + byte;
    }

    opcode = op->name;
    byte_mode = op->flags & (OP_8|OP_S8);

    if (op->flags & OP_RM) {
      modrm = pc[pos++];
      params[0] = amd64_describe_reg(byte_mode, legacy_prefix, rex,
				     rex & 4, (modrm>>3) & 0x07);
      params[1] = buffers[1];
      pos += amd64_disassemble_modrm(pc + pos, buffers[1], byte_mode,
				     legacy_prefix, modrm, rex);
      if (op->flags & OP_OPS) {
	params[0] = NULL;
	opcode = modrm_ops[op->flags & 0x0f][(modrm >> 3) & 0x07];
      }
    }

    if (op->flags & OP_REG) {
      int reg = byte & 0x07;
      params[1] = amd64_describe_reg(byte_mode, legacy_prefix, rex,
				     rex & 1, byte & 0x07);
    }

    if (op->flags & OP_IMM) {
      if (!params[0]) {
	params[0] = buffers[0];
	buffers[0][0] = 0;
      }
      if (op->flags & (OP_8|OP_S8)) {
	sprintf(buffers[0] + strlen(buffers[0]), "$%d", ((signed char *)pc)[pos++]);
      } else if (legacy_prefix[2] == 0x66) {
	/* 16-bit */
	sprintf(buffers[1] + strlen(buffers[1]), "$%d", ((signed INT16 *)(pc + pos))[0]);
	pos += 2;
      } else {
	sprintf(buffers[0] + strlen(buffers[0]), "$");
	pos += amd64_readint32(pc + pos, buffers[0] + strlen(buffers[0]));
      }
    } else if (op->flags & OP_PCREL) {
      INT32 val;
      if (!params[0]) {
	params[0] = buffers[0];
	buffers[0][0] = 0;
      }
      if (op->flags & (OP_8|OP_S8)) {
	val = ((signed char *)pc)[pos++];
      } else {
	val = ((INT32 *)(pc + pos))[0];
	pos += 4;
      }
      sprintf(buffers[0] + strlen(buffers[0]), "%p", pc + (pos + val));
    }

    if (op->flags & OP_B_RM) {
      /* Swap the two parameters. */
      const char *tmp = params[0];
      params[0] = params[1];
      params[1] = tmp;
    }

    if (!params[0]) {
      params[0] = params[1];
      params[1] = NULL;
    }

    if (!opcode) {
      opcode = ".byte";
    }

    if (params[0]) {
      if ((params[0][0] == '%') &&
	  (!strcmp(opcode, "call") || !strcmp(opcode, "jmp"))) {
	/* NB: We know that these opcodes only have one parameter. */
	/* Add the star prefix. */
	sprintf(buffers[0], "*%s", params[0]);
	params[0] = buffers[0];
      }
    }

    string_builder_append_disassembly(&buf, (size_t)(pc + op_start),
				      pc + op_start, pc + pos,
				      opcode, params, NULL);
  }

  /* NUL-terminate. */
  string_builder_putchar(&buf, 0);
  fprintf(stderr, "%s", buf.s->str);
  free_string_builder(&buf);
}
