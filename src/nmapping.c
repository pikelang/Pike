#include "svalue.h"
#include "gjalloc.h"

static struct block_allocator keypair_allocator
    = BA_INIT(sizeof(struct ht_keypair), 1024);

ATTRIBUTE((malloc))
static INLINE struct ht_keypair * ht_allocate() {
    struct ht_keypair * k = (struct ht_keypair *)ba_alloc(&keypair_allocator);
}

static INLINE void ht_free(struct ht_keypair * k) {
    free_svalue(k->key); 
    free_svalue(k->value); 
    ba_free(&keypair_allocator, k);
}

INLINE void ht_reset_iterator(struct hash_iterator * it) {
    // detect grow/shrink. on grow, hash_mask needs to be 
    // not so easy
}

/* 
 * Chains can be frozen. If they are, namely when an iterator is walking
 * through them, or during calls to code that can reenter, elements are
 * not deleted but rather flagged as deleted. Furthermore, chains are not
 * split until the last iterator leaves them (its the one that does it).
 */

static INLINE void ht_freeze_bucket(struct ht_bucket * b) {
    ++b->freeze_count;
}

static INLINE void ht_bucket_insert(struct ht_bucket * b,
				    struct ht_keypair * k) {
    k->next = b->u.k;
    b->u.k = k;
}

static INLINE void ht_unfreeze_bucket(struct hash_table * h,
				      const uint32_t n) {
    const struct ht_bucket b = h->table[n];
    // TODO: flag the buckets which a delete happened on
    // and check if it needs to be extended
    if (!(--b.freeze_count & ~HT_FLAG_MASK) && b.freeze_count & HT_FLAG_MASK) {
	struct ht_keypair ** k = &(b.k);
	struct ht_keypair * tmp;
	while (*k) {
	    if ((*k)->deleted) {
		tmp = *k;
		*k = (*k)->next;
		ht_free(tmp);
		continue;
	    }
	    if (*k->hval & h->hash_mask != n) {
		/* we have to split this one off */
		tmp = *k;
		*k = &((*k)->next);
		ht_bucket_insert(h->table + (*k->hval & h->hash_mask),
				 tmp);
		continue;
	    }
	    k = &((*k)->next);
	}

	/* possibly split the bucket */
    }
}

INLINE void ht_iterate_bucket(struct hash_iterator * it) {
    if (it->current) {
	it->current = it->current->next;
    } else {
	it->current = ht_get_bucket(it->h, it->n);
    }

    if (it->hash_mask > it->h->hash_mask) {
	/* the table has been shrunken, so there might be extra entries, that
	 * do not belong to the bucket we are walking currently */
	while (it->current && it->n != (it->current->hval & it->hash_mask)) {
	    it->current = it->current->next;
	}
    } else if (it->hash_mask > it->h->hash_mask) {
	/* the table has been grown, so we have to continue to the next
	 * bucket until we hit the table size limit */
	it->n += it->hash_mask + 1;
	if (it->n > it->h->hash_mask) {
	    it->current = NULL;
	}
    }
}

struct ht_keypair * ht_iterate(struct hash_iterator * it, uint32_t steps) {
    if (it->current) {
	if (it->generation != it->h->generation)
	    ht_reset_iterator(it);
redo:
	ht_iterate_bucket(it);
	if (!it->current) {
	    size_t n = it->n;
	    for (; n <= it->h->hash_mask;) {
	    }
	    do {
		if (++it->n > it->hash_mask) return NULL; /* done */
		it->current = ht_get_bucket(it->h, it->n);
	    } while(!it->current);
	    goto redo;
	}

	if (--steps) goto redo;
    }
    return it->current;
}

static INLINE struct ht_bucket * ht_get_bucket(const struct hash_table * h,
					       const uint32_t hval) {
    const struct ht_bucket * b = h->table[hval & h->hash_mask];

    if (b->freeze_count & HT_FLAG_REL) {
	return b->u.b;
    }
    return b;
}

void ht_init_iterator(struct hash_iterator * it, const struct hash_table * h) {
    uint32_t i;
    it->h = h;
    it->generation = h->generation;
    it->hash_mask = h->hash_mask;
    it->current = NULL;
    for (i = 0; i <= it->hash_mask; i++) {
	if (h->table[i].u.k) {
	    it->current = h->table[i].u.k;
	    it->n = i;
	    break;
	}
    }
}

/* the returned pointer is only ok to use until the next mapping operation */
static INLINE struct svalue * ht_lookup(const struct hash_table * h,
					const struct svalue * key) {
    const uint32_t hval = HT_HASH(key);
    struct svalue * ret = NULL;
    int frozen = 0;
    struct ht_bucket * b = ht_get_bucket(h, hval);
    struct ht_keypair * pair = b->u.k;

    for (;pair && (pair->hval < hval); pair = pair->next) {}

    for (;pair && (pair->hval == hval); pair = pair->next) {

	if (HT_IS_NEQ(pair->key, *key) || pair->deleted) continue;

	if (HT_IS_IDENTICAL(pair->key, *key)) {
	    ret = &(pair->value);
	    break;
	}

	if (!frozen) {
	    frozen = 1;
	    ht_freeze_bucket(b);
	}
	

	if (HT_IS_EQ(pair->key, *key)) {
	    ret = &(pair->value);
	}
    }

    if (frozen) ht_unfreeze_bucket(b);

    return ret;
}

