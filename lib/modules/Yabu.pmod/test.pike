// Yabu test program

#define ERR(msg) throw(({ msg+"\n", backtrace() }));

int main(int argc, array argv)
{
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

  transaction->commit();

  if(table["Buck"] != "Rogers")
    ERR("Table diff #9!");

  // Test multiple commands.
  mapping m = ([]);
  for(int i = 0; i < 10; i++) {
    string s = (string)(i%3);
    m[s] = m[s] || ([]);
    object t = db[s];
    for(int j = 0; j < 100; j++) {
      string q = (string)(j%43);
      m[s][q] += 1;
      t[q] = t[q]+1;
    }
    t->sync();
  }

  for(int i = 0; i < 10; i++) {
    string s = (string)(i%3);
    object t = db[s];
    for(int j = 0; j < 100; j++) {
      string q = (string)(j%43);
      if(t[q] != m[s][q])
	ERR("Table diff #10!");
    }
  }

  // Remove test database.
  db->purge();

  return 0;
}
