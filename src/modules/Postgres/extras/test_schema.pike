// $Id: test_schema.pike,v 1.2 1998/03/28 14:38:49 grubba Exp $

//#include "postgres.h"
#include <sql.h>

int main () {
	object o,r;
	array row;
	mixed err;
	int pid;
	o=Sql.postgres("","template1");
	write(sprintf("****** Databases: ******\n%O\n",o->list_dbs()));
	write(sprintf("****** Tables: ******\n%O\n",o->list_tables()));
	write(sprintf("****** Attributes: ******\n%O\n",o->list_fields("prova")));
}
