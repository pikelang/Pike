#ifndef LIST_H
#define LIST_H

#include "las.h"

struct list
{
  INT32 refs;
  struct list *next,*prev;
  struct array *ind;
};

#define free_list(L) do{ struct list *l_=(L); if(!--l_->refs) really_free_list(l_); }while(0)

/* Prototypes begin here */
int list_member(struct list *l, struct svalue *ind);
void really_free_list(struct list *l);
struct list *mklist(struct array *ind);
void list_insert(struct list *l,
		 struct svalue *ind);
struct array *list_indices(struct list *l);
void list_delete(struct list *l,struct svalue *ind);
void check_list_for_destruct(struct list *l);
struct list *copy_list(struct list *tmp);
struct list *merge_lists(struct list *a,
			  struct list *b,
			  INT32 operator);
struct list *add_lists(struct svalue *argp,INT32 args);
int list_equal_p(struct list *a, struct list *b, struct processing *p);
void describe_list(struct list *l,struct processing *p,int indent);
node * make_node_from_list(struct list *l);
void f_aggregate_list(INT32 args);
struct list *copy_list_recursively(struct list *l,
				   struct processing *p);
/* Prototypes end here */

#endif
