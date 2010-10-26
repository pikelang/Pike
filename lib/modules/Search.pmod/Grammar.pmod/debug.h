// $Id$

#define TRACE werror("### %s:%d\n"	\
                     "  %O()\n"		\
		     "  %{%O, %}\n",	\
		     __FILE__, __LINE__,\
		     backtrace()[-1][2],\
		     column(map(indices(allocate(5)),peek), 1))
#define SHOW(x) werror("### %s(%d) %s == %O\n", __FILE__, __LINE__, #x, (x))
