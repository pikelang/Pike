
/* function *get_function_list(object); */
void f_get_function_list(int num_arg,struct svalue *argp)
{
  int e;
  int funs;
  struct program *p;
  struct vector *v;
  struct object *o;
  o=argp[0].u.ob;

  p=o->prog;
  for(funs=e=0;e<p->num_funindex;e++)
    if(!(p->function_ptrs[e].type & TYPE_MOD_STATIC))
      funs++;

  v=allocate_array_no_init(funs,0);
  funs=0;

  for(e=0;e<p->num_funindex;e++)
  {
    if(!(p->function_ptrs[e].type & TYPE_MOD_STATIC))
    {
      v->item[funs].type=T_FUNCTION;
      v->item[funs].u.ob=o;
      v->item[funs].subtype=e;
      add_ref(o,"get_function_list");
      funs++;
    }
  }
  v->types=BIT_FUNCTION;
  free_svalue(argp);
  argp->type=T_ARRAY;
  argp->u.vec=v;
}

/* int function_args(function); */
void f_function_args(int num_arg,struct svalue *argp)
{
  int i;
  i=FUNC(argp[0].u.ob->prog,argp[0].subtype)->num_arg;
  free_svalue(argp);
  argp->type=T_INT;
  argp->subtype=0;
  argp->u.number=i;
}

