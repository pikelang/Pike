#!/home/grubba/src/pike/build/sol2.5/pike
/*
 * $Id: idea.pike,v 1.1 1996/11/08 23:00:30 grubba Exp $
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
  object crypto_object = clone((program)"/precompiled/crypto",
			       (program)("/precompiled/crypto/" +
					 ((argv[0]/"/")[-1] / ".")[0]));
  object input = clone((program)"/precompiled/file", "stdin");
  object output = clone((program)"/precompiled/file", "stdout");
  string data = "";
  string outstr = "";

  for (i=1; i < argc; i++) {
    switch(argv[i]) {
    case "-E":
      encrypt = 1;
      break;
    case "-D":
      encrypt=0;
      break;
    case "-k":
      if (hexkey) {
	crypto_object->set_key(hex_to_string(argv[i+1]));
      } else {
	crypto_object->set_key(argv[i+1]);
      }
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
  
  while (1) {
    string newdata = input->read(1000000);

    if (stringp(newdata) && sizeof(newdata)) {
      int len;

      data += newdata;

      /* Make blocks */
      len = sizeof(data) & ~0x0f;

      if (len) {
	if (encrypt) {

	  output->write(crypto_object->cbc_encrypt(data[0..len-1]));

	} else {
	  output->write(outstr);
	  outstr = crypto_object->cbc_decrypt(data[0..len-1]);
	}
	data = data[len..];
      }
    } else {
      if (encrypt) {
	int len = sizeof(data) & 0x0f;
	
	data += sprintf("%@c",
			({ random(256), random(256), random(256), random(256),
			   random(256), random(256), random(256), random(256),
			   random(256), random(256), random(256), random(256),
			   random(256), random(256), random(256), len })[len..]);
	output->write(crypto_object->cbc_encrypt(data));
      } else {
	if ((sizeof(data))||(outstr[-1] & ~0x0f)||(sizeof(outstr) & 0x0f)) {
	  werror("Data didn't decrypt correctly\n");
	  output->write(outstr);
	  destruct(crypto_object);
	  exit(1);
	} else {
	  int len = ((sizeof(outstr) & ~0x0f) | outstr[-1]) - 16;

	  if ((len > sizeof(outstr)) || (len < 1)) {
	    werror("Data didn't decrypt correctly\n");
	    werror(sprintf("len:%d, %d\n", sizeof(outstr), len));
	    destruct(crypto_object);
	    exit(1);
	  } else {
	    output->write(outstr[0..len-1]);
	  }
	}
      }
      break;
    }
  }

  destruct(crypto_object);
  input->close();

  return(0);
}
