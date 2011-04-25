/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/*
 * Default bytecode assembler for Pike.
 */

void ins_f_byte(unsigned int b)
{
  b-=F_OFFSET;
#ifdef PIKE_DEBUG
  if(b>255)
    Pike_error("Instruction too big %d\n",b);
#endif
    
  add_to_program((unsigned char)b);
}

void ins_f_byte_with_arg(unsigned int a,unsigned INT32 b)
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

void ins_f_byte_with_2_args(unsigned int a,
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
