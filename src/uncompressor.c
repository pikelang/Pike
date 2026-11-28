/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#ifndef __NT__
#include <unistd.h>
#else
#include <windows.h>
#include <io.h>
#include <direct.h>
#endif
#include <zlib.h>


#ifdef __NT__
#define DELIM ';'
#define SLASH '\\'
#define ISSLASH(X) ((X)=='\\' || (X)=='/')
#else
#define DELIM ':'
#define SLASH '/'
#define ISSLASH(X) ((X)=='/')
#endif

#define FEND(X,Y) (strchr(X,Y)?strchr(X,Y):(X)+strlen(X))

/* #define DEBUG */

#ifdef DEBUG
#define WERR(...) fprintf(stderr,__VA_ARGS__)
#else
#define WERR(...)
#endif

gzFile gz;

static void gr(char *ptr, int len)
{
  if(gzread(gz, ptr, len) != len)
  {
    puts("Failed to decompress data!\n");
    exit(1);
  }
}

#define GR(X) gr( (char *) & (X), sizeof(X))
static unsigned int read_int(void)
{
  unsigned char x[4];
  gr((char *)x, sizeof(x));
  return ((unsigned int)x[0] << 24) | (x[1]<<16) | (x[2]<<8) | x[3];
}

#if !( SEEK_TO - 0 )
#define SEEK_TO -1
#endif

#if !( SEEK_SET - 0 )
#define SEEK_SET 0
#endif


static char *alloc_buffer(size_t size)
{
  char *buffer = malloc(size);

  if (!buffer) {
    fputs("Out of memory unpacking installer.\n", stderr);
    exit(1);
  }
  return buffer;
}

