inherit Search.Grammar.Base;

static array build_tree(string query)
{
  ADT.Queue stack=ADT.Queue();

  query=replace(query,({"(", ")"}), ({" ( ", " ) "}) );
  array(string) xq = (query/" ")-({""});

  foreach(xq, string term)
  {
    switch (term) {
     case "(" :
       stack->put( ({ }) );
       break;
     case ")" :
       array tmp=stack->get();
       stack->put( stack->get() + tmp );
       break;
     default :
       stack->put( stack->get() + ({ term }) );
       break;
    }
  }
  return stack->get() || ({});
}

static array(mapping) evaluate_tree(Search.Database db, array tree)
{
  mapping(string:mapping) result = 0;
  string last_operator="+";
  array(mapping)|mapping(string:mapping) current_pages;

  foreach(tree, array|string node)
  {
    if(arrayp(node))
      current_pages = evaluate_tree(db, node);
    else
    {
      if((node=="|") || (node=="+") || (node=="^") || (node=="-"))
	last_operator=node;
      else {
	current_pages = db->lookup_word(node);
	current_pages = mkmapping(current_pages->uri, current_pages);
      }
    }

    if(!result)
      result=current_pages;
    
    if( node!="|" && node!="^" && node!="+" && node!="-")
    {
      switch (last_operator)
      {
        case "-" :  // NOT
	  result=result-current_pages;
	  break;

        case "^" :  // NOT
	  result=result^current_pages;
	  break;

        // FIXME: hits and score should be merged.
        case "+" :  // AND
        default :
	  result=result&current_pages;
	  break;
      }
    }
  }
  return values(result) || ([]);
}


// Plugin callbacks

array(mapping) do_query(Search.Database db, string query)
{
  int t0=gethrtime();
  array tree=build_tree(string_to_utf8(lower_case(query)));
  array(mapping) tmp=evaluate_tree(db, tree);
  werror("Query took %.1f ms.\n",(gethrtime()-t0)/1000.0);
  return tmp;
}
