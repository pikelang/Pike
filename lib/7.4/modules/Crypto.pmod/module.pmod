#pike 7.5

inherit Crypto;

string crypt_md5(string pw, void|string salt) {
  sscanf(pw, "%s\0", pw);
  sscanf(salt, "%s\0", salt);
  ::crypt_md5(pw,salt);
}
