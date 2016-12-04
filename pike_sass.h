#ifndef PIKE_SASS_H
#define PIKE_SASS_H


#ifdef SASS_DEBUG
# define SASS_TRACE(X...) printf ("# " X)
#else
# define SASS_TRACE(X...)
#endif


#define MY_THROW_ERR(Y...)              \
  do {                                  \
    my_opts_delete (THIS->options);     \
    Pike_error (Y);                     \
  } while (0)


#define OPTS_INC_ARRAY_BLK_SIZE 4

typedef struct my_opts_array
{
  char **items;
  int    len;
} my_opts_array;


typedef struct my_opts
{
  int             style;
  my_opts_array  *include_paths;
  long            precision;
  bool            comments;
  char           *map_path;
  bool            map_embed;
  char           *map_root;
  bool            omit_map_url;
} my_opts;


my_opts* my_opts_create ()
{
  my_opts *opts             = malloc (sizeof (my_opts));
  opts->include_paths       = NULL;
  opts->map_path            = NULL;
  opts->map_root            = NULL;
  opts->style               = SASS_STYLE_NESTED;
  opts->precision           = 5;
  opts->omit_map_url        = true;

  return opts;
}


void my_opts_push_include_path (my_opts *opts, const char *path)
{
  if (opts->include_paths == NULL) {
    int blksize = OPTS_INC_ARRAY_BLK_SIZE * sizeof (char*);
    opts->include_paths = malloc (sizeof (my_opts_array));
    opts->include_paths->len = 0;
    opts->include_paths->items = malloc (blksize);
  }
  else {
    if (opts->include_paths->len % OPTS_INC_ARRAY_BLK_SIZE == 0) {
      int blksize = OPTS_INC_ARRAY_BLK_SIZE * sizeof (char*);
      int size = blksize * opts->include_paths->len;
      opts->include_paths->items =
        realloc (opts->include_paths->items, size + blksize);
    }
  }

  int pos = opts->include_paths->len;
  opts->include_paths->items[pos] = malloc (strlen (path) + 1);
  strcpy (opts->include_paths->items[pos], path);

  opts->include_paths->len += 1;
}


void my_opts_set_include_path (my_opts *opts, const char *path)
{
  if (path != NULL) {
    SASS_TRACE ("::: my_opts_set_include_path(%s)\n", path);

    if (opts->include_paths == NULL) {
      my_opts_push_include_path (opts, path);
    }
    else {
      opts->include_paths->items[0] = strdup (path);
    }
  }
}


void my_opts_include_paths_delete (my_opts *opts)
{
  SASS_TRACE ("----- my_opts_include_paths_delete -----\n");

  if (opts->include_paths) {
    for (int i = 0; i < opts->include_paths->len; i++) {
      free (opts->include_paths->items[i]);
    }

    free (opts->include_paths->items);
  }

  free (opts->include_paths);
  opts->include_paths = NULL;
}


void my_opts_delete (my_opts *opts)
{
  SASS_TRACE ("----- my_opts_delete -----\n");
  if (opts->include_paths) {
    my_opts_include_paths_delete (opts);
  }

  if (opts->map_path) {
    free (opts->map_path);
  }

  if (opts->map_root) {
    free (opts->map_root);
  }

  free (opts);
  SASS_TRACE ("----- /my_opts_delete -----\n");
}

#endif /* PIKE_SASS_H */
