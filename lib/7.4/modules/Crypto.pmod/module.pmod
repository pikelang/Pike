#pike 7.5

string crypt_md5(string pw, void|string salt) {
  sscanf(pw, "%s\0", pw);
  sscanf(salt, "%s\0", salt);
  return Crypto.crypt_md5(pw,salt);
}

constant string_to_hex = __builtin.string2hex;
constant hex_to_string = __builtin.hex2string;
