#!/home/grubba/src/pike/build/sol2.5/pike
/*
 * $Id: des.pike,v 1.1.1.1 1996/11/05 15:10:10 grubba Exp $
 *
 * An equvivalent for /usr/local/ssl/bin/des written in Pike
 *
 * Henrik Grubbström 1996-10-25
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
  object des_object = clone((program)"/precompiled/crypto/des");
  object input = clone((program)"/precompiled/file", "stdin");
  object output = clone((program)"/precompiled/file", "stdout");
  string data = "";
  string outstr = "";

  for (i=1; i < argc; i++) {
    switch(argv[i]) {
    case "-e":
      sunos = 1;
      /* FALLTHROUGH */
    case "-E":
      encrypt = 1;
      break;
    case "-d":
      sunos=1;
      /* FALLTHROUGH */
    case "-D":
      encrypt=0;
      break;
    case "-k":
      if (hexkey) {
	des_object->set_key(hex_to_string(argv[i+1]));
      } else if (sunos) {
	des_object->set_key(des_object->make_sun_key(argv[i+1]));
      } else {
	string tmp = des_object->make_key(argv[i+1]);
	werror(sprintf("rawkey:%s\nkey:%s\n", argv[i+1],
		       string_to_hex(tmp)));
	des_object->set_key(tmp);
	werror(sprintf("schedule:%s\n",
		       string_to_hex(des_object->get_schedule())));
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

    werror("State:"+string_to_hex(des_object->dump())+"\n");

    if (stringp(newdata) && sizeof(newdata)) {
      int len;

      data += newdata;

      /* Make blocks */
      len = sizeof(data) & ~0x07;

      if (len) {
	if (encrypt) {
perror("foo\n");
	  output->write(des_object->encrypt(data[0..len-1]));
perror("foo\n");
	} else {
	  output->write(outstr);
	  outstr = des_object->decrypt(data[0..len-1]);
	}
	data = data[len..];
      }
    } else {
      if (encrypt) {
	int len = sizeof(data);
	
	data += sprintf("%@c",
			({ random(256), random(256), random(256), random(256),
			   random(256), random(256), random(256), len })[len..]);
	output->write(des_object->encrypt(data));
      } else {
	if ((sizeof(data))||(outstr[-1] & ~0x07)) {
	  werror("Data didn't decrypt correctly\n");
	  destruct(des_object);
	  exit(1);
	} else {
	  int len = ((sizeof(outstr) & ~0x07) | outstr[-1]) - 8;

	  if ((len > sizeof(outstr)) || (len < 1)) {
	    werror("Data didn't decrypt correctly\n");
	    werror(sprintf("len:%d, %d\n", sizeof(outstr), len));
	    destruct(des_object);
	    exit(1);
	  } else {
	    output->write(outstr[0..len-1]);
	  }
	}
      }
      break;
    }
  }

  destruct(des_object);
  input->close();

  return(0);
}
