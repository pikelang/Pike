ADT.Stack st;

array find_local_names(string file, int line)
{
  st = ADT.Stack();

  string f = Stdio.read_file(file);
  
  int current_block = 0;
  int stop_line = line;
  int func_block = 0;
  mapping current_locals;

  array t = Parser.Pike.tokenize(Parser.Pike.split(f), file);

 // werror("T: %O\n", t);
  
  mapping locals = (["locals" : ({}) ]);

  st->push(locals);
  current_locals = locals;

  foreach(t; int i; Parser.Pike.Token token)
  {
    int xt = 0;

	// werror("text: %O\n", token->text);
    if(token == "{" && t[i-1] != "(")
    {
     current_block ++;
     write(("  "*current_block) + "in\n");
     locals = (["locals" : ({}) ]);
     st->push(locals);
     current_locals = locals;
     continue;
    }
    else if (token == "}" && t[i+1] != ")")
    {
     write(("  "*current_block) + "out\n");
     current_block --;
     if(token->line > stop_line) break;

     st->pop();
     current_locals = st->top();
     continue;
    }

   array x;

   switch(token->text)
   {   
     case "int":
     case "float":
     case "string":
     case "array":
     case "mapping":
     case "multiset":
     case "function":
     case "object":
       x = get_var(t,i+1);
       if(sizeof(x)) 
       {
//         werror("%s: %s\n", token->text, x[1]*", ");
         i = x[0];     
         foreach(x[1], string vn)
           current_locals->locals+=({ ({token->text, vn, token->line}) });
       }
     xt=1;
     continue;
     break;
     
   }

   string tx="";

   if(t[i+1]->text ==".")
   {
//     write("got a .\n");
     int z=i;

     do {

     tx+=t[z]->text;
     if(t[z+1] == ".") tx +=".";
     else break;
     z=z+2;  
     }
     while(1);
     i = z;
   }
   else tx = token->text;

//   write("candidate token: %s\n", tx);

   if(!xt && is_object_identifier(tx))
   {
       x = get_var(t,i+1, 1);
       if(sizeof(x))
       {
         i = x[0];     
//          werror("%s: %s\n", tx, x[1]*", ");
         foreach(x[1], string vn)
           current_locals->locals+=({ ({token->text, vn, token->line}) });
       }
   }

  }
  mapping block_locals;
  array outdata=({});
// werror("sizeof blocks: %d\n", sizeof(st));
  array vals = ({});
  if(sizeof(st) > 1) vals = values(st)[1..];
//vals=values(st);
  foreach(vals, block_locals)
  {
     foreach(block_locals->locals, array cv)
     {
       outdata+=({cv});
     }
  }
//  write("current_block: %d\n", current_block);
  return outdata;
}

#define FOUND_VAR 1
#define LOOKING_VAR 2
#define IN_LIMITATION 3
#define FOUND_VAR 4
#define COMPLETE 5
#define IDLE 6
#define HAD_LIMITATION 1

array get_var(array tokens, int loc, void|int obj)
{
  array vn=({});
  int state, state2;
  int parens = 0;
  string lt;

  werror("get_var(%d, %O) => %O\n", loc, obj, tokens[loc]);

  state = LOOKING_VAR;

  do
  {
    if(tokens[loc] == ";" || (tokens[loc] == "{" && state != IN_LIMITATION))
    {
//      write("end of the line!\n");
      if(lt != 0) { 
//         werror("adding %s to varnames\n", lt);
         vn+=({ lt });
      }
      if(sizeof(vn))
        return ({loc, vn});
      else return ({});
    }
 
    else if(state2 != HAD_LIMITATION
       && tokens[loc] == "(") // we're starting a variable limitation
    {
      state = IN_LIMITATION;
      parens++;
      loc++;
      continue;
    }

    else if(state == IN_LIMITATION && tokens[loc] == ")")
    {
      parens --;
      if(!parens) // we are back out of the limitation
      {
        state = LOOKING_VAR;
        state2 = HAD_LIMITATION;
      }
      loc++;
      continue;
    }

    else if(state == IN_LIMITATION)
    {
      loc++;
      continue;
    }

    else if(is_whitespace(tokens[loc]->text))
    {
       loc ++;
       continue;
    }

    else if (state2 == HAD_LIMITATION && tokens[loc] =="(" && state != IDLE) // started a function, probably.
    {
      werror("parsing function, %s.\n", lt);
      state = IDLE;
      lt = 0;
      loc++;
      continue;
    }

    else if(state == FOUND_VAR && tokens[loc] !=",")
    {
      state = IDLE;
    }

    else if((state == FOUND_VAR) && 
          tokens[loc] ==",") // we're looking for another var name.
    {
      state = LOOKING_VAR;
      write ("adding %s to varnames\n", lt);
      vn+=({lt});
      lt=0;
      loc++;
      continue;
    }

    else if((state == LOOKING_VAR) && 
      is_identifier(tokens[loc]->text))
    { 
      state = FOUND_VAR;
      state2 = HAD_LIMITATION;
      write("%s is an identifier!\n", tokens[loc]->text);
      lt = tokens[loc]->text;
    }

    else
    {
       state = IDLE;
       loc ++;
       continue;
    }

    loc ++;

  } while(loc < sizeof(tokens));

}

int is_identifier(string t)
{
  switch(t)
  {
    case "if":
    case "else":
    case "return":
    case "while":
    case "do":
    case "catch":
    case "gauge":
    case "for":
    case "int":
    case "float":
    case "array":
    case "string":
    case "mapping":
    case "multiset":
    case "object":
    case "function":
    case "void":
    case "mixed":
	case "constant":
	case "goto":
	case "this":
      return 0;
    break;
  }
  object r = Regexp.SimpleRegexp("^[a-zA-Z_][a-zA-Z0-9_]*$");

  return r->match(t);
}

int is_object_identifier(string t)
{
  foreach(t/".", string s)
    if(!is_identifier(s)) return 0;

  return 1;
}

int is_whitespace(string t)
{
  if(t == " " || t == "\n" || t == "\t")
    return 1;
  return 0;
}