#ifdef __NT__
static int open_archive(const WCHAR *file)
{
  HANDLE handle = CreateFileW(file, GENERIC_READ, FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  int fd;

  if (handle == INVALID_HANDLE_VALUE) return -1;
  fd = _open_osfhandle((intptr_t)handle, _O_RDONLY | _O_BINARY);
  if (fd == -1) CloseHandle(handle);
  return fd;
}
#endif

static void my_uncompress(int fd, int argc, char **argv)
{
  const char *d;

  if(fd == -1) return;

  if(lseek(fd, SEEK_TO, SEEK_SET)<0)
  {
    perror("lseek");
    exit(1);
  }

  gz=gzdopen(fd,"rb");
  if(!gz)
  {
    puts("gzdopen failed!");
    exit(1);
  }

#ifdef __NT__
  d=getenv("TMP");
  if(!d) d=getenv("TEMP");
  if(!d) d="C:\\Windows\\temp";
#else
  d=getenv("TMPDIR");
  if(!d) d="/tmp";
#endif
  if(chdir(d)<0)
  {
    perror("chdir");
    exit(1);
  }
  WERR("Changing dir to %s\n",d);

  while(1)
  {
    char type;
    unsigned int len;
    char *buffer;
    GR(type);

    len=read_int();
    WERR("namelen=%u\n",len);
    if (len > INT_MAX) {
      fputs("Installer entry name too long.\n", stderr);
      exit(1);
    }
    buffer = alloc_buffer((size_t)len + 1);
    gr(buffer, len);
    buffer[len]=0;

    WERR("Type %c: %s\n",type,buffer);

   switch(type)
    {
      case 'q': /* quit */
	exit(0);

      case 'w': /* write */
	puts(buffer);
	break;

      case 'e': /* setenv */
        WERR("putenv(%s)\n",buffer);
        /* On POSIX, putenv retains the supplied string. */
#ifdef __NT__
        _putenv(buffer);
#else
        if (!putenv(buffer)) buffer = NULL;
#endif
        break;

      case 's': /* system */
	/* We ignore error so that we can continue with
	 * file deletion
	 */
        if (len && buffer[len - 1] == '$') {
          size_t size = (size_t)len;
          char *command, *ptr;
          int e;

          for (e = 0; e < argc; e++) {
            size_t arglen = strlen(argv[e]);

            if (size > (size_t)-1 - 3 ||
                arglen > ((size_t)-1 - size - 3) / 4) {
              fputs("Installer command too long.\n", stderr);
              exit(1);
            }
            size += 4 * arglen + 3;
          }
          command = alloc_buffer(size);
          memcpy(command, buffer, len - 1);
          free(buffer);
          buffer = command;
          ptr = buffer + len - 1;
	  for(e=0;e<argc;e++)
	  {
	    char *c;
            WERR("argv[%d]=%s\n",e,argv[e]);

	    *(ptr++)=' ';
	    *(ptr++)='"';
	    for(c=argv[e];*c;c++)
	    {
	      switch(*c)
	      {
		case '\\':
#ifdef __NT__
		  *(ptr++)='"';
		  *(ptr++)='\\';
		  *(ptr++)='\\';
		  *(ptr++)='"';
		  break;
#endif
		case '"':
		  *(ptr++)='\\';
		default:
		  *(ptr++)=*c;
	      }
	    }
	    *(ptr++)='"';
	  }
	  *(ptr++)=0;
	}
        WERR("system(%s)\n",buffer);
        system(buffer);
	break;

      case 'd': /* dir */
	/* We ignore mkdir errors and just assume that the
	 * directory already exists
	 */
	/* fprintf(stderr,"mkdir(%s)\n",buffer); */
#ifdef __NT__
        mkdir(buffer);
#else
	mkdir(buffer, 0777);
#endif
	break;

      case 'f': /* file */
      {
        FILE *f;
        char chunk[8192];
        WERR("file(%s)\n",buffer);
        f=fopen(buffer,"wb");
        if(!f)
        {
          perror("fopen");
          fprintf(stderr,"Failed to open %s\n",buffer);
          exit(1);
        }
        len=read_int();
        while(len)
        {
          unsigned int r = len > sizeof(chunk) ? sizeof(chunk) : len;
          gr(chunk, r);
          if(fwrite(chunk, 1, r, f) != r)
          {
            perror("fwrite");
            exit(1);
          }
          len-=r;
        }
        if(fclose(f)<0)
        {
          perror("fclose");
          exit(1);
        }
      }
      break;

      case 'D':
        WERR("unlink(%s)\n",buffer);
        if(unlink(buffer)<0 && rmdir(buffer)<0)
	{
#ifdef DEBUG
	  fprintf(stderr,"Failed to delete %s\n",buffer);
	  perror("unlink");
#endif
/*	  exit(1); */
	}
	break;

      default:
	fprintf(stderr,"Wrong type (%c (%d))!\n",type,type);
	exit(1);
    }
    free(buffer);
  }
}

int main(int argc, char **argv)
{
#ifdef __NT__
  DWORD capacity = 256;

  _fmode = _O_BINARY;
  for (;;) {
    WCHAR *path = (WCHAR *)alloc_buffer(capacity * sizeof(WCHAR));
    DWORD length = GetModuleFileNameW(NULL, path, capacity);

    if (length && length < capacity) {
      int fd = open_archive(path);

      free(path);
      my_uncompress(fd, argc - 1, argv + 1);
      break;
    }
    free(path);
    if (!length || capacity > ((DWORD)-1 / sizeof(WCHAR)) / 2) break;
    capacity *= 2;
  }
#else
  const char *path = getenv("PATH");
  const char *file = argv[0];
  size_t file_length = strlen(file);
  const char *pos;

  if (!path || strchr(file, SLASH)) {
    my_uncompress(open(file, O_RDONLY), argc - 1, argv + 1);
    perror("open");
    exit(1);
  }

  for (pos = path;;) {
    const char *end = FEND(pos, DELIM);
    size_t prefix_length = end - pos;

    if (prefix_length) {
      char *candidate;
      size_t separator = !ISSLASH(end[-1]);
      int fd;

      if (file_length > (size_t)-1 - prefix_length - separator - 1)
        break;
      candidate = alloc_buffer(prefix_length + separator + file_length + 1);
      memcpy(candidate, pos, prefix_length);
      if (separator) candidate[prefix_length] = SLASH;
      memcpy(candidate + prefix_length + separator, file, file_length + 1);
      fd = open(candidate, O_RDONLY);
      free(candidate);
      my_uncompress(fd, argc - 1, argv + 1);
    }
    if (!*end) break;
    pos = end + 1;
  }
#endif
  puts("Failed to find self!");
  return 9;
}
