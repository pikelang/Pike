// This file is part of Roxen Search
// Copyright © 2001 Roxen IS. All rights reserved.
//
// $Id: debug.h,v 1.2 2001/06/22 01:28:36 nilsson Exp $

#define TRACE werror("### %s(%d)\n", __FILE__, __LINE__);
#define SHOW(x) werror("### %s(%d) %s == %O\n", __FILE__, __LINE__, #x, (x));
