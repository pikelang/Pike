# $Id: Makefile,v 1.20 2002/02/04 14:51:26 nilsson Exp $

module_testsuite.in: testsuite.in.in mktests.pike
	pike mktests.pike -t testsuite.in.in >module_testsuite.in

tests.pike: testsuite.in.in mktests.pike
	pike mktests.pike testsuite.in.in >tests.pike

test:	tests.pike
	pike tests.pike
