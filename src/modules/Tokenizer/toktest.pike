/*
 * $Id: toktest.pike,v 1.1 2002/10/17 10:43:18 grubba Exp $
 *
 * Tokenizer testing program.
 *
 * Henrik Grubbström 2002-09-23
 */

import Tokenizer;

int main(int argc, array(string) argv)
{
  //array res = group(Tokenizer.pike_tokenizer(cpp(#string "toktest.pike")));
  Group res = group(pike_tokenizer(cpp(#string "toktest.pike", "toktest.pike")));

  werror("Res:%O\n", res);

  if (res) {
    werror("Parsed:%O\n", parse(res));
  }
}
