#!/home/grubba/src/pike/build/sol2.5/pike
/*
 * $Id: md5.pike,v 1.1.1.1 1996/11/05 15:10:10 grubba Exp $
 *
 * An md5 checksummer written in Pike
 *
 * Henrik Grubbström 1996-10-24
 */

/*
 * Includes
 */

#include "crypto.h"

/*
 * Functions
 */

int main(int argc, string *argv)
{
  object input;

  input = clone((program)"/precompiled/file");

  if (argc == 1) {
  } else {
    string filename;

    foreach (argv[1..], filename) {
      string data;
      string foo, bar;
      object checksum;
      object checksum2;

      checksum = clone((program)"/precompiled/crypto/md5");
      checksum2 = clone((program)"/precompiled/crypto/md2");

      input->open(filename, "rb");

      while (1) {
	data = input->read(1000000000);

	if (stringp(data) && sizeof(data)) {
	  checksum->push(data);
	  checksum2->push(data);
	} else {
	  break;
	}
      }

      input->close();

      werror("MD5("+filename+")= "+string_to_hex((string)checksum)+"\n");
      werror("MD2("+filename+")= "+string_to_hex((string)checksum2)+"\n");

      foo = ((string)checksum)+((string)checksum2);
      bar = hex_to_string(string_to_hex((string)checksum) +
			  string_to_hex((string)checksum2));

      if (foo != bar) { 
	werror("Mismatch!\nfoo: " + string_to_hex(foo) +"\nbar: "+string_to_hex(bar)+"\n");
      }

      destruct(checksum);
      destruct(checksum2);
    }
  }
}
