/*
 * $Id: toktest.pike,v 1.2 2003/01/17 16:46:28 grubba Exp $
 *
 * Tokenizer testing program.
 *
 * Henrik Grubbström 2002-09-23
 */

import Tokenizer;

int main(int argc, array(string) argv)
{
  //array res = group(Tokenizer.pike_tokenizer(cpp(#string "toktest.pike")));
  Compiler c = Compiler();

  Group res = c->group(c->pike_tokenizer(cpp(#string "toktest.pike" +
					     "mixed `(;\n"
					     ,
					     __FILE__)));

  werror("Res:%O\n", res);

  if (res) {
    werror("Parsed:%O\n", parse(res));
  }
}
