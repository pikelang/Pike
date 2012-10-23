#include "las.h"
#include "opcodes.h"

void gc_mark_all_mappings(void)
{
  gc_mark_mapping_pos = gc_internal_mapping;
  while (gc_mark_mapping_pos) {
    struct mapping *m = gc_mark_mapping_pos;
    gc_mark_mapping_pos = m->next;

    debug_malloc_touch(m);
#ifdef OLD_MAPPING
    debug_malloc_touch(m->data);
#endif

    if(gc_is_referenced(m))
      gc_mark_mapping_as_referenced(m);
  }
}

void gc_cycle_check_all_mappings(void)
{
  struct mapping *m;
  for (m = gc_internal_mapping; m; m = m->next) {
    real_gc_cycle_check_mapping(m, 0);
    gc_cycle_run_queue();
  }
}

void gc_zap_ext_weak_refs_in_mappings(void)
{
  gc_mark_mapping_pos = first_mapping;
  while (gc_mark_mapping_pos != gc_internal_mapping && gc_ext_weak_refs) {
    struct mapping *m = gc_mark_mapping_pos;
    gc_mark_mapping_pos = m->next;
    gc_mark_mapping_as_referenced(m);
  }
  gc_mark_discard_queue();
}

size_t gc_free_all_unreferenced_mappings(void)
{
  struct mapping *m,*next;
#ifdef OLD_MAPPING
  struct mapping_data *md;
#endif
  size_t unreferenced = 0;

  for(m=gc_internal_mapping;m;m=next)
  {
    debug_malloc_touch(m);
#ifdef OLD_MAPPING
    debug_malloc_touch(m->data);
#endif

    if(gc_do_free(m))
    {
      /* Got an extra ref from gc_cycle_pop(). */
#ifdef OLD_MAPPING
      md = m->data;
#endif

      debug_malloc_touch(m);
#ifdef OLD_MAPPING
      debug_malloc_touch(md);
#endif

#ifdef OLD_MAPPING
      /* Protect against unlink_mapping_data() recursing too far. */
      m->data=&empty_data;
      add_ref(m->data);
      unlink_mapping_data(md);
#endif

#ifdef MAPPING_SIZE_DEBUG
      m->debug_size=0;
#endif
      gc_free_extra_ref(m);
      SET_NEXT_AND_FREE(m, free_mapping);
    }
    else
    {
      next=m->next;
    }
    unreferenced++;
  }

  return unreferenced;
}

PMOD_EXPORT struct svalue *low_mapping_string_lookup(struct mapping *m,
                                                     struct pike_string *p)
{
  struct svalue tmp;
  SET_SVAL(tmp, T_STRING, 0, string, p);
  return low_mapping_lookup(m, &tmp);
}

PMOD_EXPORT void mapping_string_insert(struct mapping *m,
                                       struct pike_string *p,
                                       const struct svalue *val)
{
  struct svalue tmp;
  SET_SVAL(tmp, T_STRING, 0, string, p);
  mapping_insert(m, &tmp, val);
}

PMOD_EXPORT void mapping_string_insert_string(struct mapping *m,
				  struct pike_string *p,
				  struct pike_string *val)
{
  struct svalue tmp;
  SET_SVAL(tmp, T_STRING, 0, string, val);
  mapping_string_insert(m, p, &tmp);
}

PMOD_EXPORT struct svalue *simple_mapping_string_lookup(struct mapping *m,
							const char *p)
{
  struct pike_string *tmp;
  if((tmp=findstring(p)))
    return low_mapping_string_lookup(m,tmp);
  return 0;
}

