#!/home/grubba/src/pike/build/sol2.5/pike
/*
 * $Id: idea.pike,v 1.3 1996/11/11 14:23:29 grubba Exp $
 *
 * An crypto using IDEA written in Pike
 * rename it to get it to use another crypto.
 *
 * Henrik Grubbström 1996-11-08
 */

/*
 * Includes
 */

#include <stdio.h>

#include "crypto.h"

/*
 * Functions
 */

int main(int argc, string *argv)
{
  int encrypt = 1;
  int sunos = 0;
  int hexkey = 0;
  int i;
  object crypto_object = ((program)"/precompiled/crypto")
                         ((program)"/precompiled/crypto/cbc",
			  (program)("/precompiled/crypto/" +
				    ((argv[0]/"/")[-1] / ".")[0]));
  object input = ((program)"/precompiled/file")("stdin");
  object output = ((program)"/precompiled/file")("stdout");
  string data = "";
  string outstr = "";
  string key = "";

  for (i=1; i < argc; i++) {
    switch(argv[i]) {
    case "-E":
      encrypt = 1;
      break;
    case "-D":
      encrypt=0;
      break;
    case "-k":
      key = argv[i+1];
      i++;
      break;
    case "-h":
      hexkey = 1;
      break;
    default:
      if (argv[i][0] == "-"[0]) {
	werror("Unknown argument \""+argv[i]+"\"\n");
	exit(1);
      }
      input->open(argv[i], "r");
    }
  }
  
  if (hexkey) {
    key = hex_to_string(key);
  }
  if (encrypt) {
    crypto_object->set_encrypt_key(key);
  } else {
    crypto_object->set_decrypt_key(key);
  }
  
  key="";

  while (1) {
    string newdata = input->read(1000000);

    if (stringp(newdata) && sizeof(newdata)) {
      if (encrypt) {
	output->write(crypto_object->crypt(newdata));
      } else {
	output->write(outstr);
	outstr = crypto_object->crypt(newdata);
      }
    } else {
      if (encrypt) {
	output->write(crypto_object->pad());
      } else {
	output->write(crypto_object->unpad(outstr));
      }
      break;
    }
  }

  destruct(crypto_object);
  input->close();

  return(0);
}
