#ifndef FILE_H
#define FILE_H

struct file
{
  int fd;
  struct svalue id;
  struct svalue read_callback;
  struct svalue write_callback;
  struct svalue close_callback;
  int errno;
};

/* Prototypes begin here */
struct object *file_make_object_from_fd(int fd, int mode);
void file_setbuf(int fd, int bufcmd);
void exit_files();
void init_files_programs();
/* Prototypes end here */

#define FILE_READ 1
#define FILE_WRITE 2
#define FILE_APPEND 4
#define FILE_CREATE 8
#define FILE_TRUNC 16
#define FILE_EXCLUSIVE 32

#endif
