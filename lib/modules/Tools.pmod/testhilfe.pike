
class TestHilfe {
  inherit Tools.Hilfe.Evaluator;

  string out = "";

  string get() {
    string ret = out;
    out = "";
    return ret;
  }

  int(0..) mywrite(string in, mixed ... args) {
    out += sprintf(in, @args);
    return 0;
  }

  void create() {
    write = mywrite;
    ::create();
  }

  function put = add_input_line;

  int(0..1) test(string|array(string) in, string result) {
    if(arrayp(in))
      foreach(in, string line)
	put(line);
    else
      put(in);

    string res = get();
    if(res!=result) {
      werror("Error: %O\n", ({ in, res, result }) );
      return 0;
    }
    if(!state->finishedp()) {
      werror("Error: %O didn't finish.\n", in);
      state->flush();
      return 0;
    }
    return 1;
  }
}

TestHilfe testhilfe=TestHilfe();
int tests;
int fails;

void test(string|array(string) in, string result) {
  tests++;
  if(!testhilfe->test(in, result))
    fails++;
}


int main() {
  test(".", "Pike v7.3 release 31 running Hilfe v3.3 (Incremental Pike Frontend)\n");
  test("set format sprintf \"%s\"", "");
  test("1;", "1");

  // Testing parser
  test( ({ "(", "." }), "");
  test( ({ "int", "." }), "");
  test("string a=\"hej", "Hilfe Error: Unterminated string.\n");
  test("({)", "Hilfe Error: \")\" end parenthesis does not match "
       "closest start parenthesis \"{\".\n");
  test( ({ "int c // =3;", "=4;" }), "");
  test("c;", "4");
  test( ({ "string a=#\"hej", "san\";" }), "");
  test("a;", "\"hej\\nsan\"");
  test("class A{}", "");
  test("A();", "object");
  test("A a=A();", "");
  //  test("class A{}();", "object");

  test("void foo(function f) { write(\"a\"); f(); write(\"b\"); }", "");
  //  test("foo() { write(\"-\"); }", "a-b");

  // Test variable delarations.
  test("int|string a=2;", "");
  test("a=2.0;", "2.000000");
  test("int b=2;", "");
  test("b=2.0;", "2.000000"); // We should get an error here.
  test("Image.Image i=Image.Image();", "");
  test("i;", "Image.Image( 0 x 0 /* 0.0Kb */)");
  test("constant z=`+(1,2),y=2;", "");
  test("z;", "3");
  test("y;", "2");
  test("int z;", "Hilfe Error: \"z\" already defined as constant.\n");
  test("int y;", "Hilfe Error: \"y\" already defined as constant.\n");

  // Test variable bindings.
  test("new", "");
  test("int i;", "");
  test("i;", "0");
  test("void foo() { i++; }", "");
  test("foo();", "0");
  test("i;", "1");

  test("for(int j; j<5; j++) write(\"%d\",j);", "01234Ok.\n");
  test("j;", "Compiler Error: 1:'j' undefined.\n");
  test("for(int i; i<5; i++) write(\"%d\",i);", "01234Ok.\n");
  test("i;", "1");
  test("catch { for(int i; i<5; i++) write(\"%d\",i); };", "012340");
  test("i;", "1");

  // Clear history...
  testhilfe=TestHilfe();
  test(".", "Pike v7.3 release 31 running Hilfe v3.3 (Incremental Pike Frontend)\n");
  test("set format sprintf \"%s\"", "");

  // Testing history.
  test("2;", "2");
  test("_*_;", "4");
  test("_*_;", "16");
  test("_==__[-1];", "1");
  test("__[1]+__[2]+__[-1];", "7");

  werror("Did %d tests, %d failed.\n", tests, fails);
}
