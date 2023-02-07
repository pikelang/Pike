h
/#/s/#\(.*\)/\/*\1 *\//p
g
/#/d
g
s/.*/#ifdef &/
p
g
s/.*/add_integer_constant("&", &, 0);/
p
g
s/.*/#endif \/* & *\//
p
g
/^[^E]/d
s/.*/#ifdef WSA&/
p
g
s/.*/add_integer_constant("WSA&", WSA&, 0);/
p
g
s/.*/#endif \/* WSA& *\//
p
