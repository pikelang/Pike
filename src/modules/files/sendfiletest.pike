#!/usr/local/bin/pike

/* $Id$ */

constant TEST_SIZE = 16384;

string testdata = Crypto.randomness.reasonably_random()->read(TEST_SIZE);

int testno;

object(Stdio.Port) loopback = Stdio.Port();
int loopbackport;

/*
 * Some helper functions.
 */

object(Stdio.File) From(string f)
{
  object(Stdio.File) from = Stdio.File();

  if (!from->open(f, "r")) {
    werror("Failed to open %O for reading.\n", f);
    exit(1);
  }
  return from;
}

object(Stdio.File) To(string f)
{
  object(Stdio.File) to = Stdio.File();

  if (!to->open(f, "cwt")) {
    werror("Failed to open %O for writing.\n", f);
    exit(1);
  }
  return to;
}

array(object(Stdio.File)) SocketPair()
{
  object(Stdio.File) sock1, sock2;
  sock1 = Stdio.File();
  sock1->connect("127.0.0.1", loopbackport);
  sock2 = loopback->accept();
  if(!sock2)
  {
    werror("Accept returned 0\n");
    exit(1);
  }
  return ({ sock1, sock2 });
}

void Verify()
{
  array(string) data = From("conftest.dst")->read()/TEST_SIZE;
  int i;
  for(i=0; i < sizeof(data); i++) {
    if (data[i] != testdata) {
      werror("Segment %d corrupted!\n", i);
      int j;
      for (j=0; j < TEST_SIZE; j++) {
	if (data[i][j] != testdata[j]) {
	  werror("First corrupt byte at segment offset %d: 0x%02x != 0x%02x\n",
		 j, data[i][j], testdata[j]);
	  exit(1);
	}
      }
      werror("Corrupt byte not found!\n");
      exit(1);
    }
  }
}

/*
 * The driver function.
 */

void next()
{
  testno++;

  function test;
  if (!(test = this_object()["test"+testno])) exit(0);
  mixed err;
  if (err = catch {
    werror("Sendfile test: %d\n", testno);
    test();
  }) {
    catch {
      werror("Test %d failed!\n"
	     "%s\n",
	     testno,
	     describe_backtrace(err));
    };
    exit(1);
  }
}

void done(int sent, int expected)
{
  if (sent != expected) {
    werror(sprintf("Test %d failed: %d != %d\n", testno, sent, expected));
    exit(1);
  }
  call_out(next, 0);
}

/*
 * The actual tests.
 */

void test1()
{
  /* First try sending out testdata to a plain file. */

  if (!Stdio.sendfile(testdata/1024, 0, 0, -1, 0,
		      To("conftest.src"), done, TEST_SIZE)) {
    werror("Stdio,sendfile() failed!\n");
    exit(1);
  }
}

void test2()
{
  /* Then try copying it to another file. */

  if (!Stdio.sendfile(0, From("conftest.src"), 0, -1, 0,
		      To("conftest.dst"), done, TEST_SIZE)) {
    werror("Stdio.sendfile() failed!\n");
    exit(1);
  }
}

void test3()
{
  Verify();

  /* Try with a headers + file + trailers combo. */

  if (!Stdio.sendfile(testdata/4096, From("conftest.src"), 0, -1,
		      testdata/512, To("conftest.dst"), done, TEST_SIZE*3)) {
    werror("Stdio.sendfile() failed!\n");
    exit(1);
  }
}

void test4()
{
  Verify();

  /* Try a loopback test. */

  array(object) pair = SocketPair();

  if (!Stdio.sendfile(testdata/4096, From("conftest.src"), 0, -1,
		      testdata/512, pair[0], done, TEST_SIZE*3)) {
    werror("Stdio.sendfile() failed!\n");
    exit(1);
  }

  if (!Stdio.sendfile(testdata/4096, pair[1], 0, -1,
		      testdata/512, To("conftest.dst"), done, TEST_SIZE*5)) {
    werror("Stdio.sendfile() failed!\n");
    exit(1);
  }
}

void test5()
{
  /* Dummy (test 4 will call done twice). */
}

void test6()
{
  Verify();

  next();
}

void test7()
{
  /* Clean up. */
  rm("conftest.src");
  rm("conftest.dst");
  next();
}

/*
 * Start the backend.
 */

int main(int argc, array(string) argv)
{
  werror("\n");
  loopback->bind(0);
  loopbackport = (int)((loopback->query_address()/" ")[1]);
  call_out(next, 0);
  return -1;
}
