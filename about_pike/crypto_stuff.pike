/* encrypt.pike
 *
 */

// The name of the receiver:
#define NAME "nisse"

int main(int argc, array(string) argv)
{
  object rsa = keys.pub_to_rsa(Stdio.read_file(NAME + ".pub"));
  //Reads a rsa-key from nisse.pub and converts it to a rsa-object.


  function r = Crypto.randomness.reasonably_random()->read;
  //Gets a function that kan be used for generation of random bits.


  string iv = r(8);
  //Creates a random 8-octet initvector (Just as big as a IDEA-block)

  string session_key = r(16);
  //Creates a 16 octet (128-bit, key size for IDEA) seasion key.

  object cipher = Crypto.crypto(Crypto.idea_cbc()
    ->set_encrypt_key(session_key)
    ->set_iv(iv));
  //creates an object that crypts with IDEA in CBC-mode

  
  string data = Stdio.stdin->read(0x7fffffff);
  //Reads a file from Standard Input.

  if (!data)
  {
    werror("Read from stdin failed.\n");
    return 1;
  }

  string gibberish = cipher->crypt(data) + cipher->pad();
  //Crypt the file with IDEA-CBC-crypto-object

  string encrypted_key = rsa->encrypt(session_key);
  //Crypt season key with RSA.
  
  string data = sprintf("%s%s%s%s", iv,
			iv ^ sprintf("%4c%4c",
				     strlen(encrypted_key),
				     strlen(gibberish)),
			encrypted_key,
			gibberish);
  //Writes the crypted file with the format:
  // IV (8 octets)
  // IV XOR (Length of crypted saeason key and crypted data. 8 octets)
  // season key, crypted with RSA
  // data, crypted with season key.
	


  if (strlen(data) != Stdio.stdout->write(data))
  {
    werror("Write to stdout failed.\n");
    return 1;
  }

  return 0;
}
