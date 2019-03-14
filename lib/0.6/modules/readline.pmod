#pike 7.1
import Stdio;
import Readline;

protected private object(History) readline_history = History(512);

string _module_value(string prompt, function|void complete_callback)
{
  object rl = Readline();
  rl->enable_history(readline_history);
  rl->set_prompt(prompt);
  if(complete_callback)
    rl->get_input_controller()->
      bind("^I", lambda() {
                   array(string) compl = ({ });
                   string c, buf = rl->gettext();
                   int st = 0, point = rl->getcursorpos();
                   int wordstart = search(replace(reverse(buf),
                                                  ({"\t","\r","\n"}),
                                                  ({" "," "," "})),
                                          " ", sizeof(buf)-point);
                   string word = buf[(wordstart>=0 && sizeof(buf)-wordstart)..
                                    point-1];
                   while((c = complete_callback(word, st++, buf, point)))
                     compl += ({ c });
                   switch(sizeof(compl)) {
                   case 0:
                     break;
                   case 1:
                     rl->delete(point-sizeof(word), point);
                     rl->insert(compl[0], point-sizeof(word));
                     rl->setcursorpos(point-sizeof(word)+sizeof(compl[0]));
                     break;
                   default:
                     rl->list_completions(compl);
                     break;
                   }
                 });
  string res = rl->read();
  destruct(rl);
  return res;
}
