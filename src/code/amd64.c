/*
 * Machine code generator for AMD64.
 */

#include "operators.h"
#include "constants.h"
#include "object.h"
#include "builtin_functions.h"


/* Register encodings */
enum amd64_reg {REG_RAX = 0, REG_RBX = 3, REG_RCX = 1, REG_RDX = 2,
		REG_RSP = 4, REG_RBP = 5, REG_RSI = 6, REG_RDI = 7,
		REG_R8 = 8, REG_R9 = 9, REG_R10 = 10, REG_R11 = 11,
		REG_R12 = 12, REG_R13 = 13, REG_R14 = 14, REG_R15 = 15,
		REG_INVALID = -1,

                REG_XMM0 = 0, REG_XMM1 = 1, REG_XMM2 = 2, REG_XMM3 = 3,
                REG_XMM4 = 4, REG_XMM5 = 5, REG_XMM6 = 6, REG_XMM7 = 7,
                REG_XMM8 = 8, REG_XMM9 = 9, REG_XMM10=10, REG_XMM11=11,
                REG_XMM12=12, REG_XMM13=13, REG_XMM14=14, REG_XMM15=15,
               };

/* We reserve register r12 and above (as well as RSP, RBP and RBX). */
#define REG_BITMASK	((1 << REG_MAX) - 1)
#define REG_RESERVED	(REG_RSP|REG_RBP|REG_RBX)
#define REG_MAX			REG_R12
#define PIKE_MARK_SP_REG	REG_R12
#define PIKE_SP_REG		REG_R13
#define PIKE_FP_REG		REG_R14
#define Pike_interpreter_reg	REG_R15

#ifdef __NT__
/* From http://software.intel.com/en-us/articles/introduction-to-x64-assembly/
 *
 * Note: Space for the arguments needs to be allocated on the stack as well.
 */
#define ARG1_REG	REG_RCX
#define ARG2_REG	REG_RDX
#define ARG3_REG	REG_R8
#define ARG4_REG	REG_R9
#else
/* From SysV ABI for AMD64 draft 0.99.5. */
#define ARG1_REG	REG_RDI
#define ARG2_REG	REG_RSI
#define ARG3_REG	REG_RDX
#define ARG4_REG	REG_RCX
#define ARG5_REG	REG_R8
#define ARG6_REG	REG_R9
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
  add_to_program( x>>8 );
  add_to_program( x );
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
  if( (m&7) == REG_RSP)
    sib(0, REG_RSP, REG_RSP );
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
  else if( offset || (m&7) == REG_RSP || (m&7) == REG_RBP )
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

#define PUSH_INT(X) ins_int((INT32)(X), (void (*)(char))add_to_program)
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

