/*
 * $Id: bytecode.c,v 1.2 2001/07/20 13:20:38 grubba Exp $
 *
 * Default bytecode assembler for Pike.
 *
 */

void ins_pointer(INT32 ptr)
{
  ins_int(ptr, add_to_program);
}

INT32 read_pointer(INT32 off)
{
  return read_int(off);
}

void upd_pointer(INT32 off, INT32 ptr)
{
  upd_int(off, ptr);
}

void ins_align(INT32 align)
{
  while(Pike_compiler->new_program->num_program % align) {
    add_to_program(0);
  }
}

void ins_byte(INT32 val)
{
  add_to_program(val);
}

void ins_data(INT32 val)
{
  ins_int(val, add_to_program);
}

void ins_f_byte(unsigned int b)
{
#ifdef PIKE_DEBUG
  if(store_linenumbers && b<F_MAX_OPCODE)
    ADD_COMPILED(b);
#endif /* PIKE_DEBUG */

  b-=F_OFFSET;
#ifdef PIKE_DEBUG
  if(b>255)
    Pike_error("Instruction too big %d\n",b);
#endif
    
  add_to_program((unsigned char)b);
}

static void ins_f_byte_with_arg(unsigned int a,unsigned INT32 b)
{
  switch(b >> 8)
  {
  case 0 : break;
  case 1 : ins_f_byte(F_PREFIX_256); break;
  case 2 : ins_f_byte(F_PREFIX_512); break;
  case 3 : ins_f_byte(F_PREFIX_768); break;
  case 4 : ins_f_byte(F_PREFIX_1024); break;
  default:
    if( b < 256*256)
    {
      ins_f_byte(F_PREFIX_CHARX256);
      add_to_program((unsigned char)(b>>8));
    }else if(b < 256*256*256) {
      ins_f_byte(F_PREFIX_WORDX256);
      add_to_program((unsigned char)(b>>16));
      add_to_program((unsigned char)(b>>8));
    }else{
      ins_f_byte(F_PREFIX_24BITX256);
      add_to_program((unsigned char)(b>>24));
      add_to_program((unsigned char)(b>>16));
      add_to_program((unsigned char)(b>>8));
    }
  }
  ins_f_byte(a);
  add_to_program((PIKE_OPCODE_T)b);
}

static void ins_f_byte_with_2_args(unsigned int a,
				   unsigned INT32 c,
				   unsigned INT32 b)
{
  switch(b >> 8)
  {
  case 0 : break;
  case 1 : ins_f_byte(F_PREFIX2_256); break;
  case 2 : ins_f_byte(F_PREFIX2_512); break;
  case 3 : ins_f_byte(F_PREFIX2_768); break;
  case 4 : ins_f_byte(F_PREFIX2_1024); break;
  default:
    if( b < 256*256)
    {
      ins_f_byte(F_PREFIX2_CHARX256);
      add_to_program((unsigned char)(b>>8));
    }else if(b < 256*256*256) {
      ins_f_byte(F_PREFIX2_WORDX256);
      add_to_program((unsigned char)(b>>16));
      add_to_program((unsigned char)(b>>8));
    }else{
      ins_f_byte(F_PREFIX2_24BITX256);
      add_to_program((unsigned char)(b>>24));
      add_to_program((unsigned char)(b>>16));
      add_to_program((unsigned char)(b>>8));
    }
  }
  ins_f_byte_with_arg(a,c);
  add_to_program((PIKE_OPCODE_T)b);
}

