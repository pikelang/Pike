// This file is part of Roxen Search
// Copyright © 2001 Roxen IS. All rights reserved.
//
// $Id: debug.h,v 1.3 2002/03/11 14:26:58 grubba Exp $

#define TRACE werror("### %s:%d\n"	\
                     "  %O()\n"		\
		     "  %{%O, %}\n",	\
		     __FILE__, __LINE__,\
		     backtrace()[-1][2],\
		     column(map(indices(allocate(5)),peek), 1))
#define SHOW(x) werror("### %s(%d) %s == %O\n", __FILE__, __LINE__, #x, (x))
