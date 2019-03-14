

void main(int num, array args) {

  if(num<2 || args[1]=="--help")
    exit(0, #"Creates upper/lower case tests for testsuite.in.
Takes case_info.h as argument.
");

  string s = Stdio.read_file(args[1]);
  array ci = ({});
  int off, ch;
  string type, sign;
  while( sscanf(s, "%*s{ 0x%x, %s, %sx%x, },%s", off, type, sign, ch, s)==6) {
    if(sign=="-0")
      ch = -ch;
    ci += ({ ({ off, type, ch }) });
  }

  array a = ({});
  array b = ({});
  array B = ({});
  foreach(ci; int pos; array r) {
    a += ({ r[0] });
    switch(r[1]) {
    case "CIM_NONE":
      b += ({ r[0] });
      B += ({ r[0] });
      break;
    case "CIM_LOWERDELTA":
      b += ({ r[0] });
      B += ({ r[0]-r[2] });
      break;
    case "CIM_UPPERDELTA":
      b += ({ r[0]+r[2] });
      B += ({ r[0] });
      break;
    case "CIM_CASEBIT":
      b += ({ r[0]|r[2] });
      B += ({ r[0]&~r[2] });
      break;
    case "CIM_CASEBITOFF":
      b += ({ ((r[0]-r[2])|r[2])+r[2] });
      B += ({ ((r[0]-r[2])&~r[2])+r[2] });
      break;
    default:
      exit(1, "Error in case_info.h: Unknown type %s\n", r[1]);
    }
  }

  // Insert hard coded rules
  a = a[0..0] + ({ 0x41, 0x5b, 0x61, 0x7b }) + a[1..];
  b = b[0..0] + ({ 0x61, 0x5b, 0x61, 0x7b }) + b[1..];
  B = B[0..0] + ({ 0x41, 0x5b, 0x41, 0x7b }) + B[1..];

  make_test("lower", a, b);
  write("\n\n\n\n");
  make_test("upper", a, B);
}

void make_test(string f, array a, array b) {
  write(#"test_equal(%s_case((string) ({
// These characters correspond to the cases in case_info.h
// Please update this and the corresponding %s_case table
// when UnicodeData.txt is changed.
// Part 1: 0x0000 - 0x0FFF
", f, (["upper":"lower","lower":"upper"])[f] );

  int pos;
  while(a[pos]<0x0FFF) {
    write("0x%04x, ", a[pos++]);
    if(!(pos%8)) write("\n");
  }

  write("})), (string) ({\n");

  for(int i; i<pos; i++) {
    write("0x%04x, ", b[i]);
    if(!((i+1)%8)) write("\n");
  }

  write("}))\n");

  write(#"test_equal(%s_case((string) ({
// These characters correspond to the cases in case_info.h
// Please update this and the corresponding %s_case table
// when UnicodeData.txt is changed.
// Part 2: 0x1000 -
", f, (["upper":"lower","lower":"upper"])[f] );

  for(int i=pos; i<sizeof(a); i++) {
    write("0x%04x, ", a[i]);
    if(!((i+1)%8)) write("\n");
  }

  write("})), (string) ({\n");

  for(int i=pos; i<sizeof(b); i++) {
    write("0x%04x, ", b[i]);
    if(!((i+1)%8)) write("\n");
  }

  write("}))\n");

}
