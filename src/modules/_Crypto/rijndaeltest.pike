/*
 * $Id: rijndaeltest.pike,v 1.2 2001/03/24 22:32:22 grubba Exp $
 *
 * Test Crypto.aes against the official test-vectors.
 *
 * Henrik Grubbström 2001-03-24
 */

// Read the raw vectors. 
constant raw_cbc_d_m = #string "rijndael_cbc_d_m.txt";
constant raw_cbc_e_m = #string "rijndael_cbc_e_m.txt";
constant raw_ecb_d_m = #string "rijndael_ecb_d_m.txt";
constant raw_ecb_e_m = #string "rijndael_ecb_e_m.txt";
constant raw_ecb_iv = #string "rijndael_ecb_iv.txt";
constant raw_ecb_tbl = #string "rijndael_ecb_tbl.txt";
constant raw_ecb_vk = #string "rijndael_ecb_vk.txt";
constant raw_ecb_vt = #string "rijndael_ecb_vt.txt";

int run_test(string raw, function(mapping(string:string):int) fun)
{
  int fail;
  foreach (replace(raw, "\r\n", "\n") / "\n\n", string segment) {
    if (has_prefix(segment, "==") || !has_value(segment, "=")) continue;
    if (fun(aggregate_mapping(@(map(segment/"\n" - ({""}), `/, "=") *
				({}))))) {
      werror("\nFailure for vector:\n"
	     "%s\n", segment);
      fail = 1;
    }
  }
  return fail;
}

int check_ecb_e_m()
{
  int fail;
  string keysize;

  object aes = Crypto.aes();

  fail = run_test(raw_ecb_e_m, lambda(mapping(string:string) v) {
    if (v->KEYSIZE) {
      keysize = v->KEYSIZE;
      if (keysize) write("\n");
      return;
    }
    if (!v->I) return;

    write("Rijndael ECB Encryption (%s): %s\r", keysize, v->I);

    string pt = Crypto.hex_to_string(v->PT);
    string ct = Crypto.hex_to_string(v->CT);

    aes->set_encrypt_key(Crypto.hex_to_string(v->KEY));

    for(int i = 0; i < 10000; i++) {
      pt = aes->crypt_block(pt);
    }

    return pt != ct;
  });

  write("\n");
  return fail;
}

int check_ecb_d_m()
{
  int fail;
  string keysize;

  object aes = Crypto.aes();

  fail = run_test(raw_ecb_e_m, lambda(mapping(string:string) v) {
    if (v->KEYSIZE) {
      keysize = v->KEYSIZE;
      if (keysize) write("\n");
      return;
    }
    if (!v->I) return;

    write("Rijndael ECB Decryption (%s): %s\r", keysize, v->I);

    string pt = Crypto.hex_to_string(v->PT);
    string ct = Crypto.hex_to_string(v->CT);

    aes->set_decrypt_key(Crypto.hex_to_string(v->KEY));

    for(int i = 0; i < 10000; i++) {
      ct = aes->crypt_block(ct);
    }

    return pt != ct;
  });

  write("\n");
  return fail;
}

int check_cbc_e_m()
{
  int fail;
  string keysize;

  object aes_cbc = Crypto.cbc(Crypto.aes());

  fail = run_test(raw_cbc_e_m, lambda(mapping(string:string) v) {
    if (v->KEYSIZE) {
      keysize = v->KEYSIZE;
      if (keysize) write("\n");
      return;
    }
    if (!v->I) return;

    write("Rijndael CBC Encryption (%s): %s\r", keysize, v->I);

    string pt = Crypto.hex_to_string(v->PT);
    string ct = Crypto.hex_to_string(v->CT);
    string iv = Crypto.hex_to_string(v->IV);

    aes_cbc->set_encrypt_key(Crypto.hex_to_string(v->KEY));
    aes_cbc->set_iv(iv);

    pt += iv;

    for(int i = 0; i < 5000; i++) {
      pt = aes_cbc->crypt_block(pt);
    }

    return pt[16..] != ct;
  });

  write("\n");
  return fail;
}

int check_cbc_d_m()
{
  int fail;
  string keysize;

  object aes_cbc = Crypto.cbc(Crypto.aes());

  fail = run_test(raw_cbc_d_m, lambda(mapping(string:string) v) {
    if (v->KEYSIZE) {
      keysize = v->KEYSIZE;
      if (keysize) write("\n");
      return;
    }
    if (!v->I) return;

    write("Rijndael CBC Decryption (%s): %s\r", keysize, v->I);

    string pt = Crypto.hex_to_string(v->PT);
    string ct = Crypto.hex_to_string(v->CT);
    string iv = Crypto.hex_to_string(v->IV);

    aes_cbc->set_decrypt_key(Crypto.hex_to_string(v->KEY));
    aes_cbc->set_iv(iv);

    for(int i = 0; i < 10000; i++) {
      ct = aes_cbc->crypt_block(ct);
    }

    return pt != ct;
  });

  write("\n");
  return fail;
}

int main(int argc, array(string) argv)
{
  return check_ecb_e_m() | check_ecb_d_m() | check_cbc_e_m() | check_cbc_d_m();
}
