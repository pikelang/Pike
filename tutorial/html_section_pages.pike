inherit "html.pike";

void split_and_remove_section(object t,object prev)
{
   if (t->data)
      foreach (t->data,object t2)
	 if (objectp(t2))
	 {
	    split_and_remove_section(t2,t->tag=="anchor"?t:0);
	    if (t2->tag=="section")
	    {
	       object tq;
	       if (prev)
	       {
		  sections[t2->params->number]=({t});
		  prev->data-=({t});
	       }
	       else
	       {
		  sections[t2->params->number]=({t2});
		  t->data-=({t2});
	       }
	    }
	 }
}
