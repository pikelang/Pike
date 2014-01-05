
#pike __REAL_VERSION__

/* There should really be a deprecation warning given when using this
   file, but we want to be able to disable it with the #pragma and we
   want to report the error for the file that prompts the resolving. I
   don't think any single place (cpp, lexer, master, runtime) has all
   information. */

private object c = master()->resolv("Charset");
private mixed `[](string id) { return c[id]; }
private mixed `->(string id) { return c[id]; }
private array _indices() { return indices(c); }
private array _values() { return values(c); }