static void and_reg_imm( enum amd64_reg reg, int imm32 )
{
  rex( 1, 0, 0, reg );

  if( imm32 < -0x80 || imm32 > 0x7f )
  {
    if( reg == REG_RAX )
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
    if( reg == REG_RAX )
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
  if( sreg != REG_RCX )
    Pike_fatal("Not supported\n");

  rex( 1, 0, 0, reg );
  opcode( 0xd3 );
  modrm( 3, 4, reg );
}

static void shl_reg32_reg( enum amd64_reg reg, enum amd64_reg sreg)
{
  if( sreg != REG_RCX )
    Pike_fatal("Not supported\n");
  rex( 0, 0, 0, reg );
  opcode( 0xd3 );
  modrm( 3, 4, reg );
}

static void shl_reg_mem( enum amd64_reg reg, enum amd64_reg mem, int offset)
{
  if( reg == REG_RCX )
    Pike_fatal("Not supported\n");
  mov_mem8_reg( mem, offset, REG_RCX );
  shl_reg_reg( reg, REG_RCX );
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

static void neg_reg( enum amd64_reg reg )
{
  rex(1,0,0,reg);
  opcode(0xf7);
  modrm(3,3,reg);
}

static void mov_imm_reg( long imm, enum amd64_reg reg )
{
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

static void mov_reg_mem16( enum amd64_reg from_reg, enum amd64_reg to_reg, ptrdiff_t offset )
{
  opcode( 0x66 );
  rex(0, from_reg, 0, to_reg );
  low_mov_reg_mem( from_reg, to_reg, offset );
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
    if( to_reg == REG_RAX )
      Pike_fatal( "Clobbered TMP REG_RAX reg\n");
    mov_imm_reg( imm, REG_RAX );
    mov_reg_mem( REG_RAX, to_reg, offset );
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
    if( reg == REG_RAX )
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

static void sub_reg_imm( enum amd64_reg reg, int imm32 )
{
  if( !imm32 ) return;

  if( imm32 < 0 )
    /* This makes the disassembly easier to read. */
    return add_reg_imm( reg, -imm32 );

  rex( 1, 0, 0, reg );

  if( imm32 < -0x80 || imm32 > 0x7f )
  {
    if( reg == REG_RAX )
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

static void test_reg( enum amd64_reg reg1 )
{
  test_reg_reg( reg1, reg1 );
}

static void cmp_reg_imm( enum amd64_reg reg, int imm32 )
{
  rex(1, 0, 0, reg);
  if( imm32 > 0x7f || imm32 < -0x80 )
  {
    if( reg == REG_RAX )
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
  rex(0, 0, 0, reg);
  if( imm32 > 0x7f || imm32 < -0x80 )
  {
    if( reg == REG_RAX )
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
  rex(1, reg1, 0, reg2);
  opcode( 0x39 );
  modrm( 3, reg1, reg2 );
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
    mov_imm_reg( addr>>3, REG_RAX);
    shl_reg_imm( REG_RAX, 3 );
  }
  else
  {
    mov_imm_reg(addr, REG_RAX );
  }
  call_reg( REG_RAX );
}

/* reg += reg2 */
static void add_reg_reg( enum amd64_reg reg, enum amd64_reg reg2 )
{
  rex(1,reg2,0,reg);
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
static void jc( struct label *l )  { return jump_rel8( l, 0x72 ); }
static void jnc( struct label *l ) { return jump_rel8( l, 0x73 ); }
static void jz( struct label *l )  { return jump_rel8( l, 0x74 ); }
static void jnz( struct label *l ) { return jump_rel8( l, 0x75 ); }
static void js( struct label *l )  { return jump_rel8( l, 0x78 ); }
static void jl( struct label *l )  { return jump_rel8( l, 0x7c ); }
static void jge( struct label *l ) { return jump_rel8( l, 0x7d ); }
static void jle( struct label *l ) { return jump_rel8( l, 0x7e ); }
static void jg( struct label *l )  { return jump_rel8( l, 0x7f ); }


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
  /* Push all registers that the ABI requires to be preserved. */
  push(REG_RBP);
  mov_reg_reg(REG_RSP, REG_RBP);
  push(REG_R15);
  push(REG_R14);
  push(REG_R13);
  push(REG_R12);
  push(REG_RBX);
  sub_reg_imm(REG_RSP, 8);	/* Align on 16 bytes. */
  mov_reg_reg(ARG1_REG, Pike_interpreter_reg);
  amd64_flush_code_generator_state();
}

void amd64_flush_code_generator_state(void)
{
  sp_reg = -1;
  fp_reg = -1;
  ret_for_func = 0;
  mark_sp_reg = -1;
  dirty_regs = 0;
  amd64_prev_stored_pc = -1;
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
void amd64_load_fp_reg(void)
{
  if (fp_reg < 0) {
    mov_mem_reg(Pike_interpreter_reg,
                OFFSETOF(Pike_interpreter_struct, frame_pointer),
                PIKE_FP_REG);
    fp_reg = PIKE_FP_REG;
  }
}

void amd64_load_sp_reg(void)
{
  if (sp_reg < 0) {
    mov_mem_reg(Pike_interpreter_reg,
                OFFSETOF(Pike_interpreter_struct, stack_pointer),
                PIKE_SP_REG);
    sp_reg = PIKE_SP_REG;
  }
}

void amd64_load_mark_sp_reg(void)
{
  if (mark_sp_reg < 0) {
    mov_mem_reg(Pike_interpreter_reg,
                OFFSETOF(Pike_interpreter_struct, mark_stack_pointer),
                PIKE_MARK_SP_REG);
    mark_sp_reg = PIKE_MARK_SP_REG;
  }
}

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
  add_reg_imm( sp_reg, sizeof(struct svalue)*num);
  dirty_regs |= 1 << PIKE_SP_REG;
  flush_dirty_regs(); /* FIXME: Need to change LABEL handling to remove this  */
}

static void amd64_add_mark_sp( int num )
{
  amd64_load_mark_sp_reg();
  add_reg_imm( mark_sp_reg, sizeof(struct svalue*)*num);
  dirty_regs |= 1 << PIKE_MARK_SP_REG;
  flush_dirty_regs(); /* FIXME: Need to change LABEL handling to remove this */
}

/* Note: Uses RAX and RCX internally. reg MUST not be REG_RAX. */
static void amd64_push_svaluep_to(int reg, int spoff)
{
  LABELS();
  if( reg == REG_RAX )
    Pike_fatal("Using RAX in push_svaluep not supported\n" );
  amd64_load_sp_reg();
  mov_mem_reg(reg, OFFSETOF(svalue, type), REG_RAX);
  mov_mem_reg(reg, OFFSETOF(svalue, u.refs), REG_RCX);
  mov_reg_mem(REG_RAX, sp_reg, spoff*sizeof(struct svalue)+OFFSETOF(svalue, type));
  and_reg_imm(REG_RAX, 0x1f);
  mov_reg_mem(REG_RCX, sp_reg, spoff*sizeof(struct svalue)+OFFSETOF(svalue, u.refs));
  cmp_reg32_imm(REG_RAX, MAX_REF_TYPE);
  jg(&label_A);
  add_imm_mem( 1, REG_RCX, OFFSETOF(pike_string, refs));
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
  mov_imm_mem((subtype<<16) + PIKE_T_INT, sp_reg, OFFSETOF(svalue, type));
  mov_imm_mem(value, sp_reg, OFFSETOF(svalue, u.integer));
  amd64_add_sp( 1 );
}

static void amd64_push_int_reg(enum amd64_reg reg )
{
  amd64_load_sp_reg();
  mov_imm_mem( PIKE_T_INT, sp_reg, OFFSETOF(svalue, type));
  mov_reg_mem( reg, sp_reg, OFFSETOF(svalue, u.integer));
  amd64_add_sp( 1 );
}

static void amd64_mark(int offset)
{
  amd64_load_sp_reg();
  amd64_load_mark_sp_reg();
  if (offset) {
    add_reg_imm_reg(sp_reg, -offset * sizeof(struct svalue), REG_RAX);
    mov_reg_mem(REG_RAX, mark_sp_reg, 0);
  } else {
    mov_reg_mem(sp_reg, mark_sp_reg, 0);
  }
   amd64_add_mark_sp( 1 );
}

static void mov_sval_type(enum amd64_reg src, enum amd64_reg dst )
{
  mov_mem8_reg( src, OFFSETOF(svalue,type), dst);
/*  and_reg32_imm( dst, 0x1f );*/
}


static void amd64_call_c_function(void *addr)
{
  flush_dirty_regs();
  call_imm(addr);
}

static void amd64_free_svalue(enum amd64_reg src, int guaranteed_ref )
{
  LABELS();
  if( src == REG_RAX )
    Pike_fatal("Clobbering RAX for free-svalue\n");
    /* load type -> RAX */
  mov_sval_type( src, REG_RAX );

  /* if RAX > MAX_REF_TYPE+1 */
  cmp_reg32_imm( REG_RAX,MAX_REF_TYPE);
  jg( &label_A );

  /* Load pointer to refs -> RAX */
  mov_mem_reg( src, OFFSETOF(svalue, u.refs), REG_RAX);
   /* if( !--*RAX ) */
  add_mem32_imm( REG_RAX, OFFSETOF(pike_string,refs),  -1);
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

/* Type already in RAX */
static void amd64_free_svalue_type(enum amd64_reg src, enum amd64_reg type,
                                   int guaranteed_ref )
{
  LABELS();
  /* if type > MAX_REF_TYPE+1 */
  if( src == REG_RAX )
    Pike_fatal("Clobbering RAX for free-svalue\n");

  cmp_reg32_imm(type,MAX_REF_TYPE);
  jg( &label_A );

  /* Load pointer to refs -> RAX */
  mov_mem_reg( src, OFFSETOF(svalue, u.refs), REG_RAX);
   /* if( !--*RAX ) */
  add_mem32_imm( REG_RAX, OFFSETOF(pike_string,refs),  -1);
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
  if( src == REG_RAX ) Pike_fatal("Clobbering src in ref_svalue\n");
  if( !already_have_type )
      mov_sval_type( src, REG_RAX );
  else
      and_reg_imm( REG_RAX, 0x1f );

  /* if RAX > MAX_REF_TYPE+1 */
  cmp_reg32_imm(REG_RAX, MAX_REF_TYPE );
  jg( &label_A );
  /* Load pointer to refs -> RAX */
  mov_mem_reg( src, OFFSETOF(svalue, u.refs), REG_RAX);
   /* *RAX++ */
  add_mem32_imm( REG_RAX, OFFSETOF(pike_string,refs),  1);
 LABEL_A;
}

void amd64_assign_local( int b )
{
  amd64_load_fp_reg();
  amd64_load_sp_reg();

  mov_mem_reg( fp_reg, OFFSETOF(pike_frame, locals), ARG1_REG);
  add_reg_imm( ARG1_REG,b*sizeof(struct svalue) );
  mov_reg_reg( ARG1_REG, REG_RBX );

  /* Free old svalue. */
  amd64_free_svalue(ARG1_REG, 0);

  /* Copy sp[-1] -> local */
  amd64_load_sp_reg();
  mov_mem_reg(sp_reg, -1*sizeof(struct svalue), REG_RAX);
  mov_mem_reg(sp_reg, -1*sizeof(struct svalue)+sizeof(long), REG_RCX);

  mov_reg_mem( REG_RAX, REG_RBX, 0 );
  mov_reg_mem( REG_RCX, REG_RBX, sizeof(long) );
}

static void amd64_pop_mark(void)
{
  amd64_add_mark_sp( -1 );
}

static void amd64_push_string(int strno, int subtype)
{
  amd64_load_fp_reg();
  amd64_load_sp_reg();
  mov_mem_reg(fp_reg, OFFSETOF(pike_frame, context), REG_RAX);
  mov_mem_reg(REG_RAX, OFFSETOF(inherit, prog), REG_RAX);
  mov_mem_reg(REG_RAX, OFFSETOF(program, strings), REG_RAX);
  mov_mem_reg(REG_RAX, strno * sizeof(struct pike_string *),  REG_RAX);
  mov_imm_mem((subtype<<16) | PIKE_T_STRING, sp_reg, OFFSETOF(svalue, type));
  mov_reg_mem(REG_RAX, sp_reg,(INT32)OFFSETOF(svalue, u.string));
  add_imm_mem( 1, REG_RAX, OFFSETOF(pike_string, refs));

  amd64_add_sp(1);
}

static void amd64_push_local_function(int fun)
{
  amd64_load_fp_reg();
  amd64_load_sp_reg();
  mov_mem_reg(fp_reg, OFFSETOF(pike_frame, context), REG_RAX);
  mov_mem_reg(fp_reg, OFFSETOF(pike_frame, current_object),
			    REG_RCX);
  mov_mem32_reg(REG_RAX, OFFSETOF(inherit, identifier_level),
			      REG_RAX);
  mov_reg_mem(REG_RCX, sp_reg, OFFSETOF(svalue, u.object));
  add_reg_imm(REG_RAX, fun);
  add_imm_mem( 1, REG_RCX,(INT32)OFFSETOF(object, refs));
  shl_reg_imm(REG_RAX, 16);
  add_reg_imm(REG_RAX, PIKE_T_FUNCTION);
  mov_reg_mem(REG_RAX, sp_reg, OFFSETOF(svalue, type));
  amd64_add_sp(1);
}

static void amd64_stack_error(void)
{
  Pike_fatal("Stack error\n");
}

void amd64_update_pc(void)
{
  INT32 tmp = PIKE_PC, disp;
  const enum amd64_reg tmp_reg = REG_RAX;

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
#if 0
#ifdef PIKE_DEBUG
  if (d_flag) {
    /* Check that the stack keeps being 16 byte aligned. */
    mov_reg_reg(REG_RSP, REG_RAX);
    and_reg_imm(REG_RAX, 0x08);
    AMD64_JE(0x09);
    call_imm(amd64_stack_error);
  }
#endif
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

  if (flags & I_UPDATE_SP) sp_reg = REG_INVALID;
  if (flags & I_UPDATE_M_SP) mark_sp_reg = REG_INVALID;
  if (flags & I_UPDATE_FP) fp_reg = REG_INVALID;
}

static void amd64_call_c_opcode(void *addr, int flags)
{
  sync_registers(flags);
  call_imm( addr );
}


#ifdef PIKE_DEBUG
static void ins_debug_instr_prologue (PIKE_INSTR_T instr, INT32 arg1, INT32 arg2)
{
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

  mov_imm_mem( PIKE_T_OBJECT, sp_reg, OFFSETOF(svalue,type));
  mov_mem_reg( fp_reg, OFFSETOF(pike_frame, current_object), REG_RAX );
  mov_reg_mem( REG_RAX, sp_reg, OFFSETOF(svalue,u.object) );
  add_mem32_imm( REG_RAX, (INT32)OFFSETOF(object, refs), 1);
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
      clear_reg( REG_RAX );
      add_mem32_imm( REG_RAX,
                     (int)(ptrdiff_t)&fast_check_threads_counter,
                     0x80 );
    }
    else
    {
      mov_imm_reg( (long)&fast_check_threads_counter, REG_RAX);
      add_mem_imm( REG_RAX, 0, 0x80 );
    }
    mov_imm_reg( (ptrdiff_t)branch_check_threads_etc, REG_RAX );
    jmp_reg(REG_RAX); /* ret in BCTE will return to desired point. */
    amd64_align();
  }
  if( !code_only )
  {
    LABEL_A;
    /* Use C-stack for counter. We have padding added in entry */
    add_mem8_imm( REG_RSP, 0, 1 );
    jno( &label_B );
    call_rel_imm32( branch_check_threads_update_etc );
    LABEL_B;
  }
}


void amd64_init_interpreter_state(void)
{
  instrs[F_CATCH - F_OFFSET].address = inter_return_opcode_F_CATCH;
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
    pop(REG_RBX);	/* Stack padding. */
    pop(REG_RBX);
    pop(REG_R12);
    pop(REG_R13);
    pop(REG_R14);
    pop(REG_R15);
    pop(REG_RBP);
    ret();
  }
}

void ins_f_byte(unsigned int b)
{
  int flags;
  void *addr;
  INT32 rel_addr = 0;
  LABELS();

  b-=F_OFFSET;
#ifdef PIKE_DEBUG
  if(b>255)
    Pike_error("Instruction too big %d\n",b);
#endif
  maybe_update_pc();

  flags = instrs[b].flags;

  addr=instrs[b].address;
  switch(b + F_OFFSET)
  {
  case F_DUP:
    ins_debug_instr_prologue(b, 0, 0);
    amd64_load_sp_reg();
    add_reg_imm_reg(sp_reg, -sizeof(struct svalue), REG_R10 );
    amd64_push_svaluep( REG_R10 );
    return;

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
      mov_mem8_reg( sp_reg, -3*sizeof(struct svalue), REG_RBX );
      cmp_reg_imm( REG_RBX, T_SVALUE_PTR );
      jne( &label_A );

      /* inline version for SVALUE_PTR. */

      /* First, make room for the result, move top and inc sp */
      mov_mem_reg( sp_reg, -sizeof(struct svalue), REG_RAX );
      mov_mem_reg( sp_reg, -sizeof(struct svalue)+8, REG_RCX );
      mov_reg_mem( REG_RAX, sp_reg, 0);
      mov_reg_mem( REG_RCX, sp_reg, 8);
      amd64_add_sp( 1 );

      mov_mem_reg( sp_reg,
                   -4*sizeof(struct svalue)+OFFSETOF(svalue,u.lval),
                   REG_RBX );
      amd64_push_svaluep_to(REG_RBX, -2);

      /* Result is now in sp[-2], lval in -4. */
      /* push svaluep leaves type in RAX */
      mov_reg_reg( REG_RAX, REG_RCX );
      /* mov_mem8_reg( sp_reg, -2*sizeof(struct svalue), REG_RCX ); */
      mov_imm_reg( 1, REG_RAX );
      shl_reg32_reg( REG_RAX, REG_RCX );
      and_reg_imm( REG_RAX, (BIT_ARRAY|BIT_MULTISET|BIT_MAPPING|BIT_STRING));
      jz( &label_B ); /* Do nothing.. */

      /* Free the old value. */
      amd64_free_svalue( REG_RBX, 0 );
      /* assign 0 */
      mov_imm_mem( PIKE_T_INT, REG_RBX, 0 );
      mov_imm_mem( 0, REG_RBX, 8 );
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
      mov_mem8_reg( sp_reg, -sizeof(struct svalue)*2, REG_RAX );
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
      cmp_reg_imm( REG_RAX, T_SVALUE_PTR );
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
                   REG_RCX );
      amd64_push_svaluep(REG_RCX);
      LABEL_B;
    }
    return;
  case F_ASSIGN:
   {
     ins_debug_instr_prologue(b, 0, 0);
      amd64_load_sp_reg();
      mov_mem8_reg( sp_reg, -3*sizeof(struct svalue), REG_RAX );
      cmp_reg_imm( REG_RAX, T_SVALUE_PTR );

      je( &label_A );
      /* So, not a svalueptr. Use C-version for simplicity */
      amd64_call_c_opcode( addr, flags );
      amd64_load_sp_reg();
      jmp( &label_B );
      amd64_align();

     LABEL_A;
      mov_mem_reg( sp_reg, -3*sizeof(struct svalue)+8, REG_RBX );

      /* Free old value. */
      amd64_free_svalue( REG_RBX, 0 );

      /* assign new value */
      /* also move assigned value -> sp[-3] */
      mov_mem_reg( sp_reg, -sizeof(struct svalue), REG_RAX );
      mov_mem_reg( sp_reg, -sizeof(struct svalue)+8, REG_RCX );
      mov_reg_mem( REG_RAX, REG_RBX, 0 );
      mov_reg_mem( REG_RCX, REG_RBX, 8 );
      add_reg_imm_reg( sp_reg, -sizeof(struct svalue), REG_RCX );
      amd64_push_svaluep_to( REG_RCX, -3 );
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
      mov_mem8_reg( sp_reg, -3*sizeof(struct svalue), REG_RAX );
      cmp_reg_imm( REG_RAX, T_SVALUE_PTR );

      je( &label_A );
      /* So, not a svalueptr. Use C-version for simplicity */
      amd64_call_c_opcode( addr, flags );
      amd64_load_sp_reg();
      jmp( &label_B );
      amd64_align();

     LABEL_A;
      mov_mem_reg( sp_reg, -3*sizeof(struct svalue)+8, REG_RBX );

      /* Free old value. */
      amd64_free_svalue( REG_RBX, 0 );

      /* assign new value */
      mov_mem_reg( sp_reg, -sizeof(struct svalue), REG_RAX );
      mov_mem_reg( sp_reg, -sizeof(struct svalue)+8, REG_RCX );
      mov_reg_mem( REG_RAX, REG_RBX, 0 );
      mov_reg_mem( REG_RCX, REG_RBX, 8 );
      /*
        Note: For SVALUEPTR  we know we do not have to free
        the lvalue.
      */
      amd64_add_sp( -3 );
     LABEL_B;
   }
   return;
   return;
  case F_ADD_INTS:
    {
      ins_debug_instr_prologue(b, 0, 0);
      amd64_load_sp_reg();
      mov_mem8_reg( sp_reg, -sizeof(struct svalue)*2, REG_RAX );
      shl_reg_imm( REG_RAX, 8 );
      mov_mem8_reg( sp_reg,-sizeof(struct svalue), REG_RBX );
     /* and_reg_imm( REG_RBX, 0x1f );*/
      add_reg_reg( REG_RAX, REG_RBX );
      /* and_reg_imm( REG_RAX, 0x1f1f ); */
      cmp_reg32_imm( REG_RAX, (PIKE_T_INT<<8)|PIKE_T_INT );
      jne( &label_A );
      /* So. Both are actually integers. */
      mov_mem_reg( sp_reg,
                   -sizeof(struct svalue)+OFFSETOF(svalue,u.integer),
                   REG_RAX );

      add_reg_mem( REG_RAX,
                   sp_reg,
                   -sizeof(struct svalue)*2+OFFSETOF(svalue,u.integer)
                 );

      jo( &label_A );
      amd64_add_sp( -1 );
      mov_imm_mem( PIKE_T_INT,sp_reg, -sizeof(struct svalue));
      mov_reg_mem( REG_RAX, sp_reg,
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

  case F_SWAP:
    /*
      pike_sp[-1] = pike_sp[-2]
    */
    ins_debug_instr_prologue(b, 0, 0);
    amd64_load_sp_reg();
    add_reg_imm_reg( sp_reg, -2*sizeof(struct svalue), REG_RCX );
    mov_mem128_reg( REG_RCX, 0, REG_XMM0 );
    mov_mem128_reg( REG_RCX, 16, REG_XMM1 );
    mov_reg_mem128( REG_XMM1, REG_RCX, 0 );
    mov_reg_mem128( REG_XMM0, REG_RCX, 16 );
#if 0
    add_reg_imm_reg( sp_reg, -2*sizeof(struct svalue), REG_R10);
    mov_mem_reg( REG_R10, 0, REG_RAX );
    mov_mem_reg( REG_R10, 8, REG_RCX );
    mov_mem_reg( REG_R10,16, REG_R8 );
    mov_mem_reg( REG_R10,24, REG_R9 );
    /* load done. */
    mov_reg_mem(REG_R8,  REG_R10,0);
    mov_reg_mem(REG_R9,  REG_R10,8);
    mov_reg_mem(REG_RAX, REG_R10,sizeof(struct svalue));
    mov_reg_mem(REG_RCX, REG_R10,8+sizeof(struct svalue));
    /* save done. */
#endif
    return;

  case F_INDEX:
    /*
      pike_sp[-2][pike_sp[-1]]
    */
    ins_debug_instr_prologue(b, 0, 0);
    amd64_load_sp_reg();
    mov_mem8_reg( sp_reg, -2*sizeof(struct svalue), REG_RAX );
    mov_mem8_reg( sp_reg, -1*sizeof(struct svalue), REG_RBX );
    shl_reg_imm( REG_RAX, 8 );
    add_reg_reg( REG_RAX, REG_RBX );
    mov_mem_reg( sp_reg, -1*sizeof(struct svalue)+8, REG_RBX ); /* int */
    mov_mem_reg( sp_reg, -2*sizeof(struct svalue)+8, REG_RCX ); /* value */
    cmp_reg32_imm( REG_RAX, (PIKE_T_ARRAY<<8)|PIKE_T_INT );
    jne( &label_A );

    /* Array and int index. */
    mov_mem32_reg( REG_RCX, OFFSETOF(array,size), REG_RDX );
    cmp_reg32_imm( REG_RBX, 0 ); jge( &label_D );
    /* less than 0, add size */
    add_reg_reg( REG_RBX, REG_RDX );

   LABEL_D;
    cmp_reg32_imm( REG_RBX, 0 ); jl( &label_B ); // <0
    cmp_reg_reg( REG_RBX, REG_RDX); jge( &label_B ); // >size

    /* array, index inside array. push item, swap, pop, done */
    push( REG_RCX ); /* Save array pointer */
    mov_mem_reg( REG_RCX, OFFSETOF(array,item), REG_RCX );
    shl_reg_imm( REG_RBX, 4 );
    add_reg_reg( REG_RBX, REG_RCX );
    /* This overwrites the array. */
    amd64_add_sp( -1 );
    amd64_push_svaluep_to( REG_RBX, -1 );

    pop( REG_RCX );
    /* We know it's an array. */
    add_mem32_imm( REG_RCX, OFFSETOF(array,refs),  -1);
    jnz( &label_C );
    mov_reg_reg( REG_RCX, ARG1_REG );
    amd64_call_c_function(really_free_array);
    jmp( &label_C );

   LABEL_A;
#if 0
    cmp_reg32_imm( REG_RAX, (PIKE_T_STRING<<8)|PIKE_T_INT );
    jne( &label_B );
#endif
   LABEL_B;
    /* something else. */
    amd64_call_c_opcode( addr, flags );
    amd64_load_sp_reg();
   LABEL_C;
    /* done */
    return;

  case F_SIZEOF:
    {
      LABELS();
      ins_debug_instr_prologue(b, 0, 0);
      amd64_load_sp_reg();
      add_reg_imm_reg( sp_reg, -sizeof(struct svalue), REG_RBX);
      mov_sval_type( REG_RBX, REG_RAX );
      /* type in RAX, svalue in ARG1 */
      cmp_reg32_imm( REG_RAX, PIKE_T_ARRAY ); jne( &label_A );
      /* It's an array */
      mov_mem_reg( REG_RBX, OFFSETOF(svalue, u.array ), ARG1_REG);
      /* load size -> RAX*/
      mov_mem32_reg( ARG1_REG,OFFSETOF(array, size), REG_RAX );
      jmp( &label_C );

     LABEL_A;
      cmp_reg32_imm( REG_RAX, PIKE_T_STRING );  jne( &label_B );
      /* It's a string */
      mov_mem_reg( REG_RBX, OFFSETOF(svalue, u.string ), ARG1_REG);
      /* load size ->RAX*/
      mov_mem32_reg( ARG1_REG,OFFSETOF(pike_string, len ), REG_RAX );
      jmp( &label_C );
     LABEL_B;
      /* It's something else, svalue in RBX. */
      mov_reg_reg( REG_RBX, ARG1_REG );
      amd64_call_c_function( pike_sizeof );

     LABEL_C;/* all done, res in RAX */
      /* free value, store result */
      push( REG_RAX );
      amd64_free_svalue( REG_RBX, 0 );
      pop( REG_RAX );
      mov_reg_mem(REG_RAX,    REG_RBX, OFFSETOF(svalue, u.integer));
      mov_imm_mem(PIKE_T_INT, REG_RBX, OFFSETOF(svalue, type));
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
    {
      /* Special argument for the F_CATCH instruction. */
      addr = inter_return_opcode_F_CATCH;
      mov_rip_imm_reg(0, ARG1_REG);	/* Address for the POINTER. */
      rel_addr = PIKE_PC;
    }
    break;
  case F_ZERO_TYPE:
    {
      LABELS();
      ins_debug_instr_prologue(b, 0, 0);
      amd64_load_sp_reg();
      mov_mem32_reg( sp_reg, -sizeof(struct svalue), REG_RAX );
      /* Rax now has type + subtype. */
      mov_reg_reg( REG_RAX, REG_RBX );
      and_reg_imm( REG_RAX, 0x1f );
      cmp_reg32_imm( REG_RAX, PIKE_T_INT );
      jne( &label_A );
      /* It is an integer. */
      shr_reg_imm( REG_RBX, 16 );
      /* subtype in RBX. */
      mov_imm_mem( PIKE_T_INT, sp_reg, -sizeof(struct svalue) );
      mov_reg_mem( REG_RBX,    sp_reg,
                   -sizeof(struct svalue)+OFFSETOF(svalue,u.integer) );
      jmp( &label_B );
      LABEL_A;
      /* not an integer. Use C version for simplicitly.. */
      amd64_call_c_opcode( addr, flags );
      LABEL_B;
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
  case F_ADD:
    ins_debug_instr_prologue(b, 0, 0);
    update_arg1(2);
    addr = f_add;
    break;
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
    mov_mem_reg(mark_sp_reg, 0, REG_RBX);
    jmp(&label_A);
    LABEL_B;
    amd64_add_sp( -1 );
    amd64_free_svalue( sp_reg, 0 );
    amd64_load_sp_reg();
    LABEL_A;
    cmp_reg_reg(REG_RBX, sp_reg);
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
    mov_mem32_reg( fp_reg, OFFSETOF(pike_frame, flags), REG_RAX );
    and_reg_imm( REG_RAX, PIKE_FRAME_RETURN_INTERNAL);
    jnz( &label_A );
    /* So, it is just a normal return. */
    LABEL_B;
    /* Actually return */
    flush_dirty_regs();
    amd64_return_from_function();

    LABEL_A;
    amd64_call_c_opcode(addr,flags);
#if 0
    /* Can this happen?

       It seems to work without it, and from the code it looks like it
       should never happen, so..
    */
    cmp_reg_imm(REG_RAX, -1);
    je(&label_B);
#endif
    jmp_reg(REG_RAX);
    }
    return;
  case F_CLEAR_STRING_SUBTYPE:
    ins_debug_instr_prologue(b, 0, 0);
    amd64_load_sp_reg();
    mov_mem32_reg(sp_reg, OFFSETOF(svalue, type) - sizeof(struct svalue),
		  REG_RAX);
    /* NB: We only care about subtype 1! */
    cmp_reg32_imm(REG_RAX, (1<<16)|PIKE_T_STRING);
    jne(&label_A);
    and_reg_imm(REG_RAX, 0x1f);
    mov_reg_mem32(REG_RAX,
		  sp_reg, OFFSETOF(svalue, type) - sizeof(struct svalue));
    LABEL_A;
    return;
  }
  amd64_call_c_opcode(addr,flags);

  if (instrs[b].flags & I_RETURN) {
    LABELS();

    if ((b + F_OFFSET) == F_RETURN_IF_TRUE) {
      /* Kludge. We must check if the ret addr is
       * PC + JUMP_EPILOGUE_SIZE. */
      mov_rip_imm_reg(JUMP_EPILOGUE_SIZE, REG_RCX);
    }
    cmp_reg_imm(REG_RAX, -1);
   jne(&label_A);
    amd64_return_from_function();
   LABEL_A;

    if ((b + F_OFFSET) == F_RETURN_IF_TRUE) {
      /* Kludge. We must check if the ret addr is
       * orig_addr + JUMP_EPILOGUE_SIZE. */
      cmp_reg_reg( REG_RAX, REG_RCX );
      je( &label_B );
      jmp_reg(REG_RAX);
      LABEL_B;
      return;
    }
  }
  if (flags & I_JUMP) {
    jmp_reg(REG_RAX);

    if (b + F_OFFSET == F_CATCH) {
      upd_pointer(rel_addr - 4, PIKE_PC - rel_addr);
    }
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
  if(off>255)
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
      mov_mem_reg( sp_reg, 8, REG_RAX );
      test_reg(REG_RAX);
      if( op == F_QUICK_BRANCH_WHEN_ZERO )
        return jz_imm_rel32(0);
      return jnz_imm_rel32(0);

    case F_BRANCH_WHEN_ZERO:
    case F_BRANCH_WHEN_NON_ZERO:
      START_JUMP();
      amd64_load_sp_reg();
      mov_mem8_reg( sp_reg, -sizeof(struct svalue),  REG_RCX );
      cmp_reg32_imm( REG_RCX, PIKE_T_INT ); je( &label_C );
      mov_imm_reg( 1, REG_RAX );
      shl_reg32_reg( REG_RAX, REG_RCX );
      and_reg32_imm( REG_RAX, BIT_FUNCTION|BIT_OBJECT );
      jnz( &label_A );

      /* string, array, mapping or float. Always true */
      amd64_add_sp(-1);
      amd64_free_svalue_type( sp_reg, REG_RCX, 0 );
      mov_imm_reg( 1, REG_RBX );
      jmp( &label_B );

     LABEL_A;
      /* function or object. Use svalue_is_true. */
      add_reg_imm_reg(sp_reg, -sizeof(struct svalue), ARG1_REG );
      amd64_call_c_function(svalue_is_true);
      amd64_load_sp_reg();
      mov_reg_reg( REG_RAX, REG_RBX );
      amd64_add_sp( -1 );
      amd64_free_svalue( sp_reg, 0 ); /* Pop the stack. */
      jmp( &label_B );

     LABEL_C;
      /* integer */
      mov_mem_reg( sp_reg,
                   -sizeof(struct svalue)+OFFSETOF(svalue,u.integer),
                   REG_RBX );
      amd64_add_sp(-1);

     LABEL_B; /* Branch or not? */
      test_reg( REG_RBX );
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
      mov_mem8_reg( sp_reg, -4*sizeof(struct svalue), REG_RBX );
      cmp_reg32_imm( REG_RBX, PIKE_T_ARRAY );
      jne(&label_D);
      mov_mem_reg( sp_reg, -1*sizeof(struct svalue)+8, REG_RAX );
      mov_mem_reg( sp_reg, -4*sizeof(struct svalue)+8, REG_RBX );
      mov_mem32_reg( REG_RBX, OFFSETOF(array,size), REG_RCX );
      cmp_reg_reg( REG_RAX, REG_RCX );
      je(&label_A);

      /* increase counter */
      add_mem_imm( sp_reg, -1*(int)sizeof(struct svalue)+8, 1 );

      /* get item */
      mov_mem_reg( REG_RBX, OFFSETOF(array,item), REG_RBX );
      shl_reg_imm( REG_RAX, 4 );
      add_reg_reg( REG_RBX, REG_RAX );

      mov_mem8_reg( sp_reg, -3*sizeof(struct svalue), REG_RAX );
      cmp_reg_imm( REG_RAX,T_SVALUE_PTR);
      jne( &label_C );

      /* SVALUE_PTR optimization */
      mov_mem_reg( sp_reg, -3*sizeof(struct svalue)+8, REG_RDX );
      push( REG_RDX );
      /* Free old value. */
      amd64_free_svalue( REG_RDX, 0 );
      pop( REG_RDX );

      /* Assign new value. */
      mov_mem_reg( REG_RBX, 0, REG_RAX );
      mov_mem_reg( REG_RBX, 8, REG_RCX );
      mov_reg_mem( REG_RAX, REG_RDX, 0 );
      mov_reg_mem( REG_RCX, REG_RDX, 8 );

      /* inc refs? */
      and_reg_imm( REG_RAX, 0x1f );
      cmp_reg32_imm(REG_RAX, MAX_REF_TYPE);
      jg( &label_B );
      add_imm_mem( 1, REG_RCX, OFFSETOF(pike_string, refs));
      jmp( &label_B );

     LABEL_C;
      add_reg_imm_reg( sp_reg, -3*sizeof(struct svalue), ARG1_REG );
      mov_reg_reg( REG_RBX, ARG2_REG );
      amd64_call_c_function( assign_lvalue );
      jmp(&label_B);

     LABEL_D;
      /* Bad arg 1. Let the C opcode throw the error. */
      amd64_call_c_opcode(instrs[off].address, flags);
      /* NOT_REACHED */

     LABEL_A;
      mov_imm_reg( 0, REG_RBX );
     LABEL_B;
      test_reg(REG_RBX);
      return jnz_imm_rel32(0);

    case F_LOOP:
      START_JUMP();
      /* counter in pike_sp-1 */
      /* decrement until 0. */
      /* if not 0, branch */
      /* otherwise, pop */
      amd64_load_sp_reg();
      mov_mem32_reg( sp_reg, -sizeof(struct svalue), REG_RAX );
      /* Is it a normal integer? subtype -> 0, type -> PIKE_T_INT */
      cmp_reg32_imm( REG_RAX, PIKE_T_INT );
      jne( &label_A );

      /* if it is, is it 0? */
      mov_mem_reg( sp_reg, -sizeof(struct svalue)+8, REG_RAX );
      test_reg(REG_RAX);
      jz( &label_B ); /* it is. */

      add_reg_imm( REG_RAX, -1 );
      mov_reg_mem( REG_RAX, sp_reg, -sizeof(struct svalue)+8);
      mov_imm_reg( 1, REG_RAX );
      /* decremented. Jump -> true. */

      /* This is where we would really like to have two instances of
       * the target returned from this function...
       */
      jmp( &label_C );

      LABEL_A; /* Not an integer. */
      amd64_call_c_opcode(instrs[F_LOOP-F_OFFSET].address,
                          instrs[F_LOOP-F_OFFSET].flags );
      jmp( &label_C );

      /* result in RAX */
      LABEL_B; /* loop done, inline. Known to be int, and 0 */
      amd64_add_sp( -1 );
      mov_imm_reg(0, REG_RAX );

      LABEL_C; /* Branch or not? */
      test_reg( REG_RAX );
      return jnz_imm_rel32(0);

    case F_BRANCH_WHEN_EQ: /* sp[-2] != sp[-1] */
    case F_BRANCH_WHEN_NE: /* sp[-2] != sp[-1] */
/*      START_JUMP();*/
      ins_debug_instr_prologue(op, 0, 0);
      amd64_load_sp_reg();
      mov_mem16_reg( sp_reg, -sizeof(struct svalue),  REG_RCX );
      mov_mem16_reg( sp_reg, -sizeof(struct svalue)*2,REG_RBX );
      cmp_reg_reg( REG_RCX, REG_RBX );
      jnz( &label_A ); /* Types differ */
      
      /* Fallback to C for functions, objects and floats. */
      mov_imm_reg(1, REG_RBX);
      shl_reg32_reg(REG_RBX, REG_RCX);
      and_reg_imm(REG_RBX, (BIT_FUNCTION|BIT_OBJECT|BIT_FLOAT));
      jnz( &label_A );
      
      mov_mem_reg( sp_reg, -sizeof(struct svalue)+8,  REG_RBX );
      sub_reg_mem( REG_RBX, sp_reg, -sizeof(struct svalue)*2+8);
      /* RBX will now be 0 if they are equal.*/

      /* Optimization: The types are equal, pop_stack can be greatly
       * simplified if they are <= max_ref_type */
      cmp_reg32_imm( REG_RCX,MAX_REF_TYPE+1);
      jl( &label_B );
      /* cheap pop. We know that both are > max_ref_type */
      amd64_add_sp( -2 );
      jmp( &label_D );

     LABEL_A; /* Fallback - call opcode. */
      amd64_call_c_opcode( instrs[F_BRANCH_WHEN_NE-F_OFFSET].address,
                           instrs[F_BRANCH_WHEN_NE-F_OFFSET].flags );
      amd64_load_sp_reg();
      /* Opcode returns 0 if equal, -1 if not. */
      mov_reg_reg(REG_RAX, REG_RBX);
      jmp(&label_D);

    LABEL_B; /* comparison done, pop stack x2, reftypes */
     amd64_add_sp( -2 );

     mov_mem_reg( sp_reg, OFFSETOF(svalue,u.refs),  REG_RAX );
     add_mem32_imm( REG_RAX, OFFSETOF(pike_string,refs), -1 );
     jnz( &label_C );
     /* Free sp_reg */
     mov_reg_reg( sp_reg, ARG1_REG );
     amd64_call_c_function( really_free_svalue );
     amd64_load_sp_reg();
    LABEL_C;
     add_reg_imm_reg( sp_reg, sizeof(struct svalue), ARG1_REG );
     mov_mem_reg( ARG1_REG, OFFSETOF(svalue,u.refs),  REG_RAX );
     add_mem32_imm( REG_RAX, OFFSETOF(pike_string,refs), -1 );
     jnz( &label_D );
     amd64_call_c_function( really_free_svalue );
     /* free sp[-2] */
    LABEL_D;
     test_reg(REG_RBX);
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
      ret=DO_NOT_WARN( (INT32) PIKE_PC );
      PUSH_INT(0);
      return ret;
  }

  maybe_update_pc();
  addr=instrs[off].address;
  amd64_call_c_opcode(addr, flags);

  amd64_load_sp_reg();
  test_reg(REG_RAX);

  if (backward_jump) {
    INT32 skip;
    add_to_program (0x74);	/* jz rel8 */
    add_to_program (0);		/* Bytes to skip. */
    skip = (INT32)PIKE_PC;
    amd64_ins_branch_check_threads_etc(0);
    add_to_program (0xe9);	/* jmp rel32 */
    ret = DO_NOT_WARN ((INT32) PIKE_PC);
    PUSH_INT (0);
    /* Adjust the skip for the relative jump. */
    Pike_compiler->new_program->program[skip-1] = ((INT32)PIKE_PC - skip);
  }
  else {
    add_to_program (0x0f);	/* jnz rel32 */
    add_to_program (0x85);
    ret = DO_NOT_WARN ((INT32) PIKE_PC);
    PUSH_INT (0);
  }

  return ret;
}

void ins_f_byte_with_arg(unsigned int a, INT32 b)
{
  maybe_update_pc();
  switch(a) {
  case F_THIS_OBJECT:
    if( b == 0 )
    {
      ins_debug_instr_prologue(a-F_OFFSET, b, 0);
      amd64_push_this_object();
      return;
    }
    break; /* Fallback to C-version. */

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
      mov_mem8_reg( sp_reg, -1*sizeof(struct svalue),  REG_RAX );
      cmp_reg32_imm( REG_RAX, PIKE_T_ARRAY );
      jne( &label_A );

      mov_mem_reg( sp_reg,  -1*sizeof(struct svalue)+8, REG_RDX ); /* u.array */
      /* -> arr[sizeof(arr)-b] */
      mov_mem32_reg( REG_RDX, OFFSETOF(array,size), REG_RBX );
      sub_reg_imm( REG_RBX, b );
      js( &label_A ); /* if signed result, index outside array */

      shl_reg_imm( REG_RBX, 4 );
      add_reg_mem( REG_RBX, REG_RDX, OFFSETOF(array,item) );

      /* This overwrites the array. */
      amd64_push_svaluep_to( REG_RBX, -1 );

      /* We know it's an array. */
      add_mem32_imm( REG_RDX, OFFSETOF(array,refs),  -1);
      jnz( &label_C );
      mov_reg_reg( REG_RDX, ARG1_REG );
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
      mov_mem8_reg( sp_reg, -1*sizeof(struct svalue),  REG_RAX );
      cmp_reg32_imm( REG_RAX, PIKE_T_ARRAY );
      jne( &label_A );

      mov_mem_reg( sp_reg,  -1*sizeof(struct svalue)+8, REG_RDX ); /* u.array */
      /* -> arr[sizeof(arr)-b] */
      mov_mem32_reg( REG_RDX, OFFSETOF(array,size), REG_RCX );
      mov_imm_reg( b, REG_RBX);
      cmp_reg_reg( REG_RCX, REG_RBX );
      jg( &label_A ); /* b > RBX, index outside array */
      shl_reg_imm( REG_RBX, 4 );
      add_reg_mem( REG_RBX, REG_RDX, OFFSETOF(array,item) );

      /* This overwrites the array. */
      amd64_push_svaluep_to( REG_RBX, -1 );

      /* We know it's an array. */
      add_mem32_imm( REG_RDX, OFFSETOF(array,refs),  -1);
      jnz( &label_C );
      mov_reg_reg( REG_RDX, ARG1_REG );
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
        pike_sp[-2][pike_sp[-1]]
      */
      ins_debug_instr_prologue(a-F_OFFSET, b, 0);
      amd64_load_sp_reg();
      amd64_load_fp_reg();

      mov_mem_reg( fp_reg, OFFSETOF(pike_frame,locals), REG_RDX);
      add_reg_imm( REG_RDX, b*sizeof(struct svalue));
      mov_mem8_reg( REG_RDX, 0, REG_RAX );
      mov_mem8_reg( sp_reg, -1*sizeof(struct svalue), REG_RBX );
      shl_reg_imm( REG_RAX, 8 );
      add_reg_reg( REG_RAX, REG_RBX );
      mov_mem_reg( sp_reg, -1*sizeof(struct svalue)+8, REG_RBX ); /* int */
      mov_mem_reg( REG_RDX, 8, REG_RCX );                         /* value */
      cmp_reg32_imm( REG_RAX, (PIKE_T_ARRAY<<8)|PIKE_T_INT );
      jne( &label_A );

      /* Array and int index. */
      mov_mem32_reg( REG_RCX, OFFSETOF(array,size), REG_RDX );
      cmp_reg32_imm( REG_RBX, 0 ); jge( &label_D );
      /* less than 0, add size */
      add_reg_reg( REG_RBX, REG_RDX );

      LABEL_D;
      cmp_reg32_imm( REG_RBX, 0 ); jl( &label_B ); // <0
      cmp_reg_reg( REG_RBX, REG_RCX); jge( &label_B ); // >size

      /* array, index inside array. push item, swap, pop, done */
      mov_mem_reg( REG_RCX, OFFSETOF(array,item), REG_RCX );
      shl_reg_imm( REG_RBX, 4 );
      add_reg_reg( REG_RBX, REG_RCX );
      amd64_push_svaluep_to( REG_RBX, -1 );
      jmp( &label_C );

      LABEL_A;
#if 0
      cmp_reg32_imm( REG_RAX, (PIKE_T_STRING<<8)|PIKE_T_INT );
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


  case F_ADD_NEG_INT:
    b = -b;

  case F_ADD_INT:
    {
      LABELS();
      ins_debug_instr_prologue(a-F_OFFSET, b, 0);
      amd64_load_sp_reg();
      mov_mem16_reg( sp_reg, -sizeof(struct svalue), REG_RAX );
      cmp_reg32_imm( REG_RAX,PIKE_T_INT );
      jne( &label_A );
      mov_mem_reg(sp_reg,
                  -sizeof(struct svalue)+OFFSETOF(svalue,u.integer),
                  REG_RAX );
      test_reg( REG_RAX );
      jz( &label_C );

      add_reg_imm( REG_RAX, b );
      jo( &label_A ); /* if overflow, use f_add */
      mov_reg_mem( REG_RAX,sp_reg,
                   -sizeof(struct svalue)+OFFSETOF(svalue,u.integer));
      jmp(&label_B); /* all done. */

      LABEL_A;
      amd64_push_int(b,0);
      update_arg1(2);
      amd64_call_c_opcode( f_add, I_UPDATE_SP );
      amd64_load_sp_reg();
      jmp( &label_B );
      LABEL_C;
      // int, and 0, we need to set it to b and clear subtype
      mov_imm_mem( PIKE_T_INT, sp_reg, -sizeof(struct svalue ) );
      mov_imm_mem( b, sp_reg,
                   -sizeof(struct svalue )+OFFSETOF(svalue,u.integer) );
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
      mov_sval_type( ARG1_REG, REG_RAX );
      /* type in RAX, svalue in ARG1 */
      cmp_reg32_imm( REG_RAX, PIKE_T_ARRAY );
      jne( &label_A );
      /* It's an array */
      /* move arg to point to the array */
      mov_mem_reg( ARG1_REG, OFFSETOF(svalue, u.array ), ARG1_REG);
      /* load size -> RAX*/
      mov_mem32_reg( ARG1_REG,OFFSETOF(array, size), REG_RAX );
      jmp( &label_C );
      LABEL_A;
      cmp_reg32_imm( REG_RAX, PIKE_T_STRING );
      jne( &label_B );
      /* It's a string */
      /* move arg to point to the string */
      mov_mem_reg( ARG1_REG, OFFSETOF(svalue, u.string ), ARG1_REG);
      /* load size ->RAX*/
      mov_mem32_reg( ARG1_REG,OFFSETOF(pike_string, len ), REG_RAX );
      jmp( &label_C );
      LABEL_B;
      /* It's something else, svalue already in ARG1. */
      amd64_call_c_function( pike_sizeof );
      LABEL_C;/* all done, res in RAX */
      /* Store result on stack */
      amd64_push_int_reg( REG_RAX );
    }
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
    flush_dirty_regs();	/* In case an error is thrown. */
    call_imm(low_object_index_no_free);
    /* NB: We know that low_object_index_no_free() doesn't
     *     mess with the stack pointer. */
    amd64_add_sp(1);
    return;

  case F_LOCAL:
    ins_debug_instr_prologue(a-F_OFFSET, b, 0);
    amd64_load_fp_reg();
    amd64_load_sp_reg();
    mov_mem_reg(fp_reg, OFFSETOF(pike_frame, locals), REG_RCX);
    add_reg_imm(REG_RCX, b*sizeof(struct svalue));
    amd64_push_svaluep(REG_RCX);
    return;

  case F_CLEAR_2_LOCAL:
  case F_CLEAR_LOCAL:
    ins_debug_instr_prologue(a-F_OFFSET, b, 0);
    amd64_load_fp_reg();
    mov_mem_reg(fp_reg, OFFSETOF(pike_frame, locals), REG_RBX);
    add_reg_imm(REG_RBX, b*sizeof(struct svalue));
    amd64_free_svalue(REG_RBX, 0);
    mov_imm_mem(PIKE_T_INT, REG_RBX, OFFSETOF(svalue, type));
    mov_imm_mem(0, REG_RBX, OFFSETOF(svalue, u.integer));
    if( a == F_CLEAR_2_LOCAL )
    {
      add_reg_imm( REG_RBX, sizeof(struct svalue ) );
      amd64_free_svalue(REG_RBX, 0);
      mov_imm_mem(PIKE_T_INT, REG_RBX, OFFSETOF(svalue, type));
      mov_imm_mem(0, REG_RBX, OFFSETOF(svalue, u.integer));
    }
    return;

  case F_INC_LOCAL_AND_POP:
    {
      LABELS();
      ins_debug_instr_prologue(a-F_OFFSET, b, 0);
      amd64_load_fp_reg();
      mov_mem_reg(fp_reg, OFFSETOF(pike_frame, locals), REG_RCX);
      add_reg_imm(REG_RCX, b*sizeof(struct svalue));
      mov_sval_type(REG_RCX, REG_RAX);
      cmp_reg32_imm(REG_RAX, PIKE_T_INT);
      jne(&label_A);
      /* Integer - Zap subtype and try just incrementing it. */
      mov_reg_mem32(REG_RAX, REG_RCX, OFFSETOF(svalue, type));
      add_imm_mem(1, REG_RCX, OFFSETOF(svalue, u.integer));
      jno(&label_B);
      add_imm_mem(-1, REG_RCX, OFFSETOF(svalue, u.integer));
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
      mov_mem_reg(fp_reg, OFFSETOF(pike_frame, locals), REG_RCX);
      add_reg_imm(REG_RCX, b*sizeof(struct svalue));
      mov_sval_type(REG_RCX, REG_RAX);
      cmp_reg32_imm(REG_RAX, PIKE_T_INT);
      jne(&label_A);
      /* Integer - Zap subtype and try just decrementing it. */
      mov_reg_mem32(REG_RAX, REG_RCX, OFFSETOF(svalue, type));
      add_imm_mem(-1, REG_RCX, OFFSETOF(svalue, u.integer));
      jno(&label_B);
      add_imm_mem(1, REG_RCX, OFFSETOF(svalue, u.integer));
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

    mov_mem_reg( mark_sp_reg, -sizeof(struct svalue*), REG_RAX );
    amd64_add_mark_sp( -1 );
    mov_reg_reg( sp_reg, ARG1_REG );
    sub_reg_reg( ARG1_REG, REG_RAX );
    shr_reg_imm( ARG1_REG, 4 );
    /* arg1 = (sp_reg - *--mark_sp)/16 (sizeof(svalue)) */

  case F_MARK_CALL_BUILTIN:
    if(a == F_MARK_CALL_BUILTIN )
    {
      /* Note: It is not actually possible to do ins_debug_instr_prologue
         here.
         ins_debug_instr_prologue(a-F_OFFSET, b, 0);
      */
      mov_imm_reg( 0, ARG1_REG );
    }

  case F_CALL_BUILTIN1:
    if(a == F_CALL_BUILTIN1 )
    {
      /* Note: It is not actually possible to do ins_debug_instr_prologue
         here.
         ins_debug_instr_prologue(a-F_OFFSET, b, 0);
      */
      mov_imm_reg( 1, ARG1_REG );
    }
    /* Get function pointer */
#if 0
    amd64_load_fp_reg();
    /* Ok.. This is.. interresting.
       Let's trust that the efun really is constant, ok?
    */
    mov_mem_reg( fp_reg, OFFSETOF(pike_frame,context), REG_RAX );
    mov_mem_reg( REG_RAX, OFFSETOF(inherit,prog), REG_RAX );
    mov_mem_reg( REG_RAX, OFFSETOF(program,constants), REG_RAX );
    add_reg_imm( REG_RAX, b*sizeof(struct program_constant) +
                 OFFSETOF(program_constant,sval) );
    mov_mem_reg( REG_RAX, OFFSETOF( svalue, u.efun ), REG_RAX );
    mov_mem_reg( REG_RAX, OFFSETOF( callable, function), REG_RAX );
    call_reg( REG_RAX );
    sp_reg = -1;
#else
    amd64_call_c_opcode(Pike_compiler->new_program->constants[b].sval.u.efun->function,
                        I_UPDATE_SP);
#endif
    return;

  case F_CONSTANT:
    ins_debug_instr_prologue(a-F_OFFSET, b, 0);
    amd64_load_fp_reg();
    amd64_load_sp_reg();
    mov_mem_reg( fp_reg, OFFSETOF(pike_frame,context), REG_RCX );
    mov_mem_reg( REG_RCX, OFFSETOF(inherit,prog), REG_RCX );
    mov_mem_reg( REG_RCX, OFFSETOF(program,constants), REG_RCX );
    add_reg_imm( REG_RCX, b*sizeof(struct program_constant) +
                 OFFSETOF(program_constant,sval) );
    amd64_push_svaluep( REG_RCX );
    return;

  case F_GLOBAL_LVALUE:
    ins_debug_instr_prologue(a-F_OFFSET, b, 0);
    amd64_load_fp_reg();
    amd64_load_sp_reg();

    amd64_push_this_object( );

    mov_imm_mem( T_OBJ_INDEX,  sp_reg, OFFSETOF(svalue,type));
    mov_mem_reg(fp_reg, OFFSETOF(pike_frame, context), REG_RAX);
    mov_mem16_reg( REG_RAX,OFFSETOF(inherit, identifier_level), REG_RAX);
    add_reg_imm( REG_RAX, b );
    mov_reg_mem( REG_RAX, sp_reg, OFFSETOF(svalue,u.identifier) );
    amd64_add_sp( 1 );
    return;

  case F_LOCAL_LVALUE:
    ins_debug_instr_prologue(a-F_OFFSET, b, 0);
    amd64_load_fp_reg();
    amd64_load_sp_reg();

    /* &frame->locals[b] */
    mov_mem_reg( fp_reg, OFFSETOF(pike_frame, locals), REG_RAX);
    add_reg_imm( REG_RAX, b*sizeof(struct svalue));

    mov_imm_mem( T_SVALUE_PTR,  sp_reg, OFFSETOF(svalue,type));
    mov_reg_mem( REG_RAX, sp_reg, OFFSETOF(svalue,u.lval) );
    mov_imm_mem( T_VOID,  sp_reg, OFFSETOF(svalue,type)+sizeof(struct svalue));
    amd64_add_sp( 2 );
    return;

  case F_PROTECT_STACK:
    ins_debug_instr_prologue(a-F_OFFSET, b, 0);
    amd64_load_fp_reg();
    mov_mem_reg(fp_reg, OFFSETOF(pike_frame, locals), ARG1_REG);
    if (b) {
      add_reg_imm_reg(ARG1_REG, sizeof(struct svalue) * b, ARG1_REG);
    }
    mov_reg_mem(ARG1_REG, fp_reg,
                              OFFSETOF(pike_frame, expendible));
    return;
  case F_MARK_AT:
    ins_debug_instr_prologue(a-F_OFFSET, b, 0);
    amd64_load_fp_reg();
    amd64_load_mark_sp_reg();
    mov_mem_reg(fp_reg, OFFSETOF(pike_frame, locals), ARG1_REG);
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
      mov_sval_type( ARG1_REG, REG_RCX );
      cmp_reg32_imm( REG_RCX, PIKE_T_INT );      je( &label_C );
      mov_imm_reg( 1, REG_RAX );
      shl_reg32_reg( REG_RAX, REG_RCX );
      and_reg32_imm( REG_RAX, BIT_FUNCTION|BIT_OBJECT );
      jnz( &label_A );
      /* Not object, int or function. Always true. */
      mov_imm_reg( 1, REG_RAX );
      jmp( &label_B );
    LABEL_A;
      amd64_call_c_function(svalue_is_true);
      jmp( &label_B );

    LABEL_C;
     mov_mem_reg( ARG1_REG, OFFSETOF(svalue, u.integer ), REG_RAX );
    /* integer. */

    LABEL_B;
      test_reg( REG_RAX );
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
    amd64_push_int((((unsigned INT64)b)<<32)|(unsigned INT32)c, 0);
    return;
  case F_MARK_AND_EXTERNAL:
    ins_f_byte(F_MARK);
    ins_f_byte_with_2_args(F_EXTERNAL, b, c);
    return;

  case F_ADD_LOCAL_INT:
  case F_ADD_LOCAL_INT_AND_POP:
   {
      LABELS();
      ins_debug_instr_prologue(a-F_OFFSET, b, 0);
      amd64_load_fp_reg();
      mov_mem_reg( fp_reg, OFFSETOF(pike_frame, locals), ARG1_REG);
      add_reg_imm( ARG1_REG, b*sizeof(struct svalue) );

      /* arg1 = dst
         arg2 = int
      */
      mov_sval_type( ARG1_REG, REG_RAX );
      cmp_reg32_imm( REG_RAX, PIKE_T_INT );
      jne(&label_A); /* Fallback */
      mov_imm_mem( PIKE_T_INT, ARG1_REG, OFFSETOF(svalue,type));
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
        mov_mem_reg( ARG1_REG, OFFSETOF(svalue,u.integer), REG_RAX );
        amd64_push_int_reg( REG_RAX );
      }
      LABEL_C;
      return;
     }
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
      mov_sval_type( ARG1_REG, REG_RAX );
      mov_sval_type( ARG2_REG, REG_RBX );
      shl_reg_imm( REG_RAX, 8 );
      add_reg_reg( REG_RAX, REG_RBX );
      cmp_reg32_imm( REG_RAX, (PIKE_T_INT<<8) | PIKE_T_INT );
      jne(&label_A); /* Fallback */
      mov_mem_reg( ARG2_REG, OFFSETOF(svalue,u.integer), REG_RAX );
      add_reg_mem( REG_RAX, ARG1_REG, OFFSETOF(svalue,u.integer));
      jo( &label_A);
      /* Clear subtype */
      mov_imm_mem( PIKE_T_INT, ARG1_REG,OFFSETOF(svalue,type));
      mov_reg_mem( REG_RAX, ARG1_REG, OFFSETOF(svalue,u.integer));
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
    mov_reg_reg( ARG1_REG, REG_RBX );
    amd64_free_svalue(ARG1_REG, 0);
    mov_imm_mem(c, REG_RBX, OFFSETOF(svalue, u.integer));
    mov_imm_mem32(PIKE_T_INT, REG_RBX, OFFSETOF(svalue, type));
    return;

  case F_LOCAL_2_LOCAL:
    ins_debug_instr_prologue(a-F_OFFSET, b, c);
    if( b != c )
    {
        amd64_load_fp_reg();
        mov_mem_reg( fp_reg, OFFSETOF(pike_frame, locals), REG_RBX );
        add_reg_imm( REG_RBX, b*sizeof(struct svalue) );
        /* RBX points to dst. */
        amd64_free_svalue( REG_RBX, 0 );
        /* assign rbx[0] = rbx[c-b] */
        mov_mem_reg( REG_RBX, (c-b)*sizeof(struct svalue), REG_RAX );
        mov_mem_reg( REG_RBX, (c-b)*sizeof(struct svalue)+8, REG_RCX );
        mov_reg_mem( REG_RAX, REG_RBX, 0 );
        mov_reg_mem( REG_RCX, REG_RBX, 8 );
        amd64_ref_svalue( REG_RBX, 1 );
    }
    return;
  case F_2_LOCALS:
#if 1
    ins_debug_instr_prologue(a-F_OFFSET, b, c);
    amd64_load_fp_reg();
    amd64_load_sp_reg();
    mov_mem_reg(fp_reg, OFFSETOF(pike_frame, locals), REG_R8);
    add_reg_imm( REG_R8, b*sizeof(struct svalue) );
    amd64_push_svaluep(REG_R8);
    add_reg_imm( REG_R8, (c-b)*sizeof(struct svalue) );
    amd64_push_svaluep(REG_R8);
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
      jg(&label_B);
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

