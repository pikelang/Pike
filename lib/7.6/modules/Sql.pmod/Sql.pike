/*
 * $Id: Sql.pike,v 1.1 2004/06/13 21:47:21 mast Exp $
 *
 * Glue for the old broken Mysql-module which fetched all rows at once always
 */

#pike 7.7

inherit Sql.Sql;

static program find_dbm(string program_name) {
  program p = master()->get_compilation_handler(7,6)
   ->resolv("Sql."+program_name);
  if(!p && Sql->Provider && Sql->Provider[program_name])
    p = Sql->Provider[program_name][program_name];
  return p;
}
