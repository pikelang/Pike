#pike 8.1

inherit Yabu;

DB db(string a,string b) {
  return DB(a,b);
}

LookupDB lookup(string a,string b,mapping|void c) {
  return LookupDB(a,b,c);
}
