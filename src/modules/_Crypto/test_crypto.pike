#! /home/nisse/work/roxen/pike/src/pike

/* test_crypto.pike */

inherit "StdCrypt";

#define K(a) sprintf("%{%c%}", a)


int test_des()
{
  string *keys = ({ K( ({0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x80}) ),
		    K( ({0x80, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01}) ),
		    K( ({0x08, 0x19, 0x2a, 0x3b, 0x4c, 0x5d, 0x6e, 0x7f}) ),
		    K( ({0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef}) ),
		    K( ({0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef}) ) });

  string *texts = ({ K( ({0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}) ),
		     K( ({0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40}) ),
		     K( ({0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}) ),
		     K( ({'N',  'o',  'w',  ' ',  'i',  's',  ' ',  't' }) ),
		     K( ({0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xe7}) ) });


  string *cipher = ({ K( ({0x9C, 0xC6, 0x2D, 0xF4, 0x3B, 0x6E, 0xED, 0x74 }) ),
		      K( ({0xA3, 0x80, 0xE0, 0x2A, 0x6B, 0xE5, 0x46, 0x96 }) ),
		      K( ({0x25, 0xDD, 0xAC, 0x3E, 0x96, 0x17, 0x64, 0x67 }) ),
		      K( ({0x3F, 0xA4, 0x0E, 0x8A, 0x98, 0x4D, 0x48, 0x15 }) ),
		      K( ({0xc9, 0x57, 0x44, 0x25, 0x6a, 0x5e, 0xd3, 0x1d }) ) });

  string *no_perm = ({ K( ({0x57, 0x99, 0xF7, 0x2A, 0xD2, 0x3F, 0xAE, 0x4C }) ),
		       K( ({0x90, 0xE6, 0x96, 0xA2, 0xAD, 0x56, 0x50, 0x0D }) ),
		       K( ({0x43, 0x5C, 0xFF, 0xC5, 0x68, 0xB3, 0x70, 0x1D }) ),
		       K( ({0x80, 0xB5, 0x07, 0xE1, 0xE6, 0xA7, 0x47, 0x3D }) ) });
  int i;
  int err;
  object c;
  
  write("DES...\n");
  for (i=0; i < sizeof(keys); i++)
    {
      string gibberish;
      write(sprintf("Testing key %d\n", i));
      c = DES()->set_encrypt_key(keys[i]);
      gibberish = c->crypt_block(texts[i]);
      if (gibberish != cipher[i])
	{
	  write("Encryption failed\n");
	  err++;
	}
      c->set_decrypt_key(keys[i]);
      if (c->crypt_block(gibberish) != texts[i])
	{
	  write("Decryption failed\n");
	  err++;
	}
    }
  write(err ? "DES failed!\n" : "DES Ok.\n");
  return err;
}


int test_idea()
{
  string key = K( ({0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
		      0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef}) );
  string msg = K( ({0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xe7}) );
  string cipher = K( ({0x20, 0x11, 0xaa, 0xce, 0xf6, 0xf4, 0xbc, 0x7f}) );
  string gibberish;
  
  int err;
  object c;

  write("IDEA...\n");
  c = IDEA()->set_encrypt_key(key);
  gibberish = c->crypt_block(msg);
  if (gibberish != cipher)
    {
      write("Encryption failed\n");
      err++;
    }
  c->set_decrypt_key(key);
  if (c->crypt_block(gibberish) != msg)
    {
      write("Decryption failed\n");
      err++;
    }
  write(err ? "IDEA failed!\n" : "IDEA Ok.\n");
  return err;
}


int test_cbc()
{
  string key = "abcdefghijklmnop"; string iv = "fogazonk";
  string msg =
    "See a body and a dream of the dead days\n"
    "A following lost and blind\n"
    "Living far from here\n"
    "Tomorrow is hard to find\n"
    "And it seems like twenty five years of\n"
    "Promises and give me more\n"
    "Scenes of a hand-me-down in\n"
    "Dresses heard before\n"
    "First and last and always:\n"
    "'til the end of time\n"
    "First and last and always:\n"
    "Mine\n"
    "mine\n"
    "mine\n"
    "mine\n"
    "mine\n"
    "-\n"
    "Maybe it's not so easy\n"
    "Maybe it's a way to long\n"
    "Say,\n"
    "say you'll be by me\n"
    "When the evidence comes along\n"
    "-\n"
    "First and last and always:\n"
    "My calling,\n"
    "my time\n"
    "First and last and always:\n"
    "Mine\n"
    "mine\n"
    "mine\n"
    "mine\n"
    "-\n"
    "My calling\n"
    "My time\n"
    "-\n"
    "See a place and a dream of the dead days\n"
    "A following lost and blind\n"
    "Cross my heart with silver \n"
    "Here's the key behind\n"
    "Seems like twenty five years of\n"
    "Ever after ever more more more\n"
    "Seems I wore this face for you\n"
    "Far too long before\n";
  
  string cipher;
  string gibberish;
  object c;
    
  c = Crypto(CBC(IDEA()->set_encrypt_key(key))->set_iv(iv));
  gibberish = c->crypt(msg) + c->pad();
  write("\""+ gibberish + "\"\n");
  return 0;
}

int main()
{
  return test_des() | test_idea() | test_cbc();
}
