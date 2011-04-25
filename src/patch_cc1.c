/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/*
 * Patch gcc so that it doesn't use .ua{half,word} directives.
 *
 * Henrik Grubbström 2000-01-25
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#define BUFSIZE	65536
#define OVERLAP 32

char buffer[BUFSIZE+OVERLAP];

int main(int argc, char **argv)
{
  int fd;
  int offset = 0;
  int len;
  if (argc < 2) {
    fprintf(stderr, "Usage:\n"
	    "\t%s <cc1>\n", argv[0]);
    exit(1);
  }

  if (!strcmp(argv[1], "-v")) {
    fprintf(stdout, "$Id$\n");
    exit(0);
  }

  if ((fd = open(argv[1], O_RDONLY)) < 0) {
    fprintf(stderr, "Failed to open \"%s\" for reading.\n", argv[1]);
    exit(1);
  }

  while ((len = read(fd, buffer + offset, BUFSIZE+OVERLAP-offset)) > 0) {
    int i;
    len += offset;
    for(i=0; i < len-8; i++) {
      if (buffer[i] == '.') {
	/* Note: We can use strcmp(), since the string should be
	 * NUL-terminated in the buffer. */
	if (!strcmp(buffer+i, ".uaword")) {
	  buffer[i+1] = 'w';
	  buffer[i+2] = 'o';
	  buffer[i+3] = 'r';
	  buffer[i+4] = 'd';
	  buffer[i+5] = 0;
	  buffer[i+6] = 0;
	} else if (!strcmp(buffer+i, ".uahalf")) {
	  buffer[i+1] = 'h';
	  buffer[i+2] = 'a';
	  buffer[i+3] = 'l';
	  buffer[i+4] = 'f';
	  buffer[i+5] = 0;
	  buffer[i+6] = 0;
	}
      }
    }
    if (len < BUFSIZE+OVERLAP) {
      /* At EOF */
      write(1, buffer, len);
      exit(0);
    }
    write(1, buffer, BUFSIZE);
    memcpy(buffer, buffer+BUFSIZE, offset = (len-BUFSIZE));
  }
  write(1, buffer, offset);
  exit(0);
}
