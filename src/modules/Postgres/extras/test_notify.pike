#include <postgres.h>
/* $Id: test_notify.pike,v 1.2 1998/08/25 13:27:19 grubba Exp $
 *
 * This simple program connects to the template1 database "template1"
 * (it comes default with postgres95) and sits there waiting for
 * someone to issue a notify sql command on a table named "prova".
 * When this happens, it prints a message and exits.
 * To test it, do this:
 * $ psql
 * => create table prova (bah int)\g
 * [other shell] $ pike test_notify.pike
 * => notify prova\g
 * =>\q
 * If it worked you should have read 'notification: "prova"' as output
 * from the script.
 */

void notify_cb(string s) {
	write("notification: \""+s+"\"\n");
//	exit(0);
}

int main () {
	object o,r;
	array row;
	mixed err;
	int pid;
	o=Sql.postgres("","template1");
	write ("Setting notify callback\n");
	o->set_notify_callback(notify_cb,2);
	write ("Trying to trigger the notify callback.\n");
	o->big_query("LISTEN prova");
	return -1;
}
