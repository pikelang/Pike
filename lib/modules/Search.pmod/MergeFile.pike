private Stdio.File fd;

//!
void create(Stdio.File _fd)
{
  fd = _fd;
}

static void write_blob(String.Buffer buf, string word, string blob,
		       void|string blob2)
{
  //  werror("%O\n", word);
  buf->add(sprintf("%4c%s%4c",
		   sizeof(word), word,
		   sizeof(blob) + sizeof(blob2||"")),
	   blob);
  if(blob2)
    buf->add(blob2);
}

//!
void write_blobs(_WhiteFish.Blobs blobs)
{
  String.Buffer buf = String.Buffer();
  foreach(blobs->read_all_sorted(), array(string) pair)
  {
    string word = string_to_utf8(pair[0]);
    write_blob(buf, word, pair[1]);

    if(sizeof(buf) > 1<<18)
      fd->write(buf->get());
  }
  if(sizeof(buf))
    fd->write(buf->get());
}

array get_next_word_blob()
{
  int length;
  if(!sscanf(fd->read(4), "%4c", length))
    return 0;
  string word = fd->read(length);
  sscanf(fd->read(4), "%4c", length);
  string blob = fd->read(length);
  return ({ word, blob });  
}

//!
void merge_mergefiles(Search.MergeFile a, Search.MergeFile b)
{
  String.Buffer buf = String.Buffer();
  
  int done = 0;
  int a_used=1, b_used=1;
  array blob_a, blob_b;

  while(!done)
  {
    if( a_used )
      blob_a = a->get_next_word_blob();
    if( b_used )
      blob_b = b->get_next_word_blob();
    a_used = b_used = 0;
    if( !blob_b && !blob_a)
    {
      done = 1;
    }
    else if( !blob_a )
    {
      write_blob( buf, blob_b[0], blob_b[1] );
      b_used = 1;
    }
    else if( !blob_b )
    {
      write_blob( buf, blob_a[0], blob_a[1] );
      a_used = 1;
    }
    else if( blob_a[0] == blob_b[0] )
    {
      write_blob( buf, blob_a[0], blob_a[1], blob_b[1] );
      a_used = b_used = 1;
    }
    else if( blob_a[0] < blob_b[0] )
    {
      write_blob( buf, blob_a[0], blob_a[1] );
      a_used = 1;
    }
    else if( blob_b[0] < blob_a[0] )
    {
      write_blob( buf, blob_b[0], blob_b[1] );
      b_used = 1;
    }
    if(sizeof(buf) > 1<<18)
      fd->write(buf->get());
  }
  if(sizeof(buf))
    fd->write(buf->get());
}

void test()
{
  werror("Testing MergeFile...");
  fd->seek(0);
  string last_word;
  while(array a = get_next_word_blob())
  {
    if(last_word && a[0]<last_word)
      werror("Error in blob: %O < %O\n", a[0], last_word);
    last_word = a[0];
  }
  werror("  Done.\n");
}

void close()
{
  fd->close();
}
