array(int) a() {
  int got_error = 0;
  array(string) destruct_order;
  add_constant ("destructing", lambda (string id) {destruct_order += ({id});});
  add_constant ("my_error", lambda (string s, mixed... args) {
			      if (!got_error) werror ("\n");
			      werror (s, @args);
			      got_error++;
			    });
  program Dead = compile_string (#"
      string id;
      void create (int i) {id = sprintf (\"dead[%d]\", i);}
      mixed a = 1, b = 1; // Mustn't be zero at destruct time.
      mixed x, y, z;
      array v = set_weak_flag (({1}), 1); // Mustn't be zero at destruct time.
      array w = set_weak_flag (({0, 0}), 1);
      function(object:mixed) checkfn;
      void check_live (mapping(object:int) checked) {
	//werror (\"check_live %s\\n\", id);
	checked[this_object()] = 1;
	if (!a) my_error (id + \"->a got destructed too early.\\n\");
	else if (!b) my_error (id + \"->b got destructed too early.\\n\");
	else if (!v[0]) my_error (id + \"->v[0] got destructed too early.\\n\");
	else if (functionp (checkfn) && !checkfn (this_object()))
	  my_error (id + \"->checkfn failed.\\n\");
	else {
	  if (objectp (a) && !checked[a]) a->check_live (checked);
	  if (objectp (b) && !checked[b]) b->check_live (checked);
	  if (objectp (x) && !checked[x]) x->check_live (checked);
	  if (objectp (y) && !checked[y]) y->check_live (checked);
	  if (objectp (z) && !checked[z]) z->check_live (checked);
	  if (objectp (v[0]) && !checked[v[0] ]) v[0]->check_live (checked);
	  if (objectp (w[0]) && !checked[w[0] ]) w[0]->check_live (checked);
	  if (objectp (w[1]) && !checked[w[1] ]) w[1]->check_live (checked);
	}
	//werror (\"check_live %s done\\n\", id);
      }
    ");
  add_constant ("Dead", Dead);
  program Live = compile_string (#"
      inherit Dead;
      void create (int i) {id = sprintf (\"live[%d]\", i);}
      void destroy() {
	destructing (id);
	//werror (\"destroy %s\\n\", id);
	check_live (([]));
      }
    ");
  add_constant ("Live", Live);
  program LiveNested = compile_string (#"
      inherit Live;
      string id = \"live_nested[0]\";
      void create() {}
      void check_live_0 (mapping(object:int) checked) {check_live (checked);}
      class LiveNested1
      {
        inherit Live;
	string id = \"live_nested[1]\";
	void create() {}
	void check_live (mapping(object:int) checked) {
	  checked[this_object()] = 1;
	  if (catch (check_live_0 (checked)))
	    my_error (\"Parent for %s got destructed too early.\\n\", id);
	  else ::check_live (checked);
	}
        void check_live_1 (mapping(object:int) checked) {check_live (checked);}
	class LiveNested2
	{
	  inherit Live;
	  string id = \"live_nested[2]\";
	  void create() {}
	  void check_live (mapping(object:int) checked) {
	    checked[this_object()] = 1;
	    if (catch (check_live_1 (checked)))
	      my_error (\"Parent for %s got destructed too early.\\n\", id);
	    else ::check_live (checked);
	  }
	}
	class LiveNested3
	{
	  inherit Live;
	  string id = \"live_nested[3]\";
	  void create() {}
	  void check_live (mapping(object:int) checked) {
	    checked[this_object()] = 1;
	    if (catch (check_live_1 (checked)))
	      my_error (\"Parent for %s got destructed too early.\\n\", id);
	    else ::check_live (checked);
	  }
	}
      }
    ");
  program DeadNested = compile_string (#"
      inherit Dead;
      string id = \"dead_nested[0]\";
      void create() {}
      void check_live_0 (mapping(object:int) checked) {check_live (checked);}
      class DeadNested1
      {
        inherit Dead;
	string id = \"dead_nested[1]\";
	void create() {}
	void check_live (mapping(object:int) checked) {
	  checked[this_object()] = 1;
	  if (catch (check_live_0 (checked)))
	    my_error (\"Parent for %s got destructed too early.\\n\", id);
	  else ::check_live (checked);
	}
        void check_live_1 (mapping(object:int) checked) {check_live (checked);}
	class DeadNested2
	{
	  inherit Dead;
	  string id = \"dead_nested[2]\";
	  void create() {}
	  void check_live (mapping(object:int) checked) {
	    checked[this_object()] = 1;
	    if (catch (check_live_1 (checked)))
	      my_error (\"Parent for %s got destructed too early.\\n\", id);
	    else ::check_live (checked);
	  }
	}
	class DeadNested3
	{
	  inherit Dead;
	  string id = \"dead_nested[3]\";
	  void create() {}
	  void check_live (mapping(object:int) checked) {
	    checked[this_object()] = 1;
	    if (catch (check_live_1 (checked)))
	      my_error (\"Parent for %s got destructed too early.\\n\", id);
	    else ::check_live (checked);
	  }
	}
      }
    ");
  add_constant ("destructing");
  add_constant ("my_error");
  add_constant ("Dead");
  add_constant ("Live");

  array(object) live, dead, live_nested, dead_nested;
  array(array) destruct_order_tests = ({
    ({3,			// Wanted number of live objects.
      0,			// Wanted number of dead objects.
      0,			// Wanted live nested objects.
      0,			// Wanted dead nested objects.
      lambda() {		// Function to connect them.
	live[0]->x = live[1], live[0]->a = live[2];
	live[1]->x = live[0];
      }}),
    ({2, 2, 0, 0, lambda() {	// 1
		    live[0]->x = live[1], live[0]->a = dead[0];
		    live[1]->x = live[0];
		    dead[0]->a = dead[1];
		    dead[1]->a = dead[0];
		  }}),
    ({1, 2, 0, 0, lambda() {	// 2
		    live[0]->a = live[0], live[0]->b = dead[0];
		    dead[0]->a = dead[1];
		    dead[1]->a = dead[0];
		  }}),
    ({0, 3, 0, 0, lambda() {	// 3
		    dead[0]->a = dead[1];
		    dead[1]->a = dead[0];
		    dead[2]->a = dead[0], dead[2]->b = dead[2];
		  }}),
    ({3, 0, 0, 0, lambda() {	// 4
		    live[0]->a = live[0], live[0]->b = live[1];
		    live[1]->a = live[2];
		  }}),
    ({1, 2, 0, 0, lambda() {	// 5
		    live[0]->a = live[0], live[0]->b = dead[0];
		    dead[0]->a = dead[1];
		  }}),
    ({1, 2, 0, 0, lambda() {	// 6
		    live[0]->a = live[0], live[0]->b = dead[1];
		    dead[0]->a = dead[0], dead[0]->b = dead[1];
		    dead[1]->a = dead[1];
		  }}),
    ({2, 2, 0, 0, lambda() {	// 7
		    live[0]->a = live[0], live[0]->b = live[1];
		    dead[0]->a = dead[0];
		    dead[0]->b = live[1];
		    live[1]->a = dead[1];
		    dead[1]->a = dead[1];
		  }}),
    ({1, 3, 0, 0, lambda() {	// 8
		    live[0]->a = live[0], live[0]->b = dead[2];
		    dead[0]->a = dead[0];
		    dead[0]->b = dead[2];
		    dead[2]->a = dead[1];
		    dead[1]->a = dead[1];
		  }}),
    ({3, 1, 0, 0, lambda() {	// 9
		    live[0]->a = live[0], live[0]->b = live[1];
		    dead[0]->a = dead[0], dead[0]->b = live[1];
		    live[1]->a = live[2];
		  }}),
    ({1, 3, 0, 0, lambda() {	// 10
		    live[0]->a = live[0], live[0]->b = dead[1];
		    dead[0]->a = dead[0], dead[0]->b = dead[1];
		    dead[1]->a = dead[2];
		  }}),
    ({1, 3, 0, 0, lambda() {	// 11
		    live[0]->a = live[0], live[0]->b = dead[1];
		    dead[0]->a = dead[0], dead[0]->b = dead[1];
		    dead[1]->a = dead[1], dead[1]->b = dead[2];
		    dead[2]->a = dead[2];
		  }}),
    ({5, 0, 0, 0, lambda() {	// 12
		    live[0]->x = live[1];
		    live[1]->x = live[0], live[1]->a = live[2];
		    live[2]->x = live[3];
		    live[3]->x = live[2], live[3]->a = live[4];
		    live[4]->a = live[4];
		  }}),
    ({3, 0, 0, 0, lambda() {	// 13
		    live[0]->x = live[1], live[0]->y = live[2];
		    live[1]->x = live[2];
		    live[2]->x = live[0];
		  }}),
    ({2, 0, 0, 0, lambda() {	// 14
		    live[0]->a = live[1], live[0]->b = live[0];
		    live[1]->w[0] = live[0];
		  }}),
    ({2, 0, 0, 0, lambda() {	// 15
		    live[0]->a = live[0], live[0]->b = live[1];
		    live[1]->w[0] = live[0];
		  }}),
    ({2, 0, 0, 0, lambda() {	// 16
		    live[0]->a = live[0], live[0]->w[0] = live[1];
		    live[1]->x = live[0];
		  }}),
    ({3, 0, 0, 0, lambda() {	// 17
		    live[0]->a = live[0], live[0]->b = live[1];
		    live[1]->w[0] = live[2];
		    live[2]->a = live[2], live[2]->b = live[1];
		  }}),
    ({3, 0, 0, 0, lambda() {	// 18
		    live[0]->a = live[1], live[0]->x = live[2];
		    live[1]->w[0] = live[2];
		    live[2]->x = live[0];
		  }}),
    ({4, 0, 0, 0, lambda() {	// 19
		    live[0]->x = live[0], live[0]->a = live[1], live[0]->b = live[2];
		    live[1]->a = live[3];
		    live[2]->a = live[3];
		    live[3]->w[0] = live[0];
		  }}),
    ({3, 0, 0, 0, lambda() {	// 20
		    live[0]->x = live[1];
		    live[1]->x = live[0], live[1]->a = live[2];
		    live[2]->w[0] = live[1];
		  }}),
    ({4, 0, 0, 0, lambda() {	// 21
		    live[0]->w[0] = live[1], live[0]->a = live[3];
		    live[1]->w[0] = live[2];
		    live[2]->a = live[0], live[2]->b = live[3], live[2]->x = live[2];
		    live[3]->a = live[1];
		  }}),
    ({2, 1, 0, 0, lambda() {	// 22
		    live[0]->a = dead[0], live[0]->x = live[1];
		    live[1]->x = live[0];
		    dead[0]->w[0] = live[1];
		  }}),
    ({2, 1, 0, 0, lambda() {	// 23
		    live[0]->a = live[1], live[0]->b = dead[0];
		    live[1]->w[0] = dead[0];
		    dead[0]->x = live[0];
		  }}),
    ({3, 0, 0, 0, lambda() {	// 24
		    live[0]->x = live[0], live[0]->a = live[1], live[0]->b = live[2];
		    live[1]->w[0] = live[0], live[1]->w[1] = live[2];
		    live[2]->a = live[1];
		  }}),
    ({3, 0, 0, 0, lambda() {	// 25
		    live[0]->a = live[1];
		    live[1]->w[0] = live[2];
		    live[2]->x = live[2], live[2]->a = live[0], live[2]->b = live[1];
		  }}),
    ({3, 0, 0, 0, lambda() {	// 26
		    live[0]->w[0] = live[1], live[0]->a = live[2];
		    live[1]->x = live[1], live[1]->a = live[0], live[1]->b = live[2];
		    live[2]->w[0] = live[1];
		  }}),
    ({3, 0, 0, 0, lambda() {	// 27
		    live[0]->w[0] = live[1];
		    live[1]->x = live[1], live[1]->a = live[0], live[1]->b = live[2];
		    live[2]->a = live[0];
		  }}),
    ({3, 0, 0, 0, lambda() {	// 28
		    live[0]->a = live[0], live[0]->v[0] = live[1];
		    live[1]->a = live[1], live[1]->v[0] = live[2];
		    live[2]->a = live[2];
		  }}),
    ({2, 2, 0, 0, lambda() {	// 29
		    live[0]->x = live[1], live[0]->v[0] = dead[0];
		    live[1]->x = live[0];
		    dead[0]->a = dead[1];
		    dead[1]->a = dead[0];
		  }}),
    ({4, 0, 0, 0, lambda() {	// 30
		    live[0]->a = live[1], live[0]->b = live[2], live[0]->v[0] = live[3];
		    live[1]->w[0] = live[0];
		    live[2]->a = live[3];
		    live[3]->w[0] = live[2];
		  }}),
    ({2, 1, 0, 0, lambda() {	// 31
		    live[0]->a = dead[0];
		    dead[0]->a = live[0], dead[0]->b = live[1];
		    live[1]->a = live[1];
		  }}),
    ({2, 1, 0, 0, lambda() {	// 32
		    live[0]->a = live[0], live[0]->b = dead[0];
		    live[1]->a = dead[0];
		    dead[0]->a = live[1];
		  }}),
    ({2, 1, 0, 0, lambda() {	// 33
		    dead[0]->a = live[0];
		    live[0]->a = dead[0], live[0]->b = live[1];
		    live[1]->a = live[1];
		  }}),
    ({2, 1, 0, 0, lambda() {	// 34
		    live[0]->a = dead[0];
		    dead[0]->b = live[0], dead[0]->a = live[1];
		    live[1]->a = live[1];
		  }}),
    ({2, 1, 0, 0, lambda() {	// 35
		    live[0]->b = live[0], live[0]->a = dead[0];
		    live[1]->a = dead[0];
		    dead[0]->a = live[1];
		  }}),
    ({2, 1, 0, 0, lambda() {	// 36
		    dead[0]->a = live[0];
		    live[0]->b = dead[0], live[0]->a = live[1];
		    live[1]->a = live[1];
		  }}),
    ({3, 0, 0, 0, lambda() {	// 37
		    live[0]->a = live[0], live[0]->v[0] = live[1];
		    live[0]->checkfn = lambda (object o) {
					 return o->v[0]->w[0];
				       };
		    live[1]->w[0] = live[2];
		    live[2]->a = live[1], live[2]->b = live[2];
		  }}),
    ({4, 0, 0, 0, lambda() {	// 38
		    live[0]->x = live[1];
		    live[1]->x = live[2];
		    live[2]->x = live[0], live[2]->w[0] = live[3];
		    live[3]->a = live[1], live[3]->b = live[3];
		  }}),
    ({0, 2, 2, 0, lambda() {	// 39
		    dead[0]->x = dead[0], dead[0]->a = live_nested[0];
		    dead[1]->x = dead[1], dead[1]->a = live_nested[1];
		    live_nested[0]->x = live_nested[1];
		  }}),
    ({0, 2, 2, 0, lambda() {	// 40
		    dead[0]->x = dead[0], dead[0]->a = live_nested[0];
		    dead[1]->x = dead[1], dead[1]->a = live_nested[1];
		    live_nested[0]->w[0] = live_nested[1];
		  }}),
    ({3, 0, 3, 0, lambda() {	// 41
		    live[0]->x = live[0], live[0]->a = live_nested[0];
		    live[1]->x = live[1], live[1]->a = live_nested[2];
		    live[2]->x = live[2], live[2]->a = live_nested[1];
		    live_nested[0]->x = live_nested[2];
		  }}),
    ({4, 0, 4, 0, lambda() {	// 42
		    live[0]->x = live[0], live[0]->a = live_nested[0];
		    live[1]->x = live[1], live[1]->a = live_nested[1];
		    live[2]->x = live[2], live[2]->a = live_nested[2];
		    live[3]->x = live[3], live[3]->a = live_nested[3];
		    live_nested[0]->x = live_nested[3];
		  }}),
    ({3, 0, 2, 0, lambda() {	// 43
		    live[0]->x = live[0], live[0]->a = live_nested[0];
		    live[1]->x = live[1], live[1]->a = live_nested[1];
		    live_nested[0]->a = live[2];
		    live[2]->x = live_nested[1];
		  }}),
    ({3, 0, 3, 0, lambda() {	// 44
		    live[0]->x = live[0], live[0]->a = live_nested[0];
		    live[1]->x = live[1], live[1]->a = live_nested[2];
		    live[2]->x = live[2], live[2]->a = live_nested[1];
		    live_nested[0]->x = live_nested[2];
		    live_nested[1]->a = live[0];
		  }}),
    ({0, 2, 0, 2, lambda() {	// 45
		    dead[0]->x = dead[0], dead[0]->a = dead_nested[0];
		    dead[1]->x = dead[1], dead[1]->a = dead_nested[1];
		    dead_nested[0]->x = dead_nested[1];
		  }}),
    ({0, 2, 0, 2, lambda() {	// 46
		    dead[0]->x = dead[0], dead[0]->a = dead_nested[0];
		    dead[1]->x = dead[1], dead[1]->a = dead_nested[1];
		    dead_nested[0]->w[0] = dead_nested[1];
		  }}),
    ({3, 0, 0, 3, lambda() {	// 47
		    live[0]->x = live[0], live[0]->a = dead_nested[0];
		    live[1]->x = live[1], live[1]->a = dead_nested[2];
		    live[2]->x = live[2], live[2]->a = dead_nested[1];
		    dead_nested[0]->x = dead_nested[2];
		  }}),
    ({4, 0, 0, 4, lambda() {	// 48
		    live[0]->x = live[0], live[0]->a = dead_nested[0];
		    live[1]->x = live[1], live[1]->a = dead_nested[1];
		    live[2]->x = live[2], live[2]->a = dead_nested[2];
		    live[3]->x = live[3], live[3]->a = dead_nested[3];
		    dead_nested[0]->x = dead_nested[3];
		  }}),
    ({3, 0, 0, 2, lambda() {	// 49
		    live[0]->x = live[0], live[0]->a = dead_nested[0];
		    live[1]->x = live[1], live[1]->a = dead_nested[1];
		    dead_nested[0]->a = live[2];
		    live[2]->x = dead_nested[1];
		  }}),
    ({3, 0, 0, 3, lambda() {	// 50
		    live[0]->x = live[0], live[0]->a = dead_nested[0];
		    live[1]->x = live[1], live[1]->a = dead_nested[2];
		    live[2]->x = live[2], live[2]->a = dead_nested[1];
		    dead_nested[0]->x = dead_nested[2];
		    dead_nested[1]->a = live[0];
		  }}),
    ({0, 4, 2, 2, lambda() {	// 51
		    dead[0]->x = dead[0], dead[0]->a = live_nested[0];
		    dead[1]->x = dead[1], dead[1]->a = live_nested[1];
		    dead[2]->x = dead[2], dead[2]->a = dead_nested[0];
		    dead[3]->x = dead[3], dead[3]->a = dead_nested[1];
		    live_nested[0]->x = dead_nested[1];
		    dead_nested[0]->x = live_nested[1];
		  }}),
    ({4, 0, 0, 0, lambda() {	// 52
		    live[0]->w[0] = live[1];
		    live[1]->x = live[1], live[1]->a = live[0], live[1]->b = live[2];
		    live[2]->w[0] = live[3];
		    live[3]->x = live[3], live[3]->a = live[0];
		  }}),
    ({4, 0, 0, 0, lambda() {	// 53
		    live[0]->w[0] = live[1];
		    live[1]->x = live[1], live[1]->b = live[0], live[1]->a = live[2];
		    live[2]->w[0] = live[3];
		    live[3]->x = live[3], live[3]->a = live[0];
		  }}),
    ({4, 0, 0, 0, lambda() {	// 54
		    live[0]->x = live[0], live[0]->w[0] = live[1];
		    live[1]->w[0] = live[2];
		    live[2]->x = live[2], live[2]->a = live[1], live[2]->b = live[3];
		    live[3]->x = live[3], live[3]->a = live[0];
		  }}),
    ({4, 0, 0, 0, lambda() {	// 55
		    live[0]->x = live[0], live[0]->w[0] = live[1];
		    live[1]->w[0] = live[2];
		    live[2]->x = live[2], live[2]->b = live[1], live[2]->a = live[3];
		    live[3]->x = live[3], live[3]->a = live[0];
		  }}),
    ({3, 0, 0, 0, lambda() {	// 56
		    live[0]->a = live[2];
		    live[1]->x = live[1], live[1]->a = live[0], live[1]->b = live[2];
		  }}),
    ({3, 0, 0, 0, lambda() {	// 57
		    live[0]->a = live[2];
		    live[1]->x = live[1], live[1]->b = live[0], live[1]->a = live[2];
		  }}),
    ({2, 1, 0, 0, lambda() {	// 58
		    live[0]->x = live[1], live[0]->y = dead[0];
		    live[0]->checkfn = lambda (object o) {
					 return o->y;
				       };
		    live[1]->x = live[0];
		    dead[0]->x = dead[0];
		  }}),
    ({2, 1, 0, 0, lambda() {	// 59
		    live[0]->y = live[1], live[0]->x = dead[0];
		    live[0]->checkfn = lambda (object o) {
					 return o->x;
				       };
		    live[1]->x = live[0];
		    dead[0]->x = dead[0];
		  }}),
    ({1, 2, 0, 2, lambda() {	// 60
		    live[0]->x = dead_nested[0], live[0]->y = dead_nested[0];
		    dead[0]->x = dead[0], dead[0]->y = dead_nested[0];
		    dead[1]->x = dead[1], dead[1]->y = dead_nested[1];
		    dead_nested[0]->x = live[0], dead_nested[0]->y = dead_nested[1];
		  }}),
    ({1, 2, 0, 2, lambda() {	// 61
		    live[0]->x = dead_nested[0], live[0]->y = dead_nested[0];
		    dead[0]->x = dead[0], dead[0]->y = dead_nested[0];
		    dead[1]->x = dead[1], dead[1]->y = dead_nested[1];
		    dead_nested[0]->y = live[0], dead_nested[0]->x = dead_nested[1];
		  }}),
    ({3, 0, 0, 0, lambda() {	// 62
		    live[0]->x = live[1];
		    live[1]->x = live[0], live[1]->y = live[2];
		    live[2]->x = live[1];
		  }}),
    ({3, 0, 0, 0, lambda() {	// 63
		    live[0]->x = live[1];
		    live[1]->y = live[0], live[1]->x = live[2];
		    live[2]->x = live[1];
		  }}),
    ({2, 0, 2, 0, lambda() {	// 64
		    live[0]->x = live[1], live[0]->y = live_nested[1];
		    live[1]->w[0] = live_nested[0];
		    live_nested[0]->y = live[0];
		  }}),
    ({2, 0, 2, 0, lambda() {	// 65
		    live[0]->y = live[1], live[0]->x = live_nested[1];
		    live[1]->w[0] = live_nested[0];
		    live_nested[0]->y = live[0];
		  }}),
    ({1, 1, 3, 0, lambda() {	// 66
		    dead[0]->x = dead[0], dead[0]->a = live_nested[0];
		    live_nested[0]->x = live[0], live_nested[0]->y = live_nested[2];
		    live[0]->x = live_nested[1];
		  }}),
    ({1, 1, 3, 0, lambda() {	// 67
		    dead[0]->x = dead[0], dead[0]->a = live_nested[0];
		    live_nested[0]->y = live[0], live_nested[0]->x = live_nested[2];
		    live[0]->x = live_nested[1];
		  }}),
    ({0, 1, 2, 2, lambda() {	// 68
		    dead[0]->x = dead[0], dead[0]->a = live_nested[0];
		    live_nested[0]->y = live_nested[1], live_nested[0]->x = dead_nested[0];
		    live_nested[1]->x = dead_nested[1];
		    dead_nested[0]->x = live_nested[1];
		  }}),
    ({0, 1, 2, 2, lambda() {	// 69
		    dead[0]->x = dead[0], dead[0]->a = live_nested[0];
		    live_nested[0]->x = live_nested[1], live_nested[0]->y = dead_nested[0];
		    live_nested[1]->x = dead_nested[1];
		    dead_nested[0]->x = live_nested[1];
		  }}),
    ({0, 1, 2, 2, lambda() {	// 70
		    dead[0]->x = dead[0], dead[0]->a = live_nested[0];
		    live_nested[0]->x = live_nested[1], live_nested[0]->y = dead_nested[1];
		    live_nested[1]->x = dead_nested[0];
		    dead_nested[0]->x = live_nested[0];
		  }}),
    ({0, 1, 2, 2, lambda() {	// 71
		    dead[0]->x = dead[0], dead[0]->a = live_nested[0];
		    live_nested[0]->y = live_nested[1], live_nested[0]->x = dead_nested[1];
		    live_nested[1]->x = dead_nested[0];
		    dead_nested[0]->x = live_nested[0];
		  }}),
    ({2, 0, 2, 0, lambda() {	// 72
		    live[0]->x = live[1];
		    live[1]->x = live_nested[1];
		    live_nested[1]->x = live[0];
		    live_nested[0]->x = live[1];
		  }}),
    ({2, 0, 4, 0, lambda() {	// 73
		    live[0]->x = live[1], live[0]->y = live_nested[2], live[0]->z = live_nested[3];
		    live[1]->x = live[0];
		    live_nested[1]->x = live[0];
		  }}),
    ({2, 0, 4, 0, lambda() {	// 74
		    live[0]->y = live[1], live[0]->z = live_nested[2], live[0]->x = live_nested[3];
		    live[1]->x = live[0];
		    live_nested[1]->x = live[0];
		  }}),
    ({2, 0, 4, 0, lambda() {	// 75
		    live[0]->z = live[1], live[0]->x = live_nested[2], live[0]->y = live_nested[3];
		    live[1]->x = live[0];
		    live_nested[1]->x = live[0];
		  }}),
    ({2, 1, 2, 0, lambda() {	// 76
		    dead[0]->x = dead[0], dead[0]->a = live_nested[0];
		    live_nested[0]->y = live_nested[1], live_nested[0]->x = live[1];
		    live_nested[1]->x = live[0];
		    live[0]->x = live_nested[0];
		    live[1]->x = live[0];
		  }}),
    //       ({3, 0, 0, 0, lambda() { // Not possible without weak refs directly in objects.
    // 		   live[0]->x = live[0], live[0]->v[0] = live[1];
    // 		   live[1]->x = live[1], live[1]->w[0] = live[2];
    // 		   live[2]->x = live[2], live[2]->a = live[0];
    // 		 }}),
  });

  int quiet = !_verbose;

  int tests_failed = 0;
  int tests = 0;
  for (int test = 0; test < sizeof (destruct_order_tests); test++) {
    [int nlive, int ndead, int nlnested, int ndnested, function(void:void) setup] =
      destruct_order_tests[test];
    int objs = nlive + ndead;
    array(int) idx = indices (allocate (objs));
    int n = 1;
    for (int f = nlive + ndead; f > 1; f--) n *= f;
    if(!quiet)
      werror ("GC destruct order test %d, %d permutations      \r", test, n);
    tests += n;
    while (n--) {
      array(int) alloc_order = Array.permute (idx, n);
      array(int) create_order = ({});
      live = allocate (nlive);
      dead = allocate (ndead);
      if (nlnested >= 1) {
	// Creating these before the Dead and Live objects below assumes
	// that the gc will start with the last created object first, so
	// the order can be controlled with those objects.
	live_nested = ({LiveNested()});
	if (nlnested >= 2) live_nested += ({live_nested[0]->LiveNested1()});
	if (nlnested >= 3) live_nested += ({live_nested[1]->LiveNested2()});
	if (nlnested >= 4) live_nested += ({live_nested[1]->LiveNested3()});
      }
      if (ndnested >= 1) {
	dead_nested = ({DeadNested()});
	if (ndnested >= 2) dead_nested += ({dead_nested[0]->DeadNested1()});
	if (ndnested >= 3) dead_nested += ({dead_nested[1]->DeadNested2()});
	if (ndnested >= 4) dead_nested += ({dead_nested[1]->DeadNested3()});
      }
      for (int i = 0; i < objs; i++) {
	int p = alloc_order[i];
	if (p < nlive) live[p] = Live (p), create_order += ({p});
	else p -= nlive, dead[p] = Dead (p), create_order += ({-p - 1});
      }
      destruct_order = ({""}); // Using ({}) would alloc a new array in destructing().
      setup();
      live = dead = live_nested = dead_nested = 0;
      int garbed = gc();
      gc();			// To garb live object leftovers.
      destruct_order = destruct_order[1..];
      if (!got_error && (got_error = sizeof (destruct_order) != nlive + nlnested))
	werror ("\nGC should garb %d live objects, "
		"but took %d.\n", nlive + nlnested, sizeof (destruct_order));
      if (!got_error && (got_error = garbed < 3 * (objs + nlnested + ndnested)))
	werror ("\nGC should garb at least %d things, "
		"but took only %d.\n", 3 * (objs + nlnested + ndnested), garbed);
      if (got_error) {
	werror ("Create order was: " +
		map (create_order, lambda (int i) {
				     if (i < 0) return "dead[" + (-i - 1) + "]";
				     else return "live[" + i + "]";
				   }) * ", " + "\n"
		"Destruct order was: " + destruct_order * ", " + "\n");
	tests_failed += got_error;
	got_error = 0;
	break;
      }
    }
  }
  if (!quiet) {
    werror ("%60s\r", "");
  }
  return ({ tests-tests_failed, tests_failed });
}
