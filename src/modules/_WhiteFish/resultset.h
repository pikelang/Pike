typedef struct result_set
{
  INT32 num_docs;
  struct {
    INT32 doc_id;
    INT32 ranking;
  } hits[1];
} ResultSet;

#define RS_HITS(o) (((struct result_set_p*)o->storage)->d.hits)
#define RS_NUM_DOCS(o) (((struct result_set_p*)o->storage)->d.num_docs)

void init_resultset_program(void);
struct object *wf_resultset_new( );
void wf_resultset_add( struct object *o, int document, int weight );
void wf_resultset_free( struct object *o );

