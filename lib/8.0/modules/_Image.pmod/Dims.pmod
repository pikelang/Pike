#pike 8.1

// This code is never reached, as the `[] in Image module doesn't know
// about compat resolver.

inherit Image.Dims : pre;

// As a compatibility measure, if the @[file] is a path to an image
// file, it will be loaded and processed once the processing of the
// path as data has failed.
array(int) get(string|Stdio.File file)
{
  array ret = pre::get(file);
  if(ret) return ret;
  if(objectp(file) || sizeof(file)>1000) return 0;
  [int min, int max] = String.range(file);
  if( min<32 || max>127 ) return 0;
  catch { file = Stdio.File(file); };
  if(!file) return 0;
  return pre::get(file);
}
