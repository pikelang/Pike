#! /home/nisse/work/roxen/pike/src/pike

/* test_crypto.pike */

inherit "StdCrypt";

#define K(a) hex_to_string(a)
#define H(a) string_to_hex(a)

int test_des()
{
  string *keys = ({ K("0101010101010180"),
		    K("8001010101010101"),
		    K("08192a3b4c5d6e7f"),
		    K("0123456789abcdef"),
		    K("0123456789abcdef") });

  string *texts = ({ K("0000000000000000"),
		     K("0000000000000040"),
		     K("0000000000000000"),
		     "Now is t",
		     K("0123456789abcde7") });


  string *cipher = ({ K("9CC62DF43B6EED74"),
		      K("A380E02A6BE54696"),
		      K("25DDAC3E96176467"),
		      K("3FA40E8A984D4815"),
		      K("c95744256a5ed31d") });

  string *no_perm = ({ K("5799F72AD23FAE4C"),
		       K("90E696A2AD56500D"),
		       K("435CFFC568B3701D"),
		       K("80B507E1E6A7473D") });
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
  string key = K("0123456789abcdef0123456789abcdef");
  string msg = K("0123456789abcde7");
  string cipher = K("2011aacef6f4bc7f");
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

  string cipher = K("9e50920a38402aa89061d54109b150b32b9a0aa3996329ae319d31c902f9d4b1"
		    "49d349a90e6f500e8e99c25ed2744f757cc4d754503683a6716b0949926ea1e3"
		    "74146f3ab7c3db5521cc12e6d83fb1d8fbd55447a2f42cf4eb27de3fc291a0fe"
		    "849e152a86609dde17b06f77350c3a536502e46ef26ee98116ac272b4349d1e3"
		    "45bebf7b92c352897023470926a09e283996539ddcd5304d21b02049e54363df"
		    "0acd216f00cfcca94365d2bb23523c23d010fe68bd2aa3403f137b7ea4aef772"
		    "eca309e0989bff86c67a023ff5e6a75ed66b143220ee491488af79371ecdc759"
		    "a080ee87f69ee2831d709b0cead6f6453f97f190f33639da584bca104cdb53d3"
		    "e7d723d7337d48d25a68a259efa8c720653c4421b1a92bcf741dfaa720613e3f"
		    "00565b9df8ae7c165df3ac72a50db293d972fc914990f204fb911fe13fa320e8"
		    "14c493b8cdcb8aa3241eb017143431b1f780e49e11a2451abffb88e168774a0e"
		    "d370522ee385f3e22c824930b582c7faa8a80807e7d574676d4038c9dbb1dff9"
		    "aee922f4d27614972c2b169859fc95083975eeaa933ab3fe15215e27ef6a2fac"
		    "d00bec4dcca31d8758cc80744d97f1349cf5f93a2ff819f7794e45e0c18e6bc4"
		    "d0591eb3761898b01599be001d7c5fb9e2bbf310f5638f35f25c7757ea140fa9"
		    "3a6e4f07f0ff5b75bddcab657eae25c507511fba9d7f59769630acbf5e0c90b9"
		    "dc1058a771f44e8ab4ad14305a8e161b5e4e8553a4c0f69635011a193c21ad94"
		    "3e8c10602e82675e1941478a5c348e9897052d739b0fc07b2db3cea70d3ed0ce"
		    "111aaa0f4329b1ea27e84312f6c023d91541b4a07f2b769b173778e0abb6a11c"
		    "c3757eebea2e2eac35237db39a491f40e3160942a8a58e697b9bfd0827a9b44a"
		    "90169c8f7705b12d7f9373e22be95285804b39cf7bd16e4c5f9ef6414a7ef3e6"
		    "a1c705fa91bc0f6cdb7171bfcdfa5f78b469b8f96ab2ad54aaae3dfc09ea8d51"
		    "6a1f4b0f185e96dce1151b3abce6c8db0e2a70a49fe748602a00e96dbba8ecf4"
		    "6aab93f63f15272dd1e533c95429563cafaf5365d7085509e261b574e6e556e4"
		    "d06822acdcdeed56001f8a1e33e98191");
  
  string ciph = K("d265ed56e5ccc466aa13d1ff3475cbf0c9b1e317019d3b023d75ade0988f5a3b"
		     "a1d076700f92fade991cfd479569d454764c783805bd448ba257432b58d5d26f"
		     "f7b82ac49220a0cf07bfc7300c84d84742cb77b8d9f4edf78c3aaef002901175"
		     "f937de5f16b1426065aab6157d8027abb2ce6cab4aeb5c415ea4df9915756639"
		     "ddced3f7b1244c21a3bf094dac631c98f190a69b8e04ae617df607ee711e04b9"
		     "5b178a8686722b03b027ad8a603a1e8465fd920d06e8e7e9813b290658969146"
		     "f51fd22799f6ec4220f48d2477b214f8bff6068d3104925750ee49026dd00a1e"
		     "54ab3bfe01015d388dd09cf1bf7f52e13d2973ac6195115e196334f6c986df20"
		     "bdb422f7e9ffe8082dd278118e850ac64326a3279714b57b8e3a62000a7ad08f"
		     "e8337f61ad38c69f4262b3cb6ef52d76f39ec44b9ff18be0e9c059112f174756"
		     "9a2a7304a54883e3667c158355dac8ac8f43ca72e0d57fd3562fdea4c885f12b"
		     "c7008f4233933cfb706cfd1e2c4b22ed0b3e559cdb588b94849bbee85b6e5089"
		     "5adad99bd18d815300f07d5eb54962a9f5ec6dcb06650c8ce24ba8d6aa6fdae1"
		     "525131005593e13c5f2a35f56aca608fa50db07730ef5a7fe98e509eeab056f6"
		     "50c35e603343b96c46d6f659a4c8456d5f3ae62fd9b0e3cfbbe65e4c171e60e3"
		     "98075336a38b64b9caa2ae573ca230c173cfbec793b9fe199ea3974146adc5dd"
		     "90eaa279656da91b128a9c3c455d83a89f09dc673528a40fd50b5b9b0f29c9cf"
		     "28ed2bfbea9188c72ed7a8d33fd37cb05bc410680516883300e5eae101da4056"
		     "ea3b2a9308ee347b3f807aaf436b67cc20f8ac28f60d7bce8c7487a076be16f1"
		     "404e4da7eba398813aff64b1544c31082d0ebb74d1584d391b194a309c7d8854"
		     "23d5a8ec475f323795a0a522d0cc92c7e20d4c403ec8d7feb98822b199cf204d"
		     "9777d3400379302220ae7fdaf80295870fea2be4e404115ed9dbe50c683d0480"
		     "960bebde0b7ded4eee2d450cb71291b36ae0ede84e2574d6ec4d436e1ddeaa73"
		     "0056238300e978477103b1250c23c5a51110bc6967388f1dd6d72f0db4627b5c"
		     "8f841f76edf92a005fdd1a75b984d3b8");

  string gibberish;
  string recovered;
  object c;
  int err;

  write("CBC...\n");
  // c = Crypto(CBC(IDEA)->set_encrypt_key(key)->set_iv(iv)); /* correct */
  c = Crypto(CBC(IDEA)->set_iv(iv)->set_encrypt_key(key)); /* bad */
  gibberish = c->crypt(msg) + c->pad();
  write("\"" + gibberish[..50] + "\"\n");
  write("\"" + H(gibberish) + "\"\n");
  if (gibberish != cipher)
    {
      write("Encryption failed\n");
      err++;
    }
  c = Crypto(CBC(IDEA)->set_decrypt_key(key)->set_iv(iv));
  recovered = c->unpad(c->crypt(gibberish));
  write("\"" + recovered[..50] + "...\"\n");
  if (recovered != msg)
    {
      write("Decryption failed\n");
      err++;
    }
  write(err ? "CBC failed!\n" : "CBC Ok.\n");
  return err;
}

int main()
{
  return test_des() | test_idea() | test_cbc();
}
