/* #define AAP_DEBUG 1 */
#define CACHE_HTABLE_SIZE 40951

struct res
{
  struct pike_string *protocol;

  int header_start;
  int method_len;
  int body_start;

  char *url;
  int url_len;

  char *host;
  int host_len;

  char   *content;
  int content_len;
  
  char *leftovers;
  int leftovers_len;

  char *data;
  int data_len;
};

struct cache_entry
{
  struct cache_entry *next;
  struct pike_string *data;
  time_t stale_at;
  char *url;  int url_len;
  char *host; int host_len;
  int refs;
};

struct file_ret
{
  int fd;
  off_t size;
  time_t mtime;
};

struct pstring 
{
  int len;
  char *str;
};


#define FS_STATS

struct filesystem
{
  struct pstring base;
#ifdef FS_STATS /* These can be wrong, but should give an indication.. */
  unsigned int lookups;
  unsigned int hits;
  unsigned int notfile;
  unsigned int noperm;
#endif
};

struct cache
{
  MUTEX_T mutex;
  struct cache *next;
  struct cache_entry *htable[CACHE_HTABLE_SIZE];
  unsigned long long size, entries, max_size;
  unsigned long long hits, misses, stale;
  unsigned int num_requests, sent_data, received_data;
  int gone;
};

struct args 
{
  int fd;
  struct args *next;
  struct res res;
  int timeout;

  struct svalue cb;
  struct svalue args;
  struct sockaddr_in from;
  struct cache *cache;
  struct filesystem *filesystem;
  struct log *log;
};

struct log_entry 
{
  struct log_entry *next;
  int t;
  unsigned int sent_bytes;
  unsigned int reply;
  unsigned int received_bytes;
  struct pstring raw;
  struct pstring url;
  struct sockaddr_in from;
  struct pstring method;
  struct pike_string *protocol;
};

struct log 
{
  struct log *next;
  struct log_entry *log_head;
  struct log_entry *log_tail;
  MUTEX_T log_lock;
};


struct log_object
{
  int time;
  int reply;
  int sent_bytes;
  int received_bytes;
  struct pike_string *raw;
  struct pike_string *url;
  struct pike_string *method;
  struct pike_string *protocol;
  struct pike_string *from;
};


struct c_request_object
{
  struct args *request;
  struct mapping *done_headers;
  struct mapping *misc_variables;
  int headers_parsed;
};

#define MY_MIN(a,b) ((a)<(b)?(a):(b))

#define LOG(X,Y,Z) do { \
    if((Y)->cache) {\
      (Y)->cache->num_requests++;\
      (Y)->cache->sent_data+=(X);\
      (Y)->cache->received_data+=(Y)->res.data_len;\
    }\
    if ((Y)->log) { \
      aap_log_append((X),(Y),(Z)); \
    }\
  } while(0)

#define WRITE(X,Y,Z) aap_swrite(X,Y,Z)
#undef THIS
#define THIS ((struct c_request_object *)fp->current_storage)
#define LTHIS ((struct args *)fp->current_storage)

void aap_handle_connection(struct args *arg);
