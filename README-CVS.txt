$Id: README-CVS.txt,v 1.13 2002/04/06 20:49:08 mikael%unix.pp.se Exp $

HOW TO BUILD PIKE FROM CVS

The top-level makefile (in this directory, not the src directory) has
all the magic you need to build pike directly from CVS.  Just type
'make'.

Building Pike from cvs, in addition to the requirements for a normal
build, requires a working Pike. You will also need automake, gnu m4
and bison.

Other interesting make targets are:

install		    compile and install in default location
install_interactive interactive install
tinstall	    test install, ie. install in build directory
verify		    run the testsuite
run_hilfe	    run hilfe without installing pike
source		    prepare the source tree for compiliation without
		    the need for a Pike.
export		    create a source dist and bump up the build number
		    (if you have cvs write access). Please DO NOT
		    check in the generated files.
cvsclean	    clean the source tree for cvs actions.

You may also like to modify some variables in the beginning of the
Makefile. 

If that doesn't work, or you like making things difficult for
yourself, try the Old instructions:

1) cd src ; ./run_autoconfig
   This creates configure files and Makefile.in files.

2) Create a build directory an cd to it.  Do NOT build in the source
   dir, doing so will make it impossible to do 'make export' later.

3) Run the newly created configure file located in the src dir from
   the build dir.  Make sure to use an absolute path! This creates the
   Makefiles you need, e.g. Makefile from Makefile.in and machine.h
   from machine.h.in.  If you don't use an absolute path the debug
   information will be all warped...

   Some options for ./configure are:
   --prefix=/foo/bar         if you want to install Pike in /foo/bar,
                             default is /usr/local.
   --without-gdbm            compile without gdbm support
   --without-gmp             compile without gmp support
   --without-rtldebug        compile without runtime debugging
   --without-cdebug          compile without debug symbols (-g)
   --without-debug           compile without rtldbug and cdebug
   --without-threads         compile without threads support (See
                             below)
   --without-zlib            compile without gzip compression libary
                             support
   --without-dynamic-modules compile statically, no dynamic loading
                             used. (makes binary larger)
   --without-mysql           compile without mysql support
   --with-profiling          enables profiling pike code but slows
                             down interpreter a little
   --with-poll               use poll instead of select
   --with-dmalloc            compile with memory tracking, makes pike
                             very slow, use for debugging only.
   --without-copt            compile without -O2
   --without-bignums         disable support for large integers
   --with-security           enable internal object security system

   You might also want to set the following environment variables:
   CFLAGS     Put extra flags for your C compiler here.
   CPPFLAGS   Put extra flags for your C preprocessor here
              (such as -I/usr/gnu/include)
   LDFLAGS    Put extra flags to your linker here, such as
              -L/usr/gnu/lib  and -R/usr/gnu/lib

   Use the above flags to make sure the configure script finds the
   gdbm and gmp libraries and include files if you need or want those
   modules.  If the configure script doesn't find them, Pike will
   still compile, but without those modules.

4) Run 'make depend'
   This updates all the Makefile.in files in the source dir to contain
   the correct dependencies

5) If needed, edit config.h and Makefile to suit your purposes.  I've
   tried to make it so that you don't have to change config.h or
   Makefile at all.  If you need to do what you consider 'unnecessary
   changes' then mail us and we'll try to fit it into configure.  If
   possible, use gnu make, gcc, gnu sed and bison

6) Run 'make'
   This builds pike.

7) Optionally, run 'make verify' to check that the compiled driver
   works as it should (might be a good idea).  This will take a little
   time and use quite a lot of memory, because the test program is
   quite large.  If everything works out fine no extra messages are
   written.

8) If you want to install Pike, type 'make install'.

After doing this, DO NOT, commit the generated files.  They are placed
in .cvsignore files so you shouldn't have to bother with them. 

IF IT DOESN'T WORK:

 o Try again.

 o Your sh might be too buggy to run ./configure. (This is the case on
   A/UX) Try using bash, zsh or possibly ksh.  To use bash, first run
   /bin/sh and
   type:
   $ CONFIG_SHELL=full_path_for_bash
   $ export CONFIG_SHELL
   $ $CONFIG_SHELL ./configure

 o If you are not using GNU make, compile in the source dir rather
   than using a separate build dir.

 o ./configure relies heavily on sed, if you have several sed in your
   path try another sed (preferably gnu sed).

 o configure might have done something wrong, check machine.h and
   report any errors back to us.

 o Your gmp/gdbm libraries might not be working or incorrectly
   installed; start over by running configure with the appropriate
   --without-xxx arguments.  Also note that threads might give
   problems with I/O and signals.  If so you need to run configure
   --without-threads.

 o Try a different compiler, malloc, compiler-compiler and/or make
   (if you have any other).


BUGS

If you find a bug in the interpreter, the first thing to do is to make
sure the interpreter is compiled with PIKE_DEBUG defined.  If not,
recompile with PIKE_DEBUG and see if you get another error.  When
you've done this, please report the bug to us at
http://community.roxen.com/crunch/ and include as much as you can
muster of the following:

  o The version of the driver. (Try pike --version or look in
    src/version.h)
  o What kind of system hardware/software you use (OS, compiler, etc.)
  o The piece of code that crashes or bugs, preferably in a very
    small pike-script with the bug isolated.  Please send a complete
    running example of something that makes the interpreter bug.
  o A description of what it is that bugs and when.
  o If you know how, then also give us a backtrace and dump of vital
    variables at the point of crash.
  o Or, if you found the error and corrected it, just send us the
    bugfix along with a description of what you did and why.
