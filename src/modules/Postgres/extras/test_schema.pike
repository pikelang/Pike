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
