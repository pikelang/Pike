# Test cases for embedded Perl in Pike.
$a = "";
for(1..9) { $a .= $_;}
exit $a eq "123456789";
