#pike 7.6

string crypt_md5(string pw, void|string salt) {
  sscanf(pw, "%s\0", pw);
  sscanf(salt, "%s\0", salt);
  return Crypto.crypt_md5(pw,salt);
}

constant string_to_hex = __builtin.string2hex;
constant hex_to_string = __builtin.hex2string;

// FIXME: Only compatible when sizeof(parameter)==8.
function(string:string) des_parity = ___Nettle.DES_Info()->fix_parity;

// These haven't been modified.
constant arcfour = Crypto.arcfour;
constant cbc = Crypto.cbc;
constant des = Crypto.des;
constant des_cbc = Crypto.des_cbc;
constant des_parity = Crypto.des_parity;
constant des3 = Crypto.des3;
constant des3_cbc = Crypto.des3_cbc;
constant dsa = Crypto.dsa;
constant hmac = Crypto.hmac;
constant idea = Crypto.idea;
constant idea_cbc = Crypto.idea_cbc;
constant pipe = Crypto.pipe;
constant randomness = Crypto.randomness;
constant rsa = Crypto.rsa;

