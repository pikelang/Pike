inherit Search.Grammar.Base;

private array build_tree(string query)
{
  ADT.Queue stack=ADT.queue();

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
  return stack->get();
}

private mapping(string:Search.Document) evaluate_tree(Search.Database db, array tree)
{
  mixed first;
  mapping(string:Search.Document) result = ([]);
  string last_operator="+";
  mapping(string:Search.Document) current_pages;
  array(string) all_words;

  foreach(tree, array|string node)
  {
    if(arrayp(node))
      current_pages = evaluate_tree(node);
    else
    {
      if((node=="|") || (node=="+") || (node=="^") || (node=="-"))
	last_operator=node;
      else
	current_pages = db->lookup_word(node);
    }
    
    if( node!="|" && node!="^" && node!="+" && node!="-")
    {
      switch (last_operator)
      {
        case "+" :  // AND
	  result=result&current_pages;
	  break;
        case "-" :  // NOT
	  result=result-current_pages;
	  break;
        case "^" :  // NOT
	  result=result^current_pages;
	  break;
        default :  // AND
	  result=result&current_pages;
	  break;
      }
    }
  }
  return result;
}


// Plugin callbacks

mapping do_query(Search.Database db, string query)
{
  array tree=build_tree(query);
  return evaluate_tree(db, tree);  
}
