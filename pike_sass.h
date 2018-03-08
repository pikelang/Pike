#ifndef PIKE_SASS_H
#define PIKE_SASS_H


#ifdef PIKE_SASS_DEBUG
# define SASS_TRACE(X...) printf ("# " X)
#else
# define SASS_TRACE(X...)
#endif


#define MY_THROW_ERR(Y...)              \
  do {                                  \
    Pike_error (Y);                     \
  } while (0)


#endif /* PIKE_SASS_H */
