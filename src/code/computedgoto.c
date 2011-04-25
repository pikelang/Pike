/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/*
 * Machine code generator for sparc.
 *
 * Henrik Grubbström 20010720
 */

void ins_pointer(INT32 ptr)
{
  add_to_program((void *)(ptrdiff_t)ptr);
}

INT32 read_pointer(INT32 off)
{
  return (INT32)(ptrdiff_t)Pike_compiler->new_program->program[off];
}

void upd_pointer(INT32 off, INT32 ptr)
{
  Pike_compiler->new_program->program[off] = (void *)(ptrdiff_t)ptr;
}

void ins_align(INT32 align)
{
  /* Already aligned... */
}

void ins_byte(INT32 val)
{
  add_to_program((void *)(ptrdiff_t)val);
}

void ins_data(INT32 val)
{
  add_to_program((void *)(ptrdiff_t)val);
}

void ins_f_byte(unsigned int b)
{
  b-=F_OFFSET;
#ifdef PIKE_DEBUG
  if(b>255)
    Pike_error("Instruction too big %d\n",b);
#endif
    
  add_to_program(fcode_to_opcode[b]);
}

void ins_f_byte_with_arg(unsigned int a,unsigned INT32 b)
{
  ins_f_byte(a);
  add_to_program((PIKE_OPCODE_T)b);
}

void ins_f_byte_with_2_args(unsigned int a,
			    unsigned INT32 c,
			    unsigned INT32 b)
{
  ins_f_byte_with_arg(a,c);
  add_to_program((PIKE_OPCODE_T)b);
}
