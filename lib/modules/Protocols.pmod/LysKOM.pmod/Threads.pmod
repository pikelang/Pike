import ".";

class Thread
{
  object root;
  multiset unread_numbers=(<>);
  mapping(int:object) textno_to_node=([]);
  mapping(int:object) unread_texts;
  int max_follow;  

  class Node
  {
    Session.Text text;
    Session.Conference conf;
    Node parent;
    int unread;
  
    array(Node) children = ({ });

    string print(int depth)
    {
      return sprintf("<table cellspacing=1 cellpadding=0 border=0><tr><td><img src=/internal-roxen-unit alt='' height=1 width=%d></td><td><a target=display href=\"%s\">%d</a> - %s (%s)</td></tr></table>",
		     depth*18,
		     "/text.html?id="+text->no,
		     text->no,
		     text->subject,
		     text->author->name)+
	children->print(depth+1)*"\n";      
    }

    Node possible_parent(int follow)
    {
      foreach(text->misc->comm_to, Session.Text _parent)
      {
	if(!_parent->misc)
	{
	  werror("Parent %d. Error: %O\n",_parent->no,
		 _parent->error && _parent->error->name);
	  continue;
	}
	foreach( ({ @_parent->misc->recpt->conf,
		    @_parent->misc->ccrecpt->conf,
		    @_parent->misc->bccrecpt->conf }),
		 Session.Conference rcpt_conf)
	  if(rcpt_conf->no == conf->no)
	    if(textno_to_node[_parent->no])
	      return textno_to_node[_parent->no];
	    else if(follow || unread_numbers[_parent->no])
	      return Node(_parent, conf, follow);
      }
    }
    
    array(Node) possible_children(int follow)
    {
      foreach(text->misc->comm_in, Session.Text child)
      {
	if(!child->misc)
	{
	  werror("Child %d. Error: %O\n",child->no,
		 child->error && child->error->name);
	  continue;
	}
	foreach( ({ @child->misc->recpt->conf,
		    @child->misc->ccrecpt->conf,
		    @child->misc->bccrecpt->conf }),
		 Session.Conference rcpt_conf)
	  if(rcpt_conf->no == conf->no)
	    if(textno_to_node[child->no])
	      children += ({ textno_to_node[child->no] });
	    else if(follow || unread_numbers[child->no])
	      children += ({ Node(child, conf, follow) });
      }
      return children;
    }
    
    void create(Session.Text _text, Session.Conference _conf, int follow)
    {
      text=_text;
      conf=_conf;
      
      textno_to_node[text->no]=this_object();

      unread=unread_numbers[text->no];
      if(!unread)
	follow--;
      else
      {
	m_delete(unread_texts, text->no);
	follow=max_follow;
      }
      
      parent=possible_parent(follow);
      children=possible_children(follow);
    }
  }

  void create(mapping(int:Session.Text) _unread_texts, Session.Conference conf,
	      Session.Text start_from, int _max_follow)
  {
    unread_texts=_unread_texts;
    unread_numbers=(< @indices(unread_texts) >);
    max_follow=_max_follow;
    
    Node start_node=Node(start_from, conf, max_follow);
    Node temp;

    temp=start_node;
    do
    {
      root=temp;
    } while(temp=temp->parent);
    unread_numbers=textno_to_node=0;
  }
}


array(Thread) get_unread_threads(array(Session.Text) unread_texts,
			       Session.Conference conf, int max_follow)
{
  array threads=({ });
  mapping m_unread_texts=mkmapping(unread_texts->no, unread_texts);
  m_delete(m_unread_texts,0);
  foreach(unread_texts->no-({0}), int no)
  {
    if(!(m_unread_texts[no]->misc))
      m_delete(m_unread_texts,no);
  }
  foreach(unread_texts->no, array(int) unread_texts_no)
  {
    if(!m_unread_texts[unread_texts_no])
      continue;
    threads += ({ Thread(m_unread_texts, conf,
			 m_unread_texts[unread_texts_no], max_follow) });
  }
  return threads;
}
