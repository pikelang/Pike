
#pike __REAL_VERSION__

inherit _Image.Dims;

//! In Pike 7.6 a string argument to get meant to load a file with
//! that name.
array get(string|Stdio.File f) {
  if(stringp(f)) f =Stdio.File(f, "r");
  return Image.Dims.get(f);
}
