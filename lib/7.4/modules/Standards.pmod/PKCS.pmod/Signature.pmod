
#pike 7.5

inherit Standards.PKCS.Signature;

string build_digestinfo(string msg, object hash) {
  string d = hash->update(msg)->digest();
  string id = hash->identifier();

  return sprintf("%c%c%c%c%c%c%s%c%c%c%c%s",
		 0x30, sizeof(id) + sizeof(d) + 8, 0x30, sizeof(id) + 4,
		 0x06, sizeof(id), id, 0x05, 0x00, 0x04, sizeof(d), d);
}
