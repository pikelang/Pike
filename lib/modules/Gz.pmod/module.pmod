#pike __REAL_VERSION__
#require constant(_Gz)

inherit _Gz;

string check_header(Stdio.Stream f, string|void buf) {
  int magic1, magic2, method, flags, len;

  if(!buf) buf="";
  string buf_read(int n)
  {
    if(sizeof(buf)<n)
      buf += f->read(min(n-sizeof(buf), 10));
    string r = buf[..n-1];
    buf = buf[n..];
    return r;
  };

  if(sscanf(buf_read(4), "%1c%1c%1c%1c", magic1, magic2, method, flags)!=4 ||
     magic1 != 0x1f || magic2 != 0x8b)
    return 0;

  if(method != 8 || (flags & 0xe0))
    return 0;

  if(sizeof(buf_read(6)) != 6)
    return 0;

  if(flags & 4)
    if(sscanf(buf_read(2), "%-2c", len) != 1 ||
       sizeof(buf_read(len)) != len)
      return 0;

  if(flags & 8)
    loop: for(;;)
      switch(buf_read(1)) {
      case "": return 0;
      case "\0": break loop;
      }

  if(flags & 16)
    loop: for(;;)
      switch(buf_read(1)) {
      case "": return 0;
      case "\0": break loop;
      }

  if(flags & 2)
    if(sizeof(buf_read(2)) != 2)
      return 0;

  return buf;
}

int make_header(Stdio.Stream|Stdio.Buffer f) {
  constant fmt     = "%1c%1c%1c%1c%4c%1c%1c";
  constant fmtargs = ({0x1f, 0x8b, 8, 0, 0, 0, 3});
  if (f->sprintf) {
    f->sprintf(fmt, @fmtargs);
    return 1;
  }
  return f->write(sprintf(fmt, @fmtargs)) == 10;
}
