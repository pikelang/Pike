/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/*
 * smartlink - A smarter linker.
 * Based on the /bin/sh script smartlink 1.23.
 *
 * Henrik Grubbström 1999-03-04
 */

/* NOTE: Use confdefs.h and not machine.h, since we are compiled by configure
 */
#include "confdefs.h"

#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */

char *prog_name;

void fatal(char *message)
{
  fprintf(stderr, "%s: %s", prog_name, message);
  exit(1);
}

/* Not terribly efficient, but... */
int add_path(char *buffer, char *path)
{
  char *p = buffer - 1;
  int len = strlen(path);

  if (!len) return 0;	/* The empty path is not meaningful. */

  do {
    p++;
    if (!strncmp(p, path, len) && (p[len] == ':')) {
      return 0;
    }
  } while ((p = strchr(p, ':')));

  strcat(buffer, path);
  strcat(buffer, ":");

  return 1;
}

int main(int argc, char **argv)
{
  int i;
  int buf_size;
  char *path;
  char *buffer;
  char *lpath;
  char *rpath;
  int rpath_in_use = 0;
  char *lp;
  char *rp;
  char *full_rpath;
  char **new_argv;
  char *ld_lib_path;
  int new_argc;
  int n32 = 0;
  int linking = 1;	/* Maybe */

  prog_name = argv[0];

  if (argc < 2) {
    fatal("Too few arguments\n");
  }

  if (!strcmp(argv[1], "-v")) {
    fprintf(stdout,
	    "$Id$\n"
	    "Usage:\n"
	    "\t%s binary [args]\n",
	    argv[0]);
    exit(0);
  }
  
  if (putenv("LD_PXDB=/dev/null")) {
    fatal("Out of memory (1)!\n");
  }
  if (!getenv("SGI_ABI")) {
    if (putenv("SGI_ABI=-n32")) {
      fatal("Out of memory (2)!\n");
    }
  }

  path = getenv("PATH");

  buf_size = 0;

  if ((ld_lib_path = getenv("LD_LIBRARY_PATH"))) {
    buf_size += strlen(ld_lib_path);
  }

  for(i=0; i < argc; i++) {
    buf_size += strlen(argv[i]);
  }

  buf_size += 2*argc + 20;	/* Some extra space */

  if (path && !(buffer = malloc(strlen(path) + strlen(argv[1]) + 4))) {
    fatal("Out of memory (2.5)!\n");
  }

  if (!(rpath = malloc(buf_size))) {
    fatal("Out of memory (3)!\n");
  }

  if (!(lpath = malloc(buf_size))) {
    fatal("Out of memory (4)!\n");
  }

  rpath[0] = 0;
  lpath[0] = 0;

  /* 150 extra args should be enough... */
  if (!(new_argv = malloc(sizeof(char *)*(argc + 150)))) {
    fatal("Out of memory (5)!\n");
  }

  new_argc = 0;
  full_rpath = rpath;

#ifdef USE_Wl
  strcat(rpath, "-Wl,-rpath,");
#elif defined(USE_Wl_R)
  strcat(rpath, "-Wl,-R");
#elif defined(USE_R)
  strcat(rpath, "-R");
#elif defined(USE_LD_LIBRARY_PATH)
  strcat(rpath, "LD_LIBRARY_PATH=");
#endif /* defined(USE_Wl) || defined(USE_Wl_R) || defined(USE_R) || defined(USE_LD_LIBRARY_PATH) */
  rpath += strlen(rpath);

  new_argv[new_argc++] = argv[1];

  if (!strcmp(argv[1], "cpp")) {
    /* Not linking */
    linking = 0;
  }

  /* NOTE: Skip arg 1 */
  for(i=2; i<argc; i++) {
    if (argv[i][0] == '-') {
      if ((argv[i][1] == 'R') || (argv[i][1] == 'L')) {
	/* -R & -L */
	if (argv[i][1] == 'L') {
	  if (!argv[i][2]) {
	    if (i+1 < argc) {
	      if (add_path(lpath, argv[i+1])) {
		new_argv[new_argc++] = argv[i];
		new_argv[new_argc++] = argv[i+1];
	      }
	    }
	  } else {
	    if (add_path(lpath, argv[i]+2)) {
	      new_argv[new_argc++] = argv[i];
	    }
	  }
	}
	if (!argv[i][2]) {
	  i++;
	  if (i < argc) {
	    rpath_in_use |= add_path(rpath, argv[i]);
	  }
	} else {
	  rpath_in_use |= add_path(rpath, argv[i] + 2);
	}
	continue;
      } else if ((argv[i][1] == 'n') && (argv[i][2] == '3') &&
		 (argv[i][3] == '2') && (!argv[i][4])) {
	/* -n32 */
	n32 = 1;
	continue;	/* Skip */
      } else if (((argv[i][1] == 'c') || (argv[i][1] == 'E')) &&
		 (!argv[i][2])) {
	/* Not linking */
	linking = 0;
      }
    }
    new_argv[new_argc++] = argv[i];
  }

#ifndef USE_Wl
  /* This code strips '-Wl,' from arguments if the 
   * linker is '*ld'
   */
  if(linking)
  {
    int len=strlen(argv[1]);
    if(strchr(argv[1],' ')) len=strchr(argv[1],' ') - argv[1];
    if(len > 1 && argv[1][len-2]=='l' && argv[1][len-1]=='d')
    {
      for(i=2; i<argc; i++) {
	if (new_argv[i][0] == '-' && new_argv[i][1]=='W' &&
	    new_argv[i][2]=='l' && new_argv[i][3]==',')
	{
	  char *ptr;
	  new_argv[i]=new_argv[i]+4;

	  while((ptr=strchr(new_argv[i],',')))
	  {
	    int e;
	    *ptr=0;
	    for(e=argc;e>=i;e--) new_argv[e+1]=new_argv[e];
	    new_argv[i+1]=ptr+1;
	    i++;
	  }
	}
      }
    }
  }
#endif

  if (n32) {
    i = new_argc++;
    /* Note don't copy index 0 */
    while(--i) {
      new_argv[i+1] = new_argv[i];
    }
    new_argv[1] = "-n32";
  }

  if (ld_lib_path) {
    char *p;

    while (p = strchr(ld_lib_path, ':')) {
      *p = 0;
      rpath_in_use |= add_path(rpath, ld_lib_path);
      *p = ':';		/* Make sure LD_LIBRARY_PATH isn't modified */
      ld_lib_path = p+1;
    }
    rpath_in_use |= add_path(rpath, ld_lib_path);
  }

  if (rpath_in_use) {
    /* Delete the terminating ':' */
    rpath[strlen(rpath) - 1] = 0;

#ifdef USE_RPATH
    new_argv[new_argc++] = "-rpath";
    new_argv[new_argc++] = rpath;
#elif defined(USE_PLUS_b)
    if (linking) {
      /* +b: is probaly enough, but... */
      new_argv[new_argc++] = "+b";
      new_argv[new_argc++] = rpath;
    }
#elif defined(USE_YP_)
    if (linking) {
      new_argv[new_argc++] = "-YP,";
      new_argv[new_argc++] = rpath;
    }
#elif defined(USE_XLINKER_YP_)
    if (linking) {
      new_argv[new_argc++] = "-Xlinker";
      new_argv[new_argc++] = "-YP,";
      new_argv[new_argc++] = "-Xlinker";
      new_argv[new_argc++] = rpath;
    }
#elif defined(USE_Wl)
    if (linking) {
      new_argv[new_argc++] = full_rpath;
    }
#elif defined(USE_R) || defined(USE_Wl_R)
    new_argv[new_argc++] = full_rpath;
#elif defined(USE_LD_LIBRARY_PATH)
    if (putenv(full_rpath)) {
      fatal("Out of memory (6)!");
    }
    /* LD_LIBRARY_PATH
     *     LD_RUN_PATH
     */
    memcpy(full_rpath + 4, "LD_RUN_PATH", 11);
    if (putenv(full_rpath + 4)) {
      fatal("Out of memory (7)!");
    }
#else
#error Unknown method
#endif
  }

  new_argv[new_argc++] = NULL;

  if ((argv[1][0] != '/') && path) {
    /* Perform a search in $PATH */
    struct stat stat_buf;
    char *p;
    while ((p = strchr(path, ':'))) {
      *p = 0;
      strcpy(buffer, path);
      *p = ':';		/* Make sure PATH isn't modified */
      strcat(buffer, "/");
      strcat(buffer, argv[1]);
      /* fprintf(stderr, "Trying %s...\n", buffer); */
      if (!stat(buffer, &stat_buf)) {
	/* Found. */
	argv[1] = buffer;
	break;
      }
      path = p + 1;
    }
    if (!p) {
      strcpy(buffer, path);
      strcat(buffer, "/");
      strcat(buffer, argv[1]);
      /* fprintf(stderr, "Trying %s...\n", buffer); */
      if (!stat(buffer, &stat_buf)) {
	/* Found */
	argv[1] = buffer;
      }
    }
  }

  execv(argv[1], new_argv);
  fprintf(stderr, "%s: exec of %s failed!\n", argv[0], argv[1]);
  exit(1);
}
