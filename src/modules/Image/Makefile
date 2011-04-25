# $Id$

module_testsuite.in: testsuite.in.in mktests.pike
	pike mktests.pike -t testsuite.in.in >module_testsuite.in

tests.pike: testsuite.in.in mktests.pike
	pike mktests.pike testsuite.in.in >tests.pike

test:	tests.pike
	pike tests.pike
