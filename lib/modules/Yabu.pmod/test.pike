#!/usr/local/bin/pike
// Yabu test program

#define ERR(msg) throw(({ msg+"\n", backtrace() }));

void check_db(object db, mapping m)
{
  for(int i = 0; i < 10; i++) {
    string s = (string)(i%3);
    object t = db[s];
    for(int j = 0; j < 100; j++) {
      string q = (string)(j%43);
      if(t[q] != m[s][q])
	ERR("Table diff #10!");
    }
  }
}

int main(int argc, array argv)
{
  if(argc == 3 && argv[1] == "dbck") {
    string filename = argv[2];
    object db = .module.db(filename, "r");
    
    werror("Database data: ");
    foreach(indices(db), string table) {
      object table = db[table];
      foreach(indices(table), string row)
	werror("%O", table[row]);
    }

    werror("\nThe database %O seems to be intact.\n", filename);
    return 0;
  }
  
  .module.db("test.db", "wct")->purge();
  object db = .module.db("test.db", "wct");
  object table = db["Aces"];

  // Test transactions.
  table["Blixt"] = "Gordon";
  object transaction = table->transaction();

  if(!equal(indices(table), ({ "Blixt" })))
    ERR("Table diff #1!");
  if(!equal(indices(transaction), ({ "Blixt" })))
    ERR("Table diff #2!");
  
  transaction["Buck"] = "Rogers";
  
  if(!equal(indices(table), ({ "Blixt" })))
    ERR("Table diff #3!");
  if(!equal(sort(indices(transaction)), ({ "Blixt", "Buck" })))
    ERR("Table diff #4!");
  if(table["Buck"] != 0)
    ERR("Table diff #5!");
  if(transaction["Buck"] != "Rogers")
    ERR("Table diff #6!");
  if(table["Blixt"] != "Gordon")
    ERR("Table diff #7!");
  if(transaction["Blixt"] != "Gordon")
    ERR("Table diff #8!");
  
  // Test multiple commands.
  mapping m = ([]);
  for(int i = 0; i < 10; i++) {
    string s = (string)(i%3);
    m[s] = m[s] || ([]);
    object t = db[s];
    if((i%3 == 0))
      table->reorganize(1.0);
    for(int j = 0; j < 100; j++) {
      string q = (string)(j%43);
      m[s][q] += 1;
      t[q] = t[q]+1;
    }
    if(i == 7)
      t->sync();
  }

  transaction->commit();
  if(table["Buck"] != "Rogers")
    ERR("Table diff #9!");

  if(!catch {
    object db2 = .module.db("test.db", "wct");
  })
    ERR("Table lock error!");

  check_db(db, m);
  destruct(db);
  check_db(db = .module.db("test.db", "w"), m);
  db->reorganize(1.0);
  destruct(db);
  check_db(db = .module.db("test.db", "wct"), m);
  
  // Remove test database.
  db->purge();

  return 0;
}
