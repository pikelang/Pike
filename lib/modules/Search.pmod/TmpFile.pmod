private string tmp_unique = sprintf("%d.%d", time(), getpid());
private int tmp_sequence = 0;   // This variable must be process global.
constant contenttypes = ({ });

string tmp_filename()
{
  return sprintf("../var/tmp/search.tmp.%s.%d", tmp_unique, tmp_sequence++);
}
