/*
 * $Id: toktest.pike,v 1.3 2004/12/09 17:02:46 grubba Exp $
 *
 * Tokenizer testing program.
 *
 * Henrik Grubbström 2002-09-23
 */

import Tokenizer;

int main(int argc, array(string) argv)
{
  //array res = group(Tokenizer.pike_tokenizer(cpp(#string "toktest.pike")));
  PikeCompiler c = PikeCompiler();

  TokenList res = c->group(c->pike_tokenizer(cpp(#string "toktest.pike" +
						 "mixed `(;\n"
						 ,
						 __FILE__)));

  werror("Res:%O\n", res);

  if (res) {
    werror("Parsed:%O\n", c->parse_program(res));
  }
}
