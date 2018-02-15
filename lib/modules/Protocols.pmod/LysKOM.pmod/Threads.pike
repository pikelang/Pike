#pike __REAL_VERSION__

import ".";

class Thread
{
  object root;
  multiset unread_numbers=(<>);
  mapping(int:object) textno_to_node;
  mapping(int:object) unread_texts;
  int max_follow;  

  class Node
  {
    Session.Text text;
    int conf_no;
    Node parent;
    int unread;
  
    array(Node) children = ({ });

    string get_aux_item_author(object text)
    {
      string name, email;
      if(sizeof(text->aux_items["mx-author"]))
	name=text->aux_items["mx-author"][0]->data;
      if(sizeof(text->aux_items["mx-from"]))
	email=text->aux_items["mx-from"][0]->data;
      if(name && email)
	return sprintf("%s <%s>",name,email);
      if(name)
	return name;
      if(email)
	return email;
      return 0;
    }


    array(mapping) flatten(int depth)
    {
      string author_name;
      catch {
	author_name=text->author->name;
      };
      if(!author_name)
	author_name="Deleted person";
      return ({ ([ "no":         (string)text->no,
		   "author_no":  (string)text->author->no,
		   "author_name":get_aux_item_author(text)||author_name,
		   "subject":    text->subject,
		   "unread":     (unread?"un":""),
		   "depth":      (string)depth ]),
		children->flatten(depth+1) });
    }
    

    Node possible_parent(int follow)
    {
      foreach(text->misc->comm_to, Session.Text _parent)
      {
	if(catch(_parent->misc))
	  continue;
	foreach( ({ @_parent->misc->recpt->conf,
		    @_parent->misc->ccrecpt->conf,
		    @_parent->misc->bccrecpt->conf }),
		 Session.Conference rcpt_conf)
	  if(rcpt_conf->no == conf_no)
	    if(textno_to_node[_parent->no])
	      return textno_to_node[_parent->no];
	    else if(follow || unread_numbers[_parent->no])
	      return Node(_parent, conf_no, follow);
	return 0;
      }
    }
    
    array(Node) possible_children(int follow)
    {
      foreach(text->misc->comm_in, Session.Text child)
      {
	if(catch(child->misc))
	  continue;
	foreach( ({ @child->misc->recpt->conf,
		    @child->misc->ccrecpt->conf,
		    @child->misc->bccrecpt->conf }),
		 Session.Conference rcpt_conf)
	  if(rcpt_conf->no == conf_no)
	    if(textno_to_node[child->no])
	      children += ({ textno_to_node[child->no] });
	    else if(follow || unread_numbers[child->no])
	      children += ({ Node(child, conf_no, follow) });
      }
      return children;
    }
    
    void create(Session.Text _text, int _conf_no, int follow)
    {
      text=_text;
      conf_no=_conf_no;
      
      textno_to_node[text->no]=this;
      _text->clear_stat;

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
      //   werror("Parent to %d: %d\n",_text->no, parent && parent->text->no);
      //   werror("Children to %d: %s\n",_text->no, ((array(string))(children->text->no))*", ");
    }
  }

  void create(mapping(int:Session.Text) _unread_texts,
	      int conf_no, Session.Text start_from,
	      int _max_follow, mapping _textno_to_node)
  {
    unread_texts=_unread_texts;
    textno_to_node=_textno_to_node;
    unread_numbers=(< @indices(unread_texts) >);
    max_follow=_max_follow;
    
    Node start_node=Node(start_from, conf_no, max_follow);
    Node temp;

    temp=start_node;
    do
    {
      root=temp;
    } while(temp=temp->parent);
    unread_numbers=0;
  }
}

array(Thread) children;

object parent=0;

void create(array(Session.Text) unread_texts,
	    int conf_no, int max_follow,
	    mapping(int:object) textno_to_node)
{
  children=({ });
  mapping m_unread_texts=mkmapping(unread_texts->no,
				   unread_texts);
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
    object t=Thread(m_unread_texts, conf_no,
		    m_unread_texts[unread_texts_no],
		    max_follow,
		    textno_to_node);
    children += ({ t->root });
  }
  foreach(children, object thread)
    thread->parent=this;
}
