#pike 7.1

class File
{
  inherit Stdio.File: F;
  array(int) stat() { return (array) F::stat(); }
}

class FILE
{
  inherit Stdio.FILE : F;
  
  array(int) stat() { return (array) F::stat(); }
}
