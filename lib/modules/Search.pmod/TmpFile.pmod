private string tmp_unique = sprintf("%d.%d", time(), getpid());
private int tmp_sequence = 0;   // This variable must be process global.
constant contenttypes = ({ });

string tmp_filename()
{
  mkdir("../var/search/");
  return sprintf("../var/search/search.tmp.%s.%d", tmp_unique, tmp_sequence++);
}