static INLINE void ht_split_bucket(struct ht_bucket * src,
				   struct ht_bucket * dst) {
    /* iterators present in the bucket, so we cannot split it, yet */
    if (src->freeze_count & HT_FLAG_REL) {
	*dst = *src;
    } else if (src->freeze_count & ~HT_FLAG_MASK) {
	src->freeze_count |= HT_FLAG_GROW;
	dst->freeze_count = HT_FLAG_REL;
	dst->u.b = src;
    } else {
	uint32_t mask = h->hash_mask + 1;
	struct ht_keypair ** k = &(src->u.k);
	struct ht_keypair * tmp;
	// do the actual split
	while (*k) {
	    /* belongs into the next bucket */
	    if ((*k)->hval & mask) {
		tmp = *k;
		*k = (*k)->next;
		ht_bucket_insert(dst, tmp);
		continue;
	    }
	    k = &((*k)->next);
	}
    }
}

static INLINE void ht_merge_bucket(struct ht_bucket * one,
				   struct ht_bucket * two) {
    struct ht_keypair ** b;
    if (two->freeze_count & HT_FLAG_REL) {
	one->freeze_count ^= HT_FLAG_GROW;
	return;
    }
    one->freeze_count += two->freeze_count & (~HT_FLAG_MASK);

    if (!two->u.k) return;
    // walk to the end of the chain
    b = &(one->u.k);

    while (*b) b = &((*b)->next);
    *b = two->u.k;
}

static void ht_grow(struct hash_table * h) {
    struct ht_bucket ** tmp;
    uint32_t i, end = h->hash_mask + 1;
    tmp = (struct ht_bucket**)
	    realloc(h->table, (end*2)*sizeof(struct ht_bucket));
    if (!tmp) {
	// fail
    }
    memset(tmp+end, 0, end * sizeof(struct ht_bucket));
    h->table = tmp;
    // increase generation counter, since grows need to be noticed
    // by iterators
    for (i = 0; i < end; i++) {
	ht_split_bucket(h->table[i], h->table[end+i]);	
    }

    h->generation++;
    h->hash_mask = (end << 1) - 1;
}

static void ht_shrink(struct hash_table * h) {
    struct ht_bucket ** tmp;

    uint32_t i, end = (h->hash_mask >> 1) + 1;

    for (i = 0; i < end; i++) {
	ht_merge_bucket(h->table[i], h->table[i+end]);
    }

    tmp = (struct ht_bucket**)
	    realloc(h->table, end * sizeof(struct ht_bucket));
    if (!tmp) {
	// do something
    }
    h->table = tmp;
    h->hash_mask = end-1;
}

/* TODO 
 * we can keep them sorted by hval for free, since we have to walk the chain
 * anyhow. we can simply keep the slot we had to insert into
 *
 * this makes grow much more efficient, since we can split the chain at a
 * predefined spot.
 *
 */
PMOD_EXPORT void ht_insert(struct hash_table * h,
			   const struct svalue * key,
			   const struct svalue * val) {
    const uint32_t hval = HT_HASH(key);
    const struct ht_bucket * b = ht_get_bucket(h, hval);
    struct ht_keypair ** t = &(b->u.k);
    struct ht_keypair ** t = &(b->u.k);
    int frozen = 0;
    /* first look if the key is alreay in */
    
    while (*t && (*t)->hval < hval) {
	t = &((*t)->next);
    }

    dst = t;

    while (*t && (*t)->hval == hval) {
	const struct ht_keypair * k = *t;

	if (HT_IS_NEQ(*key, k->key)) continue;

	if (HT_IS_IDENTICAL(*key, k->key)) {
	    dst = t;
	    goto insert_into;
	}
	// do expensive check
	if (!frozen) {
	    frozen = 1;
	    ht_freeze_bucket(b);
	}
	if (HT_IS_EQ(*key, k->key)) {
	    dst = t;
	    goto insert_into;
	}
    }

    if (1) {
	/* the bucket might have changed ? */
	//b = ht_get_bucket(h, hval);
	h->size++;
	k = ht_allocate();
	k->hval = hval;
	assign_svalue_no_free(&(k->val), val);
	assign_svalue_no_free(&(k->key), key);
	k->next = *dst;
	*dst = k;
    } else {
insert_into:
	assign_svalue(&(k->val), val);
	assign_svalue(&(k->key), key);
	if (!k->deleted) h->size++;
    }

    k->deleted = 0;

    if (frozen) {
	ht_unfreeze_bucket(b);
    }
}

PMOD_EXPORT void ht_delete(struct hash_table * h,
			   const struct svalue * key) {
}
