array(int) a() {
  int got_error = 0;
  array(string) destruct_order;

  // Ugly use of constants since I'm too lazy to make a local resolver.

  add_constant ("destructing", lambda (string id) {destruct_order += ({id});});
  add_constant ("my_error", lambda (string s, mixed... args) {
			      if (!got_error) werror ("\n");
			      werror (s, @args);
			      got_error++;
			    });

  mapping valid_vars = (["nk1": 1, "nk2": 1, "n1": 1, "n2": 1, "n3": 1,
			 "wk1": 1, "w1": 1, "w2": 1,
			 "checkfn": 1]);

  add_constant ("B_dead", compile_string ("# " + __LINE__ + #"\n
      string id;
      // [n]ormal or [w]eak ref, should be [k]ept after gc.
      mixed get_nk1() {return 1;}
      mixed get_nk2() {return 1;}
      mixed get_n1()  {return 0;}
      mixed get_n2()  {return 0;}
      mixed get_n3()  {return 0;}
      mixed get_wk1() {return 1;}
      mixed get_w1()  {return 0;}
      mixed get_w2()  {return 0;}
      function(object:mixed) checkfn;
      void check_live (mapping(object:int) checked) {
	//werror (\"check_live %s\\n\", id);
	checked[this_object()] = 1;
	if (!get_nk1()) my_error (id + \"->nk1 got destructed too early.\\n\");
	else if (!get_nk2()) my_error (id + \"->nk2 got destructed too early.\\n\");
	else if (!get_wk1()) my_error (id + \"->wk1 got destructed too early.\\n\");
	else if (functionp (checkfn) && !checkfn (this_object()))
	  my_error (id + \"->checkfn failed.\\n\");
	else {
	  mixed v;
	  if (objectp (v = get_nk1()) && !checked[v]) v->check_live (checked);
	  if (objectp (v = get_nk2()) && !checked[v]) v->check_live (checked);
	  if (objectp (v = get_n1())  && !checked[v]) v->check_live (checked);
	  if (objectp (v = get_n2())  && !checked[v]) v->check_live (checked);
	  if (objectp (v = get_n3())  && !checked[v]) v->check_live (checked);
	  if (objectp (v = get_wk1()) && !checked[v]) v->check_live (checked);
	  if (objectp (v = get_w1())  && !checked[v]) v->check_live (checked);
	  if (objectp (v = get_w2())  && !checked[v]) v->check_live (checked);
	}
	//werror (\"check_live %s done\\n\", id);
      }
    ", __FILE__));

  add_constant ("B_live", compile_string ("# " + __LINE__ + #"\n
      inherit B_dead;
      void destroy() {
	destructing (id);
	//werror (\"destroy %s\\n\", id);
	check_live (([]));
      }
    ", __FILE__));

  add_constant ("B_live_nested_0", compile_string ("# " + __LINE__ + #"\n
      inherit B_live;
      void check_live_0 (mapping(object:int) checked) {check_live (checked);}
      class B_live_nested_1
      {
	inherit B_live;
	void check_live (mapping(object:int) checked) {
	  checked[this_object()] = 1;
	  if (catch (check_live_0 (checked)))
	    my_error (\"Parent for %s got destructed too early.\\n\", id);
	  else ::check_live (checked);
	}
        void check_live_1 (mapping(object:int) checked) {check_live (checked);}
	class B_live_nested_2
	{
	  inherit B_live;
	  string id = \"live_nested[2]\";
	  void check_live (mapping(object:int) checked) {
	    checked[this_object()] = 1;
	    if (catch (check_live_1 (checked)))
	      my_error (\"Parent for %s got destructed too early.\\n\", id);
	    else ::check_live (checked);
	  }
	}
	class B_live_nested_3
	{
	  inherit B_live;
	  string id = \"live_nested[3]\";
	  void check_live (mapping(object:int) checked) {
	    checked[this_object()] = 1;
	    if (catch (check_live_1 (checked)))
	      my_error (\"Parent for %s got destructed too early.\\n\", id);
	    else ::check_live (checked);
	  }
	}
      }
    ", __FILE__));

  add_constant ("B_dead_nested_0", compile_string ("# " + __LINE__ + #"\n
      inherit B_dead;
      void check_live_0 (mapping(object:int) checked) {check_live (checked);}
      class B_dead_nested_1
      {
	inherit B_dead;
	void check_live (mapping(object:int) checked) {
	  checked[this_object()] = 1;
	  if (catch (check_live_0 (checked)))
	    my_error (\"Parent for %s got destructed too early.\\n\", id);
	  else ::check_live (checked);
	}
        void check_live_1 (mapping(object:int) checked) {check_live (checked);}
	class B_dead_nested_2
	{
	  inherit B_dead;
	  void check_live (mapping(object:int) checked) {
	    checked[this_object()] = 1;
	    if (catch (check_live_1 (checked)))
	      my_error (\"Parent for %s got destructed too early.\\n\", id);
	    else ::check_live (checked);
	  }
	}
	class B_dead_nested_3
	{
	  inherit B_dead;
	  void check_live (mapping(object:int) checked) {
	    checked[this_object()] = 1;
	    if (catch (check_live_1 (checked)))
	      my_error (\"Parent for %s got destructed too early.\\n\", id);
	    else ::check_live (checked);
	  }
	}
      }
    ", __FILE__));

  array(array) destruct_order_tests = ({
    ({3,			// Wanted number of live objects.
      0,			// Wanted number of dead objects.
      0,			// Wanted live nested objects.
      0,			// Wanted dead nested objects.
      // Assignments to connect them on the form "obj1->var = obj2".
      ({"live_0->n1 = live_1",
	"live_0->nk1 = live_2",
	"live_1->n1 = live_0",
      })}),
    ({2, 2, 0, 0,	// 1
      ({"live_0->n1 = live_1", "live_0->nk1 = dead_0",
	"live_1->n1 = live_0",
	"dead_0->nk1 = dead_1",
	"dead_1->nk1 = dead_0",
      })}),
    ({1, 2, 0, 0,	// 2
      ({"live_0->nk1 = live_0", "live_0->nk2 = dead_0",
	"dead_0->nk1 = dead_1",
	"dead_1->nk1 = dead_0",
      })}),
    ({0, 3, 0, 0,	// 3
      ({"dead_0->nk1 = dead_1",
	"dead_1->nk1 = dead_0",
	"dead_2->nk1 = dead_0", "dead_2->nk2 = dead_2",
      })}),
    ({3, 0, 0, 0,	// 4
      ({"live_0->nk1 = live_0", "live_0->nk2 = live_1",
	"live_1->nk1 = live_2",
      })}),
    ({1, 2, 0, 0,	// 5
      ({"live_0->nk1 = live_0", "live_0->nk2 = dead_0",
	"dead_0->nk1 = dead_1",
      })}),
    ({1, 2, 0, 0,	// 6
      ({"live_0->nk1 = live_0", "live_0->nk2 = dead_1",
	"dead_0->nk1 = dead_0", "dead_0->nk2 = dead_1",
	"dead_1->nk1 = dead_1",
      })}),
    ({2, 2, 0, 0,	// 7
      ({"live_0->nk1 = live_0", "live_0->nk2 = live_1",
	"dead_0->nk1 = dead_0",
	"dead_0->nk2 = live_1",
	"live_1->nk1 = dead_1",
	"dead_1->nk1 = dead_1",
      })}),
    ({1, 3, 0, 0,	// 8
      ({"live_0->nk1 = live_0", "live_0->nk2 = dead_2",
	"dead_0->nk1 = dead_0",
	"dead_0->nk2 = dead_2",
	"dead_2->nk1 = dead_1",
	"dead_1->nk1 = dead_1",
      })}),
    ({3, 1, 0, 0,	// 9
      ({"live_0->nk1 = live_0", "live_0->nk2 = live_1",
	"dead_0->nk1 = dead_0", "dead_0->nk2 = live_1",
	"live_1->nk1 = live_2",
      })}),
    ({1, 3, 0, 0,	// 10
      ({"live_0->nk1 = live_0", "live_0->nk2 = dead_1",
	"dead_0->nk1 = dead_0", "dead_0->nk2 = dead_1",
	"dead_1->nk1 = dead_2",
      })}),
    ({1, 3, 0, 0,	// 11
      ({"live_0->nk1 = live_0", "live_0->nk2 = dead_1",
	"dead_0->nk1 = dead_0", "dead_0->nk2 = dead_1",
	"dead_1->nk1 = dead_1", "dead_1->nk2 = dead_2",
	"dead_2->nk1 = dead_2",
      })}),
    ({5, 0, 0, 0,	// 12
      ({"live_0->n1 = live_1",
	"live_1->n1 = live_0", "live_1->nk1 = live_2",
	"live_2->n1 = live_3",
	"live_3->n1 = live_2", "live_3->nk1 = live_4",
	"live_4->nk1 = live_4",
      })}),
    ({3, 0, 0, 0,	// 13
      ({"live_0->n1 = live_1", "live_0->n2 = live_2",
	"live_1->n1 = live_2",
	"live_2->n1 = live_0",
      })}),
    ({2, 0, 0, 0,	// 14
      ({"live_0->nk1 = live_1", "live_0->nk2 = live_0",
	"live_1->w1 = live_0",
      })}),
    ({2, 0, 0, 0,	// 15
      ({"live_0->nk1 = live_0", "live_0->nk2 = live_1",
	"live_1->w1 = live_0",
      })}),
    ({2, 0, 0, 0,	// 16
      ({"live_0->nk1 = live_0", "live_0->w1 = live_1",
	"live_1->n1 = live_0",
      })}),
    ({3, 0, 0, 0,	// 17
      ({"live_0->nk1 = live_0", "live_0->nk2 = live_1",
	"live_1->w1 = live_2",
	"live_2->nk1 = live_2", "live_2->nk2 = live_1",
      })}),
    ({3, 0, 0, 0,	// 18
      ({"live_0->nk1 = live_1", "live_0->n1 = live_2",
	"live_1->w1 = live_2",
	"live_2->n1 = live_0",
      })}),
    ({4, 0, 0, 0,	// 19
      ({"live_0->n1 = live_0", "live_0->nk1 = live_1", "live_0->nk2 = live_2",
	"live_1->nk1 = live_3",
	"live_2->nk1 = live_3",
	"live_3->w1 = live_0",
      })}),
    ({3, 0, 0, 0,	// 20
      ({"live_0->n1 = live_1",
	"live_1->n1 = live_0", "live_1->nk1 = live_2",
	"live_2->w1 = live_1",
      })}),
    ({4, 0, 0, 0,	// 21
      ({"live_0->w1 = live_1", "live_0->nk1 = live_3",
	"live_1->w1 = live_2",
	"live_2->nk1 = live_0", "live_2->nk2 = live_3", "live_2->n1 = live_2",
	"live_3->nk1 = live_1",
      })}),
    ({2, 1, 0, 0,	// 22
      ({"live_0->nk1 = dead_0", "live_0->n1 = live_1",
	"live_1->n1 = live_0",
	"dead_0->w1 = live_1",
      })}),
    ({2, 1, 0, 0,	// 23
      ({"live_0->nk1 = live_1", "live_0->nk2 = dead_0",
	"live_1->w1 = dead_0",
	"dead_0->n1 = live_0",
      })}),
    ({3, 0, 0, 0,	// 24
      ({"live_0->n1 = live_0", "live_0->nk1 = live_1", "live_0->nk2 = live_2",
	"live_1->w1 = live_0", "live_1->w2 = live_2",
	"live_2->nk1 = live_1",
      })}),
    ({3, 0, 0, 0,	// 25
      ({"live_0->nk1 = live_1",
	"live_1->w1 = live_2",
	"live_2->n1 = live_2", "live_2->nk1 = live_0", "live_2->nk2 = live_1",
      })}),
    ({3, 0, 0, 0,	// 26
      ({"live_0->w1 = live_1", "live_0->nk1 = live_2",
	"live_1->n1 = live_1", "live_1->nk1 = live_0", "live_1->nk2 = live_2",
	"live_2->w1 = live_1",
      })}),
    ({3, 0, 0, 0,	// 27
      ({"live_0->w1 = live_1",
	"live_1->n1 = live_1", "live_1->nk1 = live_0", "live_1->nk2 = live_2",
	"live_2->nk1 = live_0",
      })}),
    ({3, 0, 0, 0,	// 28
      ({"live_0->nk1 = live_0", "live_0->wk1 = live_1",
	"live_1->nk1 = live_1", "live_1->wk1 = live_2",
	"live_2->nk1 = live_2",
      })}),
    ({2, 2, 0, 0,	// 29
      ({"live_0->n1 = live_1", "live_0->wk1 = dead_0",
	"live_1->n1 = live_0",
	"dead_0->nk1 = dead_1",
	"dead_1->nk1 = dead_0",
      })}),
    ({4, 0, 0, 0,	// 30
      ({"live_0->nk1 = live_1", "live_0->nk2 = live_2", "live_0->wk1 = live_3",
	"live_1->w1 = live_0",
	"live_2->nk1 = live_3",
	"live_3->w1 = live_2",
      })}),
    ({2, 1, 0, 0,	// 31
      ({"live_0->nk1 = dead_0",
	"dead_0->nk1 = live_0", "dead_0->nk2 = live_1",
	"live_1->nk1 = live_1",
      })}),
    ({2, 1, 0, 0,	// 32
      ({"live_0->nk1 = live_0", "live_0->nk2 = dead_0",
	"live_1->nk1 = dead_0",
	"dead_0->nk1 = live_1",
      })}),
    ({2, 1, 0, 0,	// 33
      ({"dead_0->nk1 = live_0",
	"live_0->nk1 = dead_0", "live_0->nk2 = live_1",
	"live_1->nk1 = live_1",
      })}),
    ({2, 1, 0, 0,	// 34
      ({"live_0->nk1 = dead_0",
	"dead_0->nk2 = live_0", "dead_0->nk1 = live_1",
	"live_1->nk1 = live_1",
      })}),
    ({2, 1, 0, 0,	// 35
      ({"live_0->nk2 = live_0", "live_0->nk1 = dead_0",
	"live_1->nk1 = dead_0",
	"dead_0->nk1 = live_1",
      })}),
    ({2, 1, 0, 0,	// 36
      ({"dead_0->nk1 = live_0",
	"live_0->nk2 = dead_0", "live_0->nk1 = live_1",
	"live_1->nk1 = live_1",
      })}),
    ({3, 0, 0, 0,	// 37
      ({"live_0->nk1 = live_0", "live_0->wk1 = live_1",
	"live_0->checkfn = lambda (object o) {return o->wk1[0]->w1[0];}",
	"live_1->w1 = live_2",
	"live_2->nk1 = live_1", "live_2->nk2 = live_2",
      })}),
    ({4, 0, 0, 0,	// 38
      ({"live_0->n1 = live_1",
	"live_1->n1 = live_2",
	"live_2->n1 = live_0", "live_2->w1 = live_3",
	"live_3->nk1 = live_1", "live_3->nk2 = live_3",
      })}),
    ({0, 2, 2, 0,	// 39
      ({"dead_0->n1 = dead_0", "dead_0->nk1 = live_nested_0",
	"dead_1->n1 = dead_1", "dead_1->nk1 = live_nested_1",
	"live_nested_0->n1 = live_nested_1",
      })}),
    ({0, 2, 2, 0,	// 40
      ({"dead_0->n1 = dead_0", "dead_0->nk1 = live_nested_0",
	"dead_1->n1 = dead_1", "dead_1->nk1 = live_nested_1",
	"live_nested_0->w1 = live_nested_1",
      })}),
    ({3, 0, 3, 0,	// 41
      ({"live_0->n1 = live_0", "live_0->nk1 = live_nested_0",
	"live_1->n1 = live_1", "live_1->nk1 = live_nested_2",
	"live_2->n1 = live_2", "live_2->nk1 = live_nested_1",
	"live_nested_0->n1 = live_nested_2",
      })}),
    ({4, 0, 4, 0,	// 42
      ({"live_0->n1 = live_0", "live_0->nk1 = live_nested_0",
	"live_1->n1 = live_1", "live_1->nk1 = live_nested_1",
	"live_2->n1 = live_2", "live_2->nk1 = live_nested_2",
	"live_3->n1 = live_3", "live_3->nk1 = live_nested_3",
	"live_nested_0->n1 = live_nested_3",
      })}),
    ({3, 0, 2, 0,	// 43
      ({"live_0->n1 = live_0", "live_0->nk1 = live_nested_0",
	"live_1->n1 = live_1", "live_1->nk1 = live_nested_1",
	"live_nested_0->nk1 = live_2",
	"live_2->n1 = live_nested_1",
      })}),
    ({3, 0, 3, 0,	// 44
      ({"live_0->n1 = live_0", "live_0->nk1 = live_nested_0",
	"live_1->n1 = live_1", "live_1->nk1 = live_nested_2",
	"live_2->n1 = live_2", "live_2->nk1 = live_nested_1",
	"live_nested_0->n1 = live_nested_2",
	"live_nested_1->nk1 = live_0",
      })}),
    ({0, 2, 0, 2,	// 45
      ({"dead_0->n1 = dead_0", "dead_0->nk1 = dead_nested_0",
	"dead_1->n1 = dead_1", "dead_1->nk1 = dead_nested_1",
	"dead_nested_0->n1 = dead_nested_1",
      })}),
    ({0, 2, 0, 2,	// 46
      ({"dead_0->n1 = dead_0", "dead_0->nk1 = dead_nested_0",
	"dead_1->n1 = dead_1", "dead_1->nk1 = dead_nested_1",
	"dead_nested_0->w1 = dead_nested_1",
      })}),
    ({3, 0, 0, 3,	// 47
      ({"live_0->n1 = live_0", "live_0->nk1 = dead_nested_0",
	"live_1->n1 = live_1", "live_1->nk1 = dead_nested_2",
	"live_2->n1 = live_2", "live_2->nk1 = dead_nested_1",
	"dead_nested_0->n1 = dead_nested_2",
      })}),
    ({4, 0, 0, 4,	// 48
      ({"live_0->n1 = live_0", "live_0->nk1 = dead_nested_0",
	"live_1->n1 = live_1", "live_1->nk1 = dead_nested_1",
	"live_2->n1 = live_2", "live_2->nk1 = dead_nested_2",
	"live_3->n1 = live_3", "live_3->nk1 = dead_nested_3",
	"dead_nested_0->n1 = dead_nested_3",
      })}),
    ({3, 0, 0, 2,	// 49
      ({"live_0->n1 = live_0", "live_0->nk1 = dead_nested_0",
	"live_1->n1 = live_1", "live_1->nk1 = dead_nested_1",
	"dead_nested_0->nk1 = live_2",
	"live_2->n1 = dead_nested_1",
      })}),
    ({3, 0, 0, 3,	// 50
      ({"live_0->n1 = live_0", "live_0->nk1 = dead_nested_0",
	"live_1->n1 = live_1", "live_1->nk1 = dead_nested_2",
	"live_2->n1 = live_2", "live_2->nk1 = dead_nested_1",
	"dead_nested_0->n1 = dead_nested_2",
	"dead_nested_1->nk1 = live_0",
      })}),
    ({0, 4, 2, 2,	// 51
      ({"dead_0->n1 = dead_0", "dead_0->nk1 = live_nested_0",
	"dead_1->n1 = dead_1", "dead_1->nk1 = live_nested_1",
	"dead_2->n1 = dead_2", "dead_2->nk1 = dead_nested_0",
	"dead_3->n1 = dead_3", "dead_3->nk1 = dead_nested_1",
	"live_nested_0->n1 = dead_nested_1",
	"dead_nested_0->n1 = live_nested_1",
      })}),
    ({4, 0, 0, 0,	// 52
      ({"live_0->w1 = live_1",
	"live_1->n1 = live_1", "live_1->nk1 = live_0", "live_1->nk2 = live_2",
	"live_2->w1 = live_3",
	"live_3->n1 = live_3", "live_3->nk1 = live_0",
      })}),
    ({4, 0, 0, 0,	// 53
      ({"live_0->w1 = live_1",
	"live_1->n1 = live_1", "live_1->nk2 = live_0", "live_1->nk1 = live_2",
	"live_2->w1 = live_3",
	"live_3->n1 = live_3", "live_3->nk1 = live_0",
      })}),
    ({4, 0, 0, 0,	// 54
      ({"live_0->n1 = live_0", "live_0->w1 = live_1",
	"live_1->w1 = live_2",
	"live_2->n1 = live_2", "live_2->nk1 = live_1", "live_2->nk2 = live_3",
	"live_3->n1 = live_3", "live_3->nk1 = live_0",
      })}),
    ({4, 0, 0, 0,	// 55
      ({"live_0->n1 = live_0", "live_0->w1 = live_1",
	"live_1->w1 = live_2",
	"live_2->n1 = live_2", "live_2->nk2 = live_1", "live_2->nk1 = live_3",
	"live_3->n1 = live_3", "live_3->nk1 = live_0",
      })}),
    ({3, 0, 0, 0,	// 56
      ({"live_0->nk1 = live_2",
	"live_1->n1 = live_1", "live_1->nk1 = live_0", "live_1->nk2 = live_2",
      })}),
    ({3, 0, 0, 0,	// 57
      ({"live_0->nk1 = live_2",
	"live_1->n1 = live_1", "live_1->nk2 = live_0", "live_1->nk1 = live_2",
      })}),
    ({2, 1, 0, 0,	// 58
      ({"live_0->n1 = live_1", "live_0->n2 = dead_0",
	"live_0->checkfn = lambda (object o) {return o->n2[0];}",
	"live_1->n1 = live_0",
	"dead_0->n1 = dead_0",
      })}),
    ({2, 1, 0, 0,	// 59
      ({"live_0->n2 = live_1", "live_0->n1 = dead_0",
	"live_0->checkfn = lambda (object o) {return o->n1[0];}",
	"live_1->n1 = live_0",
	"dead_0->n1 = dead_0",
      })}),
    ({1, 2, 0, 2,	// 60
      ({"live_0->n1 = dead_nested_0", "live_0->n2 = dead_nested_0",
	"dead_0->n1 = dead_0", "dead_0->n2 = dead_nested_0",
	"dead_1->n1 = dead_1", "dead_1->n2 = dead_nested_1",
	"dead_nested_0->n1 = live_0", "dead_nested_0->n2 = dead_nested_1",
      })}),
    ({1, 2, 0, 2,	// 61
      ({"live_0->n1 = dead_nested_0", "live_0->n2 = dead_nested_0",
	"dead_0->n1 = dead_0", "dead_0->n2 = dead_nested_0",
	"dead_1->n1 = dead_1", "dead_1->n2 = dead_nested_1",
	"dead_nested_0->n2 = live_0", "dead_nested_0->n1 = dead_nested_1",
      })}),
    ({3, 0, 0, 0,	// 62
      ({"live_0->n1 = live_1",
	"live_1->n1 = live_0", "live_1->n2 = live_2",
	"live_2->n1 = live_1",
      })}),
    ({3, 0, 0, 0,	// 63
      ({"live_0->n1 = live_1",
	"live_1->n2 = live_0", "live_1->n1 = live_2",
	"live_2->n1 = live_1",
      })}),
    ({2, 0, 2, 0,	// 64
      ({"live_0->n1 = live_1", "live_0->n2 = live_nested_1",
	"live_1->w1 = live_nested_0",
	"live_nested_0->n2 = live_0",
      })}),
    ({2, 0, 2, 0,	// 65
      ({"live_0->n2 = live_1", "live_0->n1 = live_nested_1",
	"live_1->w1 = live_nested_0",
	"live_nested_0->n2 = live_0",
      })}),
    ({1, 1, 3, 0,	// 66
      ({"dead_0->n1 = dead_0", "dead_0->nk1 = live_nested_0",
	"live_nested_0->n1 = live_0", "live_nested_0->n2 = live_nested_2",
	"live_0->n1 = live_nested_1",
      })}),
    ({1, 1, 3, 0,	// 67
      ({"dead_0->n1 = dead_0", "dead_0->nk1 = live_nested_0",
	"live_nested_0->n2 = live_0", "live_nested_0->n1 = live_nested_2",
	"live_0->n1 = live_nested_1",
      })}),
    ({0, 1, 2, 2,	// 68
      ({"dead_0->n1 = dead_0", "dead_0->nk1 = live_nested_0",
	"live_nested_0->n2 = live_nested_1", "live_nested_0->n1 = dead_nested_0",
	"live_nested_1->n1 = dead_nested_1",
	"dead_nested_0->n1 = live_nested_1",
      })}),
    ({0, 1, 2, 2,	// 69
      ({"dead_0->n1 = dead_0", "dead_0->nk1 = live_nested_0",
	"live_nested_0->n1 = live_nested_1", "live_nested_0->n2 = dead_nested_0",
	"live_nested_1->n1 = dead_nested_1",
	"dead_nested_0->n1 = live_nested_1",
      })}),
    ({0, 1, 2, 2,	// 70
      ({"dead_0->n1 = dead_0", "dead_0->nk1 = live_nested_0",
	"live_nested_0->n1 = live_nested_1", "live_nested_0->n2 = dead_nested_1",
	"live_nested_1->n1 = dead_nested_0",
	"dead_nested_0->n1 = live_nested_0",
      })}),
    ({0, 1, 2, 2,	// 71
      ({"dead_0->n1 = dead_0", "dead_0->nk1 = live_nested_0",
	"live_nested_0->n2 = live_nested_1", "live_nested_0->n1 = dead_nested_1",
	"live_nested_1->n1 = dead_nested_0",
	"dead_nested_0->n1 = live_nested_0",
      })}),
    ({2, 0, 2, 0,	// 72
      ({"live_0->n1 = live_1",
	"live_1->n1 = live_nested_1",
	"live_nested_1->n1 = live_0",
	"live_nested_0->n1 = live_1",
      })}),
    ({2, 0, 4, 0,	// 73
      ({"live_0->n1 = live_1", "live_0->n2 = live_nested_2", "live_0->n3 = live_nested_3",
	"live_1->n1 = live_0",
	"live_nested_1->n1 = live_0",
      })}),
    ({2, 0, 4, 0,	// 74
      ({"live_0->n2 = live_1", "live_0->n3 = live_nested_2", "live_0->n1 = live_nested_3",
	"live_1->n1 = live_0",
	"live_nested_1->n1 = live_0",
      })}),
    ({2, 0, 4, 0,	// 75
      ({"live_0->n3 = live_1", "live_0->n1 = live_nested_2", "live_0->n2 = live_nested_3",
	"live_1->n1 = live_0",
	"live_nested_1->n1 = live_0",
      })}),
    ({2, 1, 2, 0,	// 76
      ({"dead_0->n1 = dead_0", "dead_0->nk1 = live_nested_0",
	"live_nested_0->n2 = live_nested_1", "live_nested_0->n1 = live_1",
	"live_nested_1->n1 = live_0",
	"live_0->n1 = live_nested_0",
	"live_1->n1 = live_0",
      })}),
//      ({3, 0, 0, 0, // Not possible without weak refs directly in objects.
//        ({"live_0->n1 = live_0", "live_0->wk1 = live_1",
//          "live_1->n1 = live_1", "live_1->w1 = live_2",
//          "live_2->n1 = live_2", "live_2->nk1 = live_0",
//        })}),
  });

  int tests_failed = 0;
  int tests = 0;
  foreach (destruct_order_tests;
	   int test;
	   [int nlive, int ndead, int nlnested, int ndnested, array setup]) {
    int to_garb = nlive + ndead + nlnested + ndnested;

    foreach (setup; int i; string assignment) {
      array a = setup[i] = array_sscanf (assignment, "%s->%s = %s");
      if (sizeof (a) != 3)
	error ("Invalid format in assignment %O in test %d.\n", assignment, test);
      if (a[1] != "checkfn") to_garb++;
    }

    array(string) obj_names = ({});
    for (int i = 0; i < nlive; i++) obj_names += ({"live_" + i});
    for (int i = 0; i < ndead; i++) obj_names += ({"dead_" + i});
    switch (nlnested) {
      // Don't count the implied parent objects.
      case 1: obj_names += ({"live_nested_0"}); break;
      case 2: obj_names += ({"live_nested_1"}); break;
      case 3: obj_names += ({"live_nested_2"}); break;
      case 4: obj_names += ({"live_nested_2", "live_nested_3"}); break;
      case 5..: error ("Too many live nested in test %d\n", test);
    }
    switch (ndnested) {
      case 1: obj_names += ({"dead_nested_0"}); break;
      case 2: obj_names += ({"dead_nested_1"}); break;
      case 3: obj_names += ({"dead_nested_2"}); break;
      case 4: obj_names += ({"dead_nested_2", "dead_nested_3"}); break;
      case 5..: error ("Too many dead nested in test %d\n", test);
    }

    int n = 1;
    for (int f = nlive + ndead; f > 1; f--) n *= f;
    switch ((mixed) _verbose) {
      case 1:
	werror ("\rGC destruct order test %d, %d permutations    ", test, n); break;
      case 2..:
	werror ("GC destruct order test %d, %d permutations\n", test, n); break;
    }
    tests += n;

    while (n--) {
      array(string) alloc_order = Array.permute (obj_names, n);

      foreach (({"dead", "live"}), string base) {
	int i, j;
	if ((i = search (alloc_order, base + "_nested_2")) >= 0 ||
	    (j = search (alloc_order, base + "_nested_3")) >= 0) {
	  i = i >= 0 ? j >= 0 ? min (i, j) : i : j;
	  alloc_order = alloc_order[..i-1] + ({base + "_nested_1"}) + alloc_order[i..];
	}
	if ((i = search (alloc_order, base + "_nested_1")) >= 0)
	  alloc_order = alloc_order[..i-1] + ({base + "_nested_0"}) + alloc_order[i..];
      }

      // All this fiddling to make sure the objects are created in the
      // permuted order, and that the variables within them are
      // visited in the reverse of that order. It's reversed since the
      // gc will visit the objects in the reverse create order.

      mapping(string:string) class_bodies = ([]);
      string obj_creates = "", var_assigns = "";
      mapping(string:int) positions = mkmapping (alloc_order, indices (alloc_order));
      array(array(string)) setup_left = setup;

      foreach (alloc_order; int i; string obj) {
	string basename = obj[..sizeof (obj) - 3];
	string class_body;

	obj_creates += "object " + obj + " = ";
	if (has_value (obj, "_nested_")) {
	  class_body = "inherit B_" + obj + ";\n";
	  switch (obj[-1]) {
	    case '0': obj_creates += "C_" + obj + "();\n"; break;
	    case '1': obj_creates += basename + "_0->C_" + obj + "();\n"; break;
	    case '2': case '3': obj_creates += basename + "_1->C_" + obj + "();\n"; break;
	  }
	}
	else {
	  class_body = "inherit B_" + basename + ";\n";
	  obj_creates += "C_" + obj + "();\n";
	}
	class_body += "string id=\"" + obj + "\";\n";

	array(array(string)) assigns =
	  Array.sort_array (
	    filter (setup_left, lambda (array a) {
				  return a[0] == obj;
				}),
	    lambda (array a, array b) {
	      int ap, bp;
	      if (zero_type (ap = positions[a[2]]) && a[1] != "checkfn")
		error ("Invalid object %O in test %d.\n", a[2], test);
	      if (zero_type (bp = positions[b[2]]) && b[1] != "checkfn")
		error ("Invalid object %O in test %d.\n", b[2], test);
	      return ap < bp;
	    });
	setup_left -= assigns;

	foreach (assigns, array(string) a) {
	  if (!valid_vars[a[1]])
	    error ("Invalid variable %O in test %d.\n", a[1], test);
	  if (a[1] == "checkfn")
	    var_assigns += obj + "->checkfn = " + a[2] + ";\n";
	  else {
	    class_body += "array " + a[1] + " = " +
	      (a[1][0] == 'w' ? "set_weak_flag (({0}), Pike.WEAK)" : "({0})") + ";\n"
	      "mixed get_" + a[1] + "() {return " + a[1] + "[0];}\n";
	    var_assigns += obj + "->" + a[1] + "[0] = " + a[2] + ";\n";
	  }
	}

	class_bodies[obj] = class_body;
      }

      if (sizeof (setup_left))
	error ("Invalid object %O in test %d.\n", setup[0][0], test);

      string prog = "";

      foreach (alloc_order; int i; string obj)
	if (!has_value (obj, "_nested_")) {
	  prog += "class C_" + obj + " {\n" + class_bodies[obj] + "}\n\n";
	}
      foreach (({"dead", "live"}), string base)
	if (string class_body = class_bodies[base + "_nested_0"]) {
	  prog += "class C_" + base + "_nested_0 {\n" + class_body + "\n";
	  if ((class_body = class_bodies[base + "_nested_1"])) {
	    prog += "class C_" + base + "_nested_1 {\n" + class_body + "\n";
	    if ((class_body = class_bodies[base + "_nested_2"]))
	      prog += "class C_" + base + "_nested_2 {\n" + class_body + "}\n";
	    if ((class_body = class_bodies[base + "_nested_3"]))
	      prog += "class C_" + base + "_nested_3 {\n" + class_body + "}\n";
	    prog += "}\n";
	  }
	  prog += "}\n\n";
	}

      prog += "void setup() {\n" + obj_creates + "\n" + var_assigns + "}\n";
      //werror ("\nTest " + alloc_order * ", " + ":\n" + prog + "\n");

      destruct_order = ({""}); // Using ({}) would alloc a new array in destructing().
      object o;
      if (mixed err = catch (o = compile_string(prog)())) {
	werror ("Failed program was:\n\n" + prog + "\n");
	throw (err);
      }
      gc();
      o->setup();
      int garbed = gc();
      destruct_order = destruct_order[1..];

      if (!got_error && (got_error = sizeof (destruct_order) != nlive + nlnested))
	werror ("\nGC should garb %d live objects, "
		"but took %d.\n", nlive + nlnested, sizeof (destruct_order));
      if (!got_error && (got_error = garbed < to_garb))
	werror ("\nGC should garb at least %d things, "
		"but took only %d.\n", to_garb, garbed);
      if (got_error) {
	werror ("Destruct order was: " + destruct_order * ", " + "\n"
		"Setup program was:\n\n" + prog + "\n");
	tests_failed += 1;
	got_error = 0;
	break;
      }

      __signal_watchdog();
    }
  }

  if (_verbose == 1) werror ("\r%60s\r", "");

  add_constant ("destructing");
  add_constant ("my_error");
  add_constant ("B_dead");
  add_constant ("B_live");
  add_constant ("B_live_nested_0");
  add_constant ("B_dead_nested_0");

  return ({ tests-tests_failed, tests_failed });
}
