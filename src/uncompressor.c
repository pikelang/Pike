#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef __NT__
#include <unistd.h>
#include <string.h>
#else
#include <windows.h>
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


gzFile gz;

static void gr(char *ptr, int len)
{
/*  fprintf(stderr,"Reading %d\n",len); */
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
  gr(x,4);
/*   fprintf(stderr,"Got 0x%02x%02x%02x%02x\n",x[0],x[1],x[2],x[3]); */
  return (x[0] << 24) | (x[1]<<16) | (x[2]<<8) | x[3];
}

#if !( SEEK_TO - 0 )
#define SEEK_TO -1
#endif

#if !( SEEK_SET - 0 )
#define SEEK_SET 0
#endif


void my_uncompress(char *file)
{
  unsigned int len;
  char buffer[8192];
  int fd;
  char *d;

  fd=open(file, O_RDONLY);
  if(fd == -1) return;

  if(lseek(fd, SEEK_TO, SEEK_SET)<0)
  {
    perror("lseek");
/*    fprintf(stderr,"SEEK_TO=%d\n",SEEK_TO); */
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
/*  fprintf(stderr,"Changing dir to %s\n",d); */

  while(1)
  {
    char type;
    int len;
    char *data;
    GR(type);

    len=read_int();
/*    fprintf(stderr,"namelen=%d\n",len); */
    
    gr(buffer, len);
    buffer[len]=0;
    

/*    fprintf(stderr,"Type %c: %s\n",type,buffer); */

   switch(type)
    {
      case 'q': /* quit */
	exit(0);

      case 'w': /* write */
	puts(buffer);
	break;

      case 'e': /* setenv */
/*	fprintf(stderr,"putenv(%s)\n",buffer); */
	putenv(buffer);
	break;
	
      case 's': /* execute */
	/* We ignore error so that we can continue with
	 * file deletion
	 */
	/* FIXME:
	 * Add support for concatenating all the parameters
	 * to this command
	 */
/*	fprintf(stderr,"system(%s)\n",buffer); */
	system(buffer);
	break;
	
      case 'd': /* dir */
	/* We ignore mkdir errors and just assume that the
	 * directory already exists
	 */
	/* fprintf(stderr,"mkdir(%s)\n",buffer); */
	mkdir(buffer, 0777);
	break;
	
      case 'f': /* file */
      {
	FILE *f;
	/* fprintf(stderr,"file(%s)\n",buffer); */
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
	  int r=len > sizeof(buffer) ? sizeof(buffer) : len;
	  gr(buffer, r);
	  if(fwrite(buffer, 1, r, f) != r)
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
/*	fprintf(stderr,"unlink(%s)\n",buffer); */
	if(unlink(buffer)<0 && rmdir(buffer)<0)
	{
	  perror("unlink");
/*	  exit(1); */
	}
	break;
	
      default:
	fprintf(stderr,"Wrong type (%c (%d))!\n",type,type);
	exit(1);
    }
  }
}

int main(int argc, char **argv)
{
  char tmp[16384];
  char *file;

#ifdef __NT__
  _fmode=_O_BINARY;
  if(SearchPath(0,
	     argv[0],
	     ".exe",
	     sizeof(tmp),
	     tmp,
	     &file))
    my_uncompress(tmp);
#else
  char *path=getenv("PATH");
  char *pos;
  file=argv[0];
  if(!path || strchr(file, SLASH))
  {
    my_uncompress(file);
    perror("open");
    exit(1);
  }

  pos=path;
  while(1)
  {
    char *next;
    next=FEND(pos,DELIM);
    if(next!=pos)
    {
      char *ptr=tmp;
      struct stat buf;

      memcpy(ptr=tmp,pos,next-pos);
      ptr+=next-pos;
      if(*ptr != '/'
#ifdef __NT
	 && *ptr != SLASH
#endif
	 )
	*(ptr++)='/';

      memcpy(ptr,
	     file,
	     strlen(file));
      ptr+=strlen(file);
      *ptr=0;

      my_uncompress(tmp);
    }
    if(!*next) break;
    pos=next+1;
  }
#endif
  puts("Failed to find self!");
  exit(9);
}
