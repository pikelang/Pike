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
