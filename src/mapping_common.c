
void gc_mark_all_mappings(void)
{
  gc_mark_mapping_pos = gc_internal_mapping;
  while (gc_mark_mapping_pos) {
    struct mapping *m = gc_mark_mapping_pos;
    gc_mark_mapping_pos = m->next;

    debug_malloc_touch(m);
    debug_malloc_touch(m->data);

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
  struct mapping_data *md;
  size_t unreferenced = 0;

  for(m=gc_internal_mapping;m;m=next)
  {
    debug_malloc_touch(m);
    debug_malloc_touch(m->data);

    if(gc_do_free(m))
    {
      /* Got an extra ref from gc_cycle_pop(). */
      md = m->data;

      debug_malloc_touch(m);
      debug_malloc_touch(md);

      /* Protect against unlink_mapping_data() recursing too far. */
      m->data=&empty_data;
      add_ref(m->data);

      unlink_mapping_data(md);
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

#ifdef PIKE_DEBUG
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

