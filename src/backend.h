#ifndef BACKEND_H
#define BACKEND_H

extern time_t current_time;
typedef void (*callback)(int,void *);

/* Prototypes begin here */
struct selectors;
void init_backend();
void set_read_callback(int fd,callback cb,void *data);
void set_write_callback(int fd,callback cb,void *data);
callback query_read_callback(int fd);
callback query_write_callback(int fd);
void *query_read_callback_data(int fd);
void *query_write_callback_data(int fd);
void do_debug(int check_refs);
void backend();
int write_to_stderr(char *a, INT32 len);
void check_signals();
/* Prototypes end here */

#endif
