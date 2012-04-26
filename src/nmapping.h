#include "bitvector.h"
#include "svalue.h"
#include <stdint.h>

#ifndef HT_INITIAL_MAGNITUDE
# define HT_INITIAL_MAGNITUDE	4
#endif

#ifndef HT_HASH
# define HT_HASH(key)	hash_svalue(key)
#endif

// FIXME: this is not good!!!
#ifndef HT_EMPTY_PAIR(key)
# define HT_EMPTY_PAIR(p)   (p.key.type == 0 && p.key.subtype == 0 && p.key.u.ptr == NULL)
#endif

#ifndef HT_KEY_EQUAL(key)
# define HT_KEY_EQUAL(a, b) is_eq(a, b)
#endif

#define HT_LOOP3(h, label, C...) do {
    struct hash_iterator __it;
    struct ht_keypair keypair;
    ht_init_iterator(&__it, h);
    while (__it.current) {
	char c = 0;
	keypair = *(__it.current);
	do { if (c++) goto SURVIVE ## label; C; goto SURVIVE ## label; } while(0);
	goto END ## label;
	SURVIVE ## label:
	ht_iterate(&__it, 1);
    }
    END ## label:
} while(0)

#define HT_LOOP2(h, l, C...) HT_LOOP3(h, l, C)
#define HT_LOOP(h, C...)    HT_LOOP2(h, XCONCAT(ht_loop_label, __LINE__), C)

#define HT_LOOP_RO3(h, label, C...) do {
    const struct hash_table * __h = (h);
    uint32_t i;
    struct ht_keypair * keypair;

    for (i = 0; i < __h->hash_mask; i++) {
	keypair = __h->table[i];
	while (keypair) {
	    char c = 0;
	    do { if (c++) goto SURVIVE ## label; C; goto SURVIVE ## label; } while(0);
	    goto END ## label;
	    SURVIVE ## label:
	    keypair = keypair->next;
	}
    }
    END ## label:
} while(0)

#define HT_LOOP_RO2(h, l, C...) HT_LOOP3(h, l, C)
#define HT_LOOP_RO(h, C...)	HT_LOOP2(h, XCONCAT(ht_loop_label, __LINE__), C)

struct ht_keypair {
    struct ht_keypair * next;
    uint32_t hval;
    uint32_t deleted;
    struct svalue key;
    struct svalue val;
}

#define HT_FLAG_DEL	(uint32_t)(1 << 31)
#define HT_FLAG_GROW	(uint32_t)(1 << 30)
#define HT_FLAG_REL	(uint32_t)(1 << 29)
#define HT_FLAG_MASK	(HT_FLAG_DEL|HT_FLAG_GROW|HT_FLAG_GROW)

struct ht_bucket {
    /* if HT_FLAG_REL is set, this bucket is a redirect to an old one */
    union {
	struct ht_keypair * k;
	struct ht_bucket * b;
    } u;
    uint32_t freeze_count; 
}

struct hash_table {
    uint32_t hash_mask;
    uint32_t size;
    size_t generation;
    struct ht_bucket * table[1]; 
}

struct hash_iterator {
    struct ht_keypair * current;
    struct hash_table * h;
    size_t generation;
    uint32_t hash_mask;
    uint32_t n;
}
