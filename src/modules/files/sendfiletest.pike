#!/usr/local/bin/pike

/* $Id: sendfiletest.pike,v 1.3 2000/10/06 21:18:10 grubba Exp $ */

constant TEST_SIZE = 16384;

string testdata = Crypto.randomness.reasonably_random()->read(TEST_SIZE);

int testno;

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
  next();
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
  /* Check that the testdata still is correct. */

  if (From("conftest.dst")->read() != testdata) {
    werror("Data corruption!\n");
    exit(1);
  }

  /* Try with a headers + file + trailers combo. */

  if (!Stdio.sendfile(testdata/4096, From("conftest.src"), 0, -1,
		      testdata/512, To("conftest.dst"), done, TEST_SIZE*3)) {
    werror("Stdio.sendfile() failed!\n");
    exit(1);
  }
}

void test4()
{
  /* Check that the testdata still is correct. */

  if (From("conftest.dst")->read() != testdata*3) {
    werror("Data corruption!\n");
    exit(1);
  }

  next();
}

void test5()
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
  call_out(next, 0);
  return -1;
}
