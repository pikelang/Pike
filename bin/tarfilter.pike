#! /usr/bin/env pike

//
// Filters a tar file applying root/root ownership.
//

void copydata(Stdio.File in, Stdio.File out, int size)
{
  while(size>0) {
    string s = in->read((size>8192? 8192 : size));
    if(s == "") {
      werror("READ ERROR on input\n");
      exit(1);
    }
    out->write(s);
    size -= sizeof(s);
  }
}

void doit(Stdio.File in, Stdio.File out)
{
  int written;
  for(;;) {
    string s = in->read(512);
    if(s == "")
      break;
    if(sizeof(s) != 512) {
      werror("READ ERROR on input\n");
      exit(1);
    }
    if(s == "\0"*512) {
      written += out->write(s);
      continue;
    }
    array a =
      array_sscanf(s, "%100s%8s%8s%8s%12s%12s%8s%c%100s%8s%32s%32s%8s%8s");
    int csum, size, perm;
    sscanf(a[1], "%o", perm);
    sscanf(a[4], "%o", size);
    sscanf(a[6], "%o", csum);
    s=s[..147]+"        "+s[156..];
    if(`+(@values(s[..511])) != csum) {
      werror("CHECKSUM ERROR on input!\n");
      exit(1);
    }
    /* Normalize the permission flags. */
    perm &= 0x3f1ff;
    if (perm & 0444) perm |= 0444;	/* Propagate r. */
    if (perm & 0111) perm |= 0111;	/* Propagate x. */
    a[1] = sprintf("%6o \0", perm);
    a[2] = "     0 \0";
    a[3] = "     0 \0";
    if((a[9]/"\0")[0]-" " == "ustar") {
      a[10] = "root\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
      a[11] = "root\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
    }
    a[6] = "        ";
    s = sprintf("%100s%8s%8s%8s%12s%12s%8s%c%100s%8s%32s%32s%8s%8s", @a)+
      s[345..];
    written +=
      out->write(s[..147]+sprintf("%07o\0", `+(@values(s[..511])))+s[156..]);
    size = (size + 511) & -512;
    copydata(in, out, size);
    written += size;
  }
  // GNU tar 1.14 complains if we don't pad to an even 20 of blocks.
  if (written % 10240) {
    out->write("\0" * (10240 - (written % 10240)));
  }
}

int main(int argc, array(string) argv)
{
  Stdio.File s = Stdio.File("stdin");
  Stdio.File d = Stdio.File("stdout");
  
  if(argc == 3)
  {
    s = Stdio.File(argv[1], "r");
    d = Stdio.File(argv[2]+".tmp", "cwt");
  }
  
  doit(s, d);

  if(argc == 3)
  {
    d->close();
    s->close();
    
    if(!mv(argv[2]+".tmp", argv[2]))
    {
      werror("FATAL! mv(%O, %O) failed.", argv[2], argv[2]);
      exit(1);
    }
  }
  
  return 0;
}
