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
  for(;;) {
    string s = in->read(512);
    if(s == "")
      break;
    if(sizeof(s) != 512) {
      werror("READ ERROR on input\n");
      exit(1);
    }
    if(s-"\0" == "") {
      out->write(s);
      continue;
    }
    array a =
      array_sscanf(s, "%100s%8s%8s%8s%12s%12s%8s%c%100s%8s%32s%32s%8s%8s");
    int csum, size;
    sscanf(a[4], "%o", size);
    sscanf(a[6], "%o", csum);
    s=s[..147]+"        "+s[156..];
    if(`+(@values(s[..511])) != csum) {
      werror("CHECKSUM ERROR on input!\n");
      exit(1);
    }
    a[1] = sprintf("%6o \0", array_sscanf(a[1], "%o")[0] & 0x3f1ff);
    a[2] = "     0 \0";
    a[3] = "     0 \0";
    if((a[9]/"\0")[0]-" " == "ustar") {
      a[10] = "root\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
      a[11] = "root\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
    }
    a[6] = "        ";
    s = sprintf("%100s%8s%8s%8s%12s%12s%8s%c%100s%8s%32s%32s%8s%8s", @a)+
      s[345..];
    out->write(s[..147]+sprintf("%07o\0", `+(@values(s[..511])))+s[156..]);
    copydata(in, out, size);
    if(size & 511)
      copydata(in, out, 512-(size & 511));
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
