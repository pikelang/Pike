/*
 * $Id: rijndaeltest.pike,v 1.2 2005/03/13 23:45:19 nilsson Exp $
 *
 * Test Crypto.aes against the official test-vectors.
 *
 * Henrik Grubbström 2001-03-24
 */

// Read the raw vectors.
constant raw_cbc_d_m = #string "rijndael_cbc_d_m.txt";
constant raw_cbc_e_m = #string "rijndael_cbc_e_m.txt";
// constant raw_ecb_d_m = #string "rijndael_ecb_d_m.txt";
constant raw_ecb_e_m = #string "rijndael_ecb_e_m.txt";
// constant raw_ecb_iv = #string "rijndael_ecb_iv.txt";
constant raw_ecb_tbl = #string "rijndael_ecb_tbl.txt";
// constant raw_ecb_vk = #string "rijndael_ecb_vk.txt";
// constant raw_ecb_vt = #string "rijndael_ecb_vt.txt";

int tests;

int run_test(string raw, function(mapping(string:string):int) fun)
{
  int fail;
  raw -= "\r";
  foreach (raw / "\n\n", string segment) {
    tests++;
    if (has_prefix(segment, "==") || !has_value(segment, "=")) continue;
    if (fun(aggregate_mapping(@(map(segment/"\n" - ({""}), `/, "=") *
				({}))))) {
      werror("\nFailure for vector:\n"
	     "%s\n", segment);
      fail++;
    }
  }
  return fail;
}

int check_ecb_e_m()
{
  int fail;
  string keysize;

  object aes = Crypto.AES();

  fail = run_test(raw_ecb_e_m, lambda(mapping(string:string) v) {
    if (v->KEYSIZE) {
      if (keysize) werror("\n");
      keysize = v->KEYSIZE;
      return;
    }
    if (!v->I) return;

    werror("Rijndael ECB Encryption (%s): %s\r", keysize, v->I);

    string pt = String.hex2string(v->PT);
    string ct = String.hex2string(v->CT);

    aes->set_encrypt_key(String.hex2string(v->KEY));

    for(int i = 0; i < 10000; i++) {
      pt = aes->crypt(pt);
    }

    return pt != ct;
  });

  werror("\n");
  return fail;
}

int check_ecb_d_m()
{
  int fail;
  string keysize;

  object aes = Crypto.AES();

  fail = run_test(raw_ecb_e_m, lambda(mapping(string:string) v) {
    if (v->KEYSIZE) {
      if (keysize) werror("\n");
      keysize = v->KEYSIZE;
      return;
    }
    if (!v->I) return;

    werror("Rijndael ECB Decryption (%s): %s   \r", keysize, v->I);

    string pt = String.hex2string(v->PT);
    string ct = String.hex2string(v->CT);

    aes->set_decrypt_key(String.hex2string(v->KEY));

    for(int i = 0; i < 10000; i++) {
      ct = aes->crypt(ct);
    }

    return pt != ct;
  });

  werror("\n");
  return fail;
}

int check_cbc_e_m()
{
  int fail;
  string keysize;

  object aes_cbc = Crypto.CBC(Crypto.AES);

  fail = run_test(raw_cbc_e_m, lambda(mapping(string:string) v) {
    if (v->KEYSIZE) {
      if (keysize) werror("\n");
      keysize = v->KEYSIZE;
      return;
    }
    if (!v->I) return;

    werror("Rijndael CBC Encryption (%s): %s\r", keysize, v->I);

    string pt = String.hex2string(v->PT);
    string ct = String.hex2string(v->CT);
    string iv = String.hex2string(v->IV);

    aes_cbc->set_encrypt_key(String.hex2string(v->KEY));
    aes_cbc->set_iv(iv);

    pt += iv;

    for(int i = 0; i < 5000; i++) {
      pt = aes_cbc->crypt(pt);
    }

    return pt[16..] != ct;
  });

  werror("\n");
  return fail;
}

int check_cbc_d_m()
{
  int fail;
  string keysize;

  object aes_cbc = Crypto.CBC(Crypto.AES);

  fail = run_test(raw_cbc_d_m, lambda(mapping(string:string) v) {
    if (v->KEYSIZE) {
      if (keysize) werror("\n");
      keysize = v->KEYSIZE;
      return;
    }
    if (!v->I) return;

    werror("Rijndael CBC Decryption (%s): %s\r", keysize, v->I);

    string pt = String.hex2string(v->PT);
    string ct = String.hex2string(v->CT);
    string iv = String.hex2string(v->IV);

    aes_cbc->set_decrypt_key(String.hex2string(v->KEY));
    aes_cbc->set_iv(iv);

    for(int i = 0; i < 10000; i++) {
      ct = aes_cbc->crypt(ct);
    }

    return pt != ct;
  });

  werror("\n");
  return fail;
}

int check_ecb_tbl()
{
  int fail;
  string keysize;

  object aes_e = Crypto.AES();
  object aes_d = Crypto.AES();

  fail = run_test(raw_ecb_tbl, lambda(mapping(string:string) v) {
    if (v->KEYSIZE) {
      werror("Rijndael ECB encrypt/decrypt (%s)...\r", v->KEYSIZE);
      return;
    }
    if (!v->I) return;

    string pt = String.hex2string(v->PT);
    string ct = String.hex2string(v->CT);

    aes_e->set_encrypt_key(String.hex2string(v->KEY));
    aes_d->set_decrypt_key(String.hex2string(v->KEY));

    string _ct = aes_e->crypt(pt);
    string _pt = aes_d->crypt(ct);

    return (ct != _ct) || (pt != _pt);
  });
  werror("\n");
  return fail;
}

int main(int argc, array(string) argv)
{
  werror("\n");
  array(array(string|int)) q = Getopt.find_all_options(argv, ({
    ({ "quick", Getopt.NO_ARG, ({ "-q", "--quick"}) })
  }));
  int fails = check_ecb_tbl();
  if (fails) {
    write( tests+" "+fails );
    return 0;
  }
  foreach(q, array(string|int) option) {
    if (option[0] == "quick") {
      write( tests+" "+fails );
      return 0;
    }
  }
  fails += check_ecb_e_m() + check_ecb_d_m() + check_cbc_e_m() + check_cbc_d_m();
  write( tests+" "+fails );
  return 0;
}
