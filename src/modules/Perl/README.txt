This file contains various notes about the Perl support.
( $Id$ )

PERL 5.6.x:
   The semantics of Perl's embedding interface seems to be in flux, changing
   a bit between versions; the most substantial problem with this that I've
   noticed is that Perl 5.6.0 and 5.6.1 have a tendency to cause a core
   dump when trying to create a second instance of a Perl interpreter
   from Pike. This doesn't seem to happen with either Perl 5.005_03 or
   5.7.3, however, even when compiling with the exact same Pike-to-Perl
   glue code that failed for Perl 5.6.x.

THREAD SUPPORT:
   A Perl compiled with the Perl thread support causes things to change
   in how certain include files behave, which in turn can cause the
   compilation of Pike with Perl support to fail with errors like "`thr'
   undeclared" when expanding the Perl dSP macro. Searching some mail
   archives yields suggestions that "`thr' undeclared" errors can be
   cured by adding a dTHR, but dTHR is the first thing done by the dSP
   macro, and adding an explicit dTHR right before the dSP did (predictably)
   not help when I tried it. And I haven't had the time to investigate
   this very much.

MULTIPLICITY:
   Using the Pike Perl support with Perls where MULTIPLICITY is enabled
   is largely uncharted territory, although it should work in theory.

RUNNING PIKE INTERACTIVELY:
   When running Pike interactively, remember that new versions of the
   interactive frontend Hilfe keeps a history of results of evaluation of
   previous commands. This means that objects that are referenced from
   the history, in particular the Perl interpreter object, are not freed
   until they fall out of the history list. Since there can only be one
   Perl interpreter object alive at the same time (unless Perl has been
   compiled with MULTIPLICITY enabled; see the note about this above),
   this may force you to empty the history list or disable the history
   feature if you need to create a new Perl interpreter object during the
   same Hilfe session.
