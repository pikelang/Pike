#!../../pike
#include <msql.h>

int main()
{
	array result;
	string table;
	mixed err;

	object db;
	
	db=msql();
	werror (sprintf ("error :'%O'\n",db->error()));

	result=db->list_dbs();
	write(sprintf("Databases: %O\n",result));
	write ("connecting to database "+result[0]+"\n");
	db->select_db(result[0]);
	result = db->list_tables();
	write(sprintf("Tables: %O\n",result));
	table=result[0];
	write("*** Dumping fields of table: "+table+"\n");
	catch {result=db->list_fields(table);};
	write(sprintf("%O\n",result));
	write ("*** Dumping contents of table:"+table+"\n");
	write(sprintf("%O\n",db->query("","")));
	err = catch (write(sprintf("%O\n",db->query("Select * from "+table))));
	if (err)
		write (db->error());
	write(sprintf("%O\n",db->error()));
	write(db->server_info()+"\n");
	write(db->host_info()+"\n");
}