/* Lookup in a mapping of mappings */
PMOD_EXPORT struct svalue *mapping_mapping_lookup(struct mapping *m,
						  const struct svalue *key1,
						  const struct svalue *key2,
						  int create)
{
  struct svalue tmp;
  struct mapping *m2;
  struct svalue *s=low_mapping_lookup(m, key1);
  debug_malloc_touch(m);

#if defined(PIKE_DEBUG) && defined(OLD_MAPPING)
  if(m->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
#endif

  if(!s || TYPEOF(*s) != T_MAPPING)
  {
    if(!create) return 0;
    SET_SVAL(tmp, T_MAPPING, 0, mapping, allocate_mapping(5));
    mapping_insert(m, key1, &tmp);
    debug_malloc_touch(m);
    debug_malloc_touch(tmp.u.mapping);
    free_mapping(tmp.u.mapping); /* There is one ref in 'm' */
    s=&tmp;
  }

  m2=s->u.mapping;
  debug_malloc_touch(m2);
  s=low_mapping_lookup(m2, key2);
  if(s) return s;
  if(!create) return 0;

  SET_SVAL(tmp, T_INT, NUMBER_UNDEFINED, integer, 0);

  mapping_insert(m2, key2, &tmp);
  debug_malloc_touch(m2);

  return low_mapping_lookup(m2, key2);
}


PMOD_EXPORT struct svalue *mapping_mapping_string_lookup(struct mapping *m,
				      struct pike_string *key1,
				      struct pike_string *key2,
				      int create)
{
  struct svalue k1,k2;
  SET_SVAL(k1, T_STRING, 0, string, key1);
  SET_SVAL(k2, T_STRING, 0, string, key2);
  return mapping_mapping_lookup(m,&k1,&k2,create);
}

PMOD_EXPORT void mapping_index_no_free(struct svalue *dest,
			   struct mapping *m,
			   const struct svalue *key)
{
  struct svalue *p;

  if(!IS_DESTRUCTED (key) && (p=low_mapping_lookup(m,key)))
  {
    assign_svalue_no_free(dest, p);

    /* Never return NUMBER_UNDEFINED for existing entries. */
    /* Note: There is code that counts on storing UNDEFINED in mapping
     * values (using e.g. low_mapping_lookup to get them), so we have
     * to fix the subtype here rather than in mapping_insert. */
    if(TYPEOF(*p) == T_INT)
      SET_SVAL_SUBTYPE(*dest, NUMBER_NUMBER);
  }else{
    SET_SVAL(*dest, T_INT, NUMBER_UNDEFINED, integer, 0);
  }
}

void describe_mapping(struct mapping *m,struct processing *p,int indent)
{
  struct processing doing;
  struct array *a;
  JMP_BUF catch;
  ONERROR err;
  INT32 e,d;
  char buf[40];

#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
#endif

  if(! m->data->size)
  {
    my_strcat("([ ])");
    return;
  }

  doing.next=p;
  doing.pointer_a=(void *)m;
  for(e=0;p;e++,p=p->next)
  {
    if(p->pointer_a == (void *)m)
    {
      sprintf(buf,"@%ld",(long)e);
      my_strcat(buf);
      return;
    }
  }

  if (Pike_in_gc > GC_PASS_PREPARE && Pike_in_gc < GC_PASS_FREE) {
    /* Have to do without any temporary allocations. */
    struct keypair *k;
    int notfirst = 0;

    if (m->data->size == 1) {
      my_strcat("([ /* 1 element */\n");
    } else {
      sprintf(buf, "([ /* %ld elements */\n", (long)m->data->size);
      my_strcat(buf);
    }

    NEW_MAPPING_LOOP(m->data) {
      if (notfirst) my_strcat(",\n");
      else notfirst = 1;
      for(d = 0; d < indent; d++)
	my_putchar(' ');
      describe_svalue(&keypair_ind(k), indent+2, &doing);
      my_strcat (": ");
      describe_svalue(&keypair_val(k), indent+2, &doing);
    }

    my_putchar('\n');
    for(e=2; e<indent; e++) my_putchar(' ');
    my_strcat("])");
    return;
  }

  a = mapping_indices(m);
  SET_ONERROR(err, do_free_array, a);

  if(! m->data->size) {		/* mapping_indices may remove elements */
    my_strcat("([ ])");
  }
  else {
    int save_t_flag = Pike_interpreter.trace_level;
    dynamic_buffer save_buf;

    if (m->data->size == 1) {
      my_strcat("([ /* 1 element */\n");
    } else {
      sprintf(buf, "([ /* %ld elements */\n", (long)m->data->size);
      my_strcat(buf);
    }

    save_buffer (&save_buf);
    Pike_interpreter.trace_level = 0;
    if(SETJMP(catch)) {
      free_svalue(&throw_value);
      mark_free_svalue (&throw_value);
    }
    else
      sort_array_destructively(a);
    UNSETJMP(catch);
    Pike_interpreter.trace_level = save_t_flag;
    restore_buffer (&save_buf);

    for(e = 0; e < a->size; e++)
    {
      struct svalue *tmp;
      if(e)
	my_strcat(",\n");
    
      for(d = 0; d < indent; d++)
	my_putchar(' ');

      describe_svalue(ITEM(a)+e, indent+2, &doing);
      my_strcat (": ");

      {
	int save_t_flag=Pike_interpreter.trace_level;
	Pike_interpreter.trace_level=0;
	
	tmp=low_mapping_lookup(m, ITEM(a)+e);
	
	Pike_interpreter.trace_level=save_t_flag;
      }
      if(tmp)
	describe_svalue(tmp, indent+2, &doing);
      else
	my_strcat("** gone **");
    }

    my_putchar('\n');
    for(e=2; e<indent; e++) my_putchar(' ');
    my_strcat("])");
  }

  UNSET_ONERROR(err);
  free_array(a);
}

node *make_node_from_mapping(struct mapping *m)
{
#ifdef PIKE_DEBUG
  if(m->data->refs <=0)
    Pike_fatal("Zero refs in mapping->data\n");
#endif

  mapping_fix_type_field(m);

  if(!mapping_is_constant(m,0))
  {
    struct array *ind, *val;
    node *n;
    ind=mapping_indices(m);
    val=mapping_values(m);
    n=mkefuncallnode("mkmapping",
		     mknode(F_ARG_LIST,
			    make_node_from_array(ind),
			    make_node_from_array(val)));
    free_array(ind);
    free_array(val);
    return n;
  }else{
    struct svalue s;

    if(!m->data->size)
      return mkefuncallnode("aggregate_mapping",0);

    SET_SVAL(s, T_MAPPING, 0, mapping, m);
    return mkconstantsvaluenode(&s);
  }
}

#ifdef PIKE_DEBUG

void simple_describe_mapping(struct mapping *m)
{
  dynamic_buffer save_buf;
  char *s;
  init_buf(&save_buf);
  describe_mapping(m,0,2);
  s=simple_free_buf(&save_buf);
  fprintf(stderr,"%s\n",s);
  free(s);
}
#endif
