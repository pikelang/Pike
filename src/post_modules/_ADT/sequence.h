/*
 * $Id$
 */

void pike_init_Sequence_module(void);
void pike_exit_Sequence_module(void);

#define FIX_AND_CHECK_INDEX(INDEX, SIZE, EXTRA)				\
  do {									\
    INT_TYPE _orig_index_ = (INDEX);					\
    ptrdiff_t _size_ = (SIZE);						\
    if((INDEX)<0) (INDEX)+=_size_;					\
    if((INDEX)<0 || (INDEX)>=_size_+(EXTRA)) {				\
      if (_size_) {							\
	Pike_error("Index %"PRINTPIKEINT"d is out of array range "	\
		   "%"PRINTPTRDIFFT"d - %"PRINTPTRDIFFT"d.\n",		\
		   _orig_index_,					\
		   -_size_,						\
		   _size_+(EXTRA)-1);					\
      } else {								\
	Pike_error("Attempt to index the empty array with %"PRINTPIKEINT"d.\n",	\
		   _orig_index_);					\
      }									\
    }									\
  } while (0)
