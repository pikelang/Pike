#pike 7.7

inherit spider;

constant XML = Parser._parser.XML.Simple;

#define DEF(X) constant X = Parser._parser.XML.##X
DEF(isbasechar);
DEF(iscombiningchar);
DEF(isdigit);
DEF(isextender);
DEF(isfirstnamechar);
DEF(ishexchar);
DEF(isidographic);
DEF(isletter);
DEF(isnamechar);
DEF(isspace);
