typedef struct result_set
{
  INT32 num_docs;
  struct hits {
    INT32 doc_id;
    INT32 ranking;
  } hits[1];
} ResultSet;

#define RS_HITS(o) (((struct result_set_p*)o->storage)->d->hits)
#define RS_NUM_DOCS(o) (((struct result_set_p*)o->storage)->d->num_docs)

struct object *wf_resultset_new( );

void init_resultset_program(void);
void exit_resultset_program(void);


void wf_resultset_add  ( struct object *o, int document, int weight );
void wf_resultset_clear( struct object *o );
void wf_resultset_free ( struct object *o );

#ifdef DEBUG
extern struct program resultset_program;
struct object *wf_not_resultset( struct object *o );
# define WF_RESULTSET(X) ((X&&find_storage(X,resultset_program))?X:wf_not_resultset(X))
#else
# define WF_RESULTSET(X) X
#endif
