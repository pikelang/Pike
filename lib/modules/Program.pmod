constant inherit_list = __builtin.inherit_list;
constant inherits = __builtin.program_inherits;
constant implements = __builtin.program_implements;
constant defined = __builtin.program_defined;

array(program) all_inherits(program p)
{
  array(program) ret = inherit_list(p);
  for(int e=0;e<sizeof(ret);e++) ret+=inherit_list(ret[e]);
  return ret;
}

array(program) inherit_tree(program p)
{
  return ({ p })+
    Array.map(inherit_list(p),inherit_tree);
}


