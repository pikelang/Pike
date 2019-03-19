/*
 * $Id$
 *
 * Coverity modeling file.
 *
 * Upload to https://scan.coverity.com/projects/1452?tab=analysis_settings .
 *
 * Notes:
 *   * This file may not include any header files, so any derived
 *     or symbolic types must be defined here.
 *
 *   * Full structs and unions are only needed if the fields are accessed.
 *
 *   * Uninitialized variables have any value.
 *
 * Henrik Grubbström 2015-04-21.
 */

/*
 * Typedefs and constants.
 */

#define NULL	0
#define INT32	int

typedef unsigned int socklen_t;

/*
 * STDC
 */

/*
 * POSIX
 */

/* This function doesn't have a model by default. */

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
  int may_fail;
  char first, last;
  __coverity_negative_sink__(size);
  if (!src)
    return NULL;
  if (may_fail)
    return NULL;
  dst[0] = first;
  dst[size-1] = first;
  __coverity_writeall__(dst);
  return dst;
}

/*
 * Pike
 */

/*
 * Modules
 */

/*
 * Image
 */

#define COLORTYPE unsigned char

typedef struct rgb_group_struct
{
   COLORTYPE r,g,b;
/*   COLORTYPE __padding_dont_use__;*/
} rgb_group;

/* Due to use of function pointers, Coverity doesn't see the assignments
 * in these colortable.c functions.
 */

#define COLORTABLE_NCTLU_CONVERT(NAME, NCTLU_DESTINATION)		\
  void NAME(rgb_group *s, NCTLU_DESTINATION *d, int n,			\
	    struct neo_colortable *nct, struct nct_dither *dith,	\
	    int rowlen)							\
  {									\
    NCTLU_DESTINATION first, last;					\
    d[0] = first;							\
    d[n-1] = last;							\
    __coverity_writeall__(d);						\
  }

COLORTABLE_NCTLU_CONVERT(_img_nct_map_to_flat_cubicles, rgb_group);
COLORTABLE_NCTLU_CONVERT(_img_nct_map_to_flat_full, rgb_group);
COLORTABLE_NCTLU_CONVERT(_img_nct_map_to_cube, rgb_group);
COLORTABLE_NCTLU_CONVERT(_img_nct_map_to_flat_rigid, rgb_group);

int image_colortable_map_image(struct neo_colortable *nct,
			       rgb_group *s,
			       rgb_group *d,
			       int len,
			       int rowlen)
{
  rgb_group first, last;
  int maybe;
  if (maybe) return 0;
  d[0] = first;
  d[len-1] = last;
  __coverity_writeall__(d);
  return 1;
}

COLORTABLE_NCTLU_CONVERT(_img_nct_index_8bit_flat_cubicles, unsigned char);
COLORTABLE_NCTLU_CONVERT(_img_nct_index_8bit_flat_full, unsigned char);
COLORTABLE_NCTLU_CONVERT(_img_nct_index_8bit_cube, unsigned char);
COLORTABLE_NCTLU_CONVERT(_img_nct_index_8bit_flat_rigid, unsigned char);

int image_colortable_index_8bit_image(struct neo_colortable *nct,
				      rgb_group *s,
				      unsigned char *d,
				      int len,
				      int rowlen)
{
  unsigned char first, last;
  int maybe;
  if (maybe) return 0;
  d[0] = first;
  d[len-1] = last;
  __coverity_writeall__(d);
  return 1;
}

COLORTABLE_NCTLU_CONVERT(_img_nct_index_16bit_flat_cubicles, unsigned short);
COLORTABLE_NCTLU_CONVERT(_img_nct_index_16bit_flat_full, unsigned short);
COLORTABLE_NCTLU_CONVERT(_img_nct_index_16bit_cube, unsigned short);
COLORTABLE_NCTLU_CONVERT(_img_nct_index_16bit_flat_rigid, unsigned short);

int image_colortable_index_16bit_image(struct neo_colortable *nct,
				       rgb_group *s,
				       unsigned short *d,
				       int len,
				       int rowlen)
{
  unsigned short first, last;
  int maybe;
  if (maybe) return 0;
  d[0] = first;
  d[len-1] = last;
  __coverity_writeall__(d);
  return 1;
}

COLORTABLE_NCTLU_CONVERT(_img_nct_index_32bit_flat_cubicles, unsigned INT32);
COLORTABLE_NCTLU_CONVERT(_img_nct_index_32bit_flat_full, unsigned INT32);
COLORTABLE_NCTLU_CONVERT(_img_nct_index_32bit_cube, unsigned INT32);
COLORTABLE_NCTLU_CONVERT(_img_nct_index_32bit_flat_rigid, unsigned INT32);

int image_colortable_index_32bit_image(struct neo_colortable *nct,
				       rgb_group *s,
				       unsigned INT32 *d,
				       int len,
				       int rowlen)
{
  unsigned INT32 first, last;
  int maybe;
  if (maybe) return 0;
  d[0] = first;
  d[len-1] = last;
  __coverity_writeall__(d);
  return 1;
}
