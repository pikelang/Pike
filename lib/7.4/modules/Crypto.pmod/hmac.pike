
#pike 7.5

inherit Crypto.HMAC;

static class Wrapper {
  static object h;
  function(void:string) asn1_id;
  void create(object _h) {
    h = _h;
    asn1_id = h->identifier;
  }
  string hash(string m) { return h->update(m)->digest(); }
  int digest_size() { return sizeof(hash("")); }
}

void create(object h, void|int b) {
  ::create(Wrapper(h()), b);
}
