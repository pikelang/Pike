function(:int(0..0)) `[](string print_what)
{
   if (print_what=="") print_what="bipp\n";
   else if (print_what[-1]!="\n") print_what+="\n";
   return lambda() { werror(print_what); return 0; };
}

int(0..0) `()()
{
   werror("blipp\n");
   return 0;
}
