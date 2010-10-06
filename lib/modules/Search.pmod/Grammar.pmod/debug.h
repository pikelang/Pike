// This file is part of Roxen Search
// Copyright © 2001 - 2009, Roxen IS. All rights reserved.
//
// $Id: debug.h,v 1.4 2009/05/25 18:26:52 mast Exp $

#define TRACE werror("### %s:%d\n"	\
                     "  %O()\n"		\
		     "  %{%O, %}\n",	\
		     __FILE__, __LINE__,\
		     backtrace()[-1][2],\
		     column(map(indices(allocate(5)),peek), 1))
#define SHOW(x) werror("### %s(%d) %s == %O\n", __FILE__, __LINE__, #x, (x))